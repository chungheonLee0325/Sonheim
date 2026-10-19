# 12. Multiplayer Synchronization

Sonheim은 Listen Server에서 게임플레이 결과를 확정하고, 각 화면·Actor가 필요한 상태를 복제합니다. 이 장에서는 **아이템 슬롯, 스킬 실행, Pal 선택, 포획, 보스와 던전 상태**에 실제 사용한 전달 방식을 정리했습니다.

## 요청과 상태 반영

~~~mermaid
flowchart LR
    A["클라이언트 입력"] --> B["Server RPC"]
    B --> C["서버 조건 확인 · 상태 변경"]
    C --> D["프로퍼티 / FastArray<br/>Subobject / Snapshot"]
    D --> E["클라이언트 상태 갱신"]
    E --> F["Widget · Actor 표현"]
    A -. "일부 조작의 로컬 피드백" .-> F
~~~

| 조작 | 서버에서 확정하는 값 | 클라이언트에 반영되는 상태 |
|---|---|---|
| 인벤토리 아이템 이동 | 슬롯 순서·아이템 수량 | `RepItems` → 인벤토리 UI |
| 무기 전환 | 활성 무기 슬롯 | `CurrentWeaponSlot` → 장비 표현 |
| 스킬 사용 | 사용 권한·시전·쿨타임 | `SkillSpecs` + `UBaseSkill`, 애니메이션 RPC |
| Pal 선택 | `CurrentPalIndex` | 소유 플레이어의 선택 UI |
| 포획 | 성공 판정·보유 목록·PartnerOwner | Reveal 이벤트와 확정된 Pal 기록 |
| 던전 진행 | Stage·목표·분기·보스·결과 | `GameState`의 현재 Snapshot |

**로컬에서 RPC를 보냈다고 동작을 확정하지 않습니다.** 스킬·인벤토리·포획 등은 서버가 요청 시점의 상태를 검증합니다. 로컬 피드백이 필요한 일부 조작에는 실제 상태를 변경하지 않는 표시 경로를 별도로 뒀습니다.

## 수신 범위가 다른 상태

| 값 | 실제 복제 방식 | 사용 대상 |
|---|---|---|
| `UInventoryComponent::RepItems / EquippedSlots` | `COND_OwnerOnly` | 소유 플레이어의 슬롯·장비 UI |
| `UInventoryComponent::CurrentWeaponSlot` | 일반 `DOREPLIFETIME` | 무기 상태를 보는 클라이언트 |
| `UQuestComponent::Quests` · `UMonsterDexComponent::Dex` | OwnerOnly FastArray | 플레이어별 퀘스트·도감 |
| `UPalInventoryComponent::OwnedPals / CurrentPalIndex` | `COND_OwnerOnly` | Pal 보유·선택 화면 |
| `USonheimSkillComponent::SkillSpecs` | 일반 FastArray | 시전 상태·쿨타임 정보 |
| `ABossMonster::Status` | 일반 프로퍼티 복제 | 현재 보스 상태 |
| `ASonheimGameState::DungeonStageState` | 일반 프로퍼티 복제 | 던전의 현재 진행 상태 |

`COND_OwnerOnly`는 **전송할 연결의 범위**를 제한하고, 서버 권한 검증은 **누가 그 값을 변경할 수 있는지**를 결정합니다. 또한 일반 `DOREPLIFETIME`이라고 해서 모든 연결이 항상 받는 것은 아니며 Actor의 Relevancy 영향을 받습니다.

## 인벤토리 — FastArray와 UI용 로컬 배열

서버의 `InventoryItems`는 게임플레이 상태이고, `FRepInventoryList`는 슬롯 인덱스·ID·수량을 복제하는 배열입니다.

~~~cpp
// InventoryComponent.cpp
DOREPLIFETIME_CONDITION(UInventoryComponent, RepItems, COND_OwnerOnly);
DOREPLIFETIME_CONDITION(UInventoryComponent, EquippedSlots, COND_OwnerOnly);
DOREPLIFETIME(UInventoryComponent, CurrentWeaponSlot);
~~~

서버는 수량·슬롯 변경 시 `MarkItemDirty`, 배열 구조 변경 시 `MarkArrayDirty`를 사용합니다. 클라이언트는 `OnRep_RepItems()`에서 `RebuildInventoryArrayFromRep()`를 호출해 슬롯 인덱스 기준 `InventoryItems`를 재구성하고 `OnInventoryChanged`로 화면을 갱신합니다.

~~~text
서버 InventoryItems
   → RepItems (FastArray Dirty)
   → OwnerOnly 복제
   → OnRep_RepItems
   → 로컬 InventoryItems 재구성
   → OnInventoryChanged → Slot UI
~~~

[실제 인벤토리·보관함 조작 영상](https://github.com/user-attachments/assets/c594e8a3-2840-456c-ae04-cabaaeb4d8ca)에서는 복제 결과가 최종적으로 슬롯 UI에 반영되는 동작을 확인할 수 있습니다. 영상은 게임플레이 결과를 보여주며, OwnerOnly 전송 범위와 FastArray 경로의 근거는 위 코드입니다.

### 공유 보관함

보관함은 공유 월드 Actor입니다. `UContainerComponent::PreReplication()`은 `Subscribers`가 있을 때만 `RepContainerItems`의 복제를 활성화합니다.

~~~cpp
// ContainerComponent.cpp
const bool bActive = Subscribers.Num() > 0;
DOREPLIFETIME_ACTIVE_OVERRIDE(
    UContainerComponent, RepContainerItems, bActive);
~~~

이 코드는 **구독자가 한 명이라도 있는지** 확인하는 방식입니다. 구독자가 아닌 클라이언트에게 개별적으로 필터를 적용하는 방식이 아니라는 점을 구분했습니다. Viewer가 등록되면 `ForceNetUpdate`도 요청합니다.

## 스킬 — Spec과 실행 객체가 다른 데이터

`USonheimSkillComponent`는 `FSonheimSkillSpecItem`의 `SkillId`·`Level`·`bIsCasting`·`CooldownEndTime`을 FastArray로 복제합니다. UI나 타 시스템은 스킬별 시전 여부와 종료 시각을 이 요약 데이터로 조회할 수 있습니다.

동시에 `EnsureSkillInstance`가 만든 `UBaseSkill` 객체도 `AAreaObject::ReplicateSubobjects()`로 복제합니다.

~~~cpp
// AreaObject.cpp — ReplicateSubobjects() 발췌
for (const auto& Pair : m_SkillComponent->GetSkillInstances())
{
    if (UBaseSkill* Skill = Pair.Value)
        bWroteSomething |= Channel->ReplicateSubobject(
            Skill, *Bunch, *RepFlags);
}
~~~

| 경로 | 전달 내용 |
|---|---|
| `SkillSpecs` FastArray | SkillId·Level·시전 여부·쿨타임 종료 시각 |
| `UBaseSkill` Subobject | `m_CurrentPhase`·`m_CurrentCoolTime`·`m_TargetPos`·`m_NextSkillID` |
| `MultiCast_CastSkill` | 스킬 사용 애니메이션 연출 |

스킬 실행의 실제 타격·비용 소비는 서버 경로이고, 애니메이션 시작은 별도 RPC입니다. Spec과 Subobject 사이 일부 상태는 중복되지만 **UI가 필요한 요약 상태와 실행 객체의 상태가 별도 복제 경로**라는 점이 현재 구조의 특징입니다.

## 로컬 피드백과 서버의 확정 결과

**인벤토리 슬롯 교환:** `bEnableClientPrediction`을 켜면 로컬에서 `InventoryItems`를 복사한 `PredictedInventory`를 교환해 `OnInventoryChanged`에 전달합니다. 원본 배열은 이 과정에서 변경하지 않고, 서버 `RepItems`가 도착하면 확정된 슬롯 순서로 재구성합니다.

~~~cpp
// InventoryComponent.cpp — SwapItems 클라이언트 경로
if (bEnableClientPrediction)
    PerformClientPrediction_SwapItems(FromIndex, ToIndex);
ServerSwapItems(FromIndex, ToIndex);
~~~

이 옵션은 C++ 기본값이 **false**이므로, 실제 Blueprint 설정을 확인하지 않고 모든 인벤토리 이동에 예측이 적용된다고 주장하지 않습니다.

**Pal 슬롯 선택:** 클라이언트는 예상 인덱스를 계산해 `OnSelectedPalChanged`로 HUD를 먼저 갱신하지만 `CurrentPalIndex` 자체는 덮어쓰지 않습니다. `ServerRPC_SwitchPalSlot`에서 서버가 확정한 인덱스를 OwnerOnly로 받고 `OnRep_CurrentPalIndex`로 다시 표시합니다.

두 경우 모두 **표시를 먼저 바꾸는 것과 권위 있는 게임플레이 값을 먼저 바꾸는 것은 다릅니다.**

## 포획 — 연출 이벤트와 소유권 확정 분리

서버는 포획 성공 여부와 `FPalCaptureRevealParams`(연출 시간·Segment·실패 지점)를 결정한 뒤 `Multicast_BeginCaptureReveal`로 연출을 시작합니다. 연출이 끝날 때 `ApplyCaptureOutcome()`에서 실제 소유권을 반영합니다.

~~~mermaid
sequenceDiagram
    participant C as Client
    participant S as Server
    participant M as Monster
    C->>S: 포획 요청
    S->>S: 포획 성공 판정
    S-->>C: Reveal 시작 이벤트
    S->>S: 연출 완료 대기 · Pal 보유 공간 재확인
    S->>M: PartnerOwner · 보유 목록 변경
    M-->>C: 최종 Pal 상태 복제
~~~

연출 시작부터 결과 적용까지 시간이 흐르므로, 그 사이 보유 Pal 수가 달라질 수 있습니다. `ApplyCaptureOutcome()`은 소유권 변경 직전에 보유 공간을 재확인하고, 공간이 없으면 포획 실패 처리로 전환합니다.

## 보스 — 현재 상태와 Montage 이벤트 분리

`ABossMonster::Status`는 복제 프로퍼티로 유지하며 `OnRep_Status()`에서 현재 상태를 반영합니다. 반면 Montage 시작·섹션 이동·종료는 `MulticastPlayMontage` 등의 이벤트로 보냅니다.

던전 진입 직후 보스가 Wake를 시작하면, 클라이언트에게 보스 Actor가 아직 relevant하지 않은 시점에 Multicast가 발생할 수 있습니다. 그 경우 **현재 보스 Status를 받는 것과 이전 Montage 시작 이벤트를 받는 것은 별개**입니다.

이 문제는 Reliable 설정만의 문제가 아니라 **Actor Relevancy, Actor 초기 상태, 이벤트 발생 시점**을 함께 확인해야 하는 사례입니다. 상태 복제만으로 이미 지나간 Montage 이벤트 전체가 자동 재생된다고 설명하지 않습니다.

## 던전 — Snapshot으로 현재 진행 상태 전달

`UDungeonStageRuntimeSubsystem`의 진행 결과를 `ASonheimGameState::DungeonStageState`에 게시합니다. Snapshot에는 `RunId`·`Revision`, Stage·목표·분기·보스 상태·서버 기준 시간·결과가 들어갑니다.

~~~cpp
// DungeonStagePresenter.cpp — OnSnapshot
if (Snapshot.RunId == Latest.RunId &&
    Snapshot.Revision <= Latest.Revision) return;
~~~

같은 실행에서 이미 처리한 Revision 이하라면 `UDungeonStagePresenter`가 다시 반영하지 않습니다. 위젯을 나중에 열거나 다시 만들 때도 마지막 Snapshot을 기반으로 ViewData를 만들기 때문에 Stage·타이머가 UI 생성 시점에 초기화되지 않습니다.

![던전 진행 상태와 별도 파티 데이터를 함께 표시하는 실제 HUD](../Media/Wiki/11_Client_State_Presentation_Pipeline/11_dungeon_hud.png)

목표·Stage·타이머 등 던전 진행 정보는 Snapshot을 통해 전달하고, 파티 HP처럼 다른 컴포넌트가 소유하는 상태는 UI에서 별도로 조합합니다.

Listen Server 호스트는 `PublishDungeonStageState()`에서 `OnRep_DungeonStageState()`를 직접 호출해 서버 로컬 UI에도 변경 알림을 전달합니다. **던전 Snapshot은 UI의 복구에 필요한 현재 값**, 진행 알림이나 보스 Montage RPC는 **발생 시점의 이벤트**라는 구분입니다.

표시 데이터 변환은 [11. Client State Presentation](11_Client_State_Presentation_Pipeline.md), 각각의 게임플레이 구현은 [08. Pal Capture](08_Pal_Capture_Partner_Lifecycle.md)·[10. Boss Encounter](10_Boss_Encounter_Runtime.md)에 정리했습니다.

## 관련 코드

- [InventoryComponent.cpp](../../Sonheim/Source/Sonheim/AreaObject/Player/Utility/InventoryComponent.cpp) · [ContainerComponent.cpp](../../Sonheim/Source/Sonheim/GameObject/Buildings/Utility/ContainerComponent.cpp)
- [PalInventoryComponent.cpp](../../Sonheim/Source/Sonheim/AreaObject/Player/Utility/PalInventoryComponent.cpp) · [PalCaptureComponent.cpp](../../Sonheim/Source/Sonheim/AreaObject/Player/Utility/PalCaptureComponent.cpp)
- [SonheimSkillComponent.cpp](../../Sonheim/Source/Sonheim/AreaObject/Skill/SonheimSkillComponent.cpp) · [AreaObject.cpp](../../Sonheim/Source/Sonheim/AreaObject/Base/AreaObject.cpp)
- [BaseSkill.cpp](../../Sonheim/Source/Sonheim/AreaObject/Skill/Base/BaseSkill.cpp) · [BossMonster.cpp](../../Sonheim/Source/Sonheim/AreaObject/Monster/Boss/BossMonster.cpp)
- [SonheimGameState.cpp](../../Sonheim/Source/Sonheim/GameManager/SonheimGameState.cpp) · [DungeonStagePresenter.cpp](../../Sonheim/Source/Sonheim/UI/Dungeon/DungeonStagePresenter.cpp)
