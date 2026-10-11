# 12. Multiplayer Synchronization

Sonheim은 **Unreal Engine Listen Server**에서 클라이언트의 실행 요청과 서버가 확정한 상태를 분리합니다. 스킬·인벤토리·포획은 서버에서 실제 결과를 결정하고, 필요한 상태를 **일반 프로퍼티 복제·FastArray·UObject 서브오브젝트 복제·GameState Snapshot**으로 전달합니다. 즉시 보여줄 입력 피드백과 순간 연출은 별도로 처리합니다.

## 핵심 구현

| 동기화 대상 | 실제 구현 | 이유 |
|---|---|---|
| **게임플레이 요청** | 소유 Actor/Component의 Server RPC | 클라이언트는 실행 의도를 전달하고 서버가 결과 결정 |
| **플레이어 인벤토리** | `COND_OwnerOnly` FastArray · 장비 목록 | 소유 플레이어가 소비하는 슬롯·수량 정보 |
| **현재 무기 슬롯** | 일반 프로퍼티 복제 | 다른 클라이언트도 무기 전환을 표현해야 함 |
| **스킬** | `FSonheimSkillSpecItem` FastArray **+** `UBaseSkill` 서브오브젝트 복제 | 스킬 목록/시전 상태와 실행 객체의 상태를 별도 경로로 전달 |
| **Pal 보유·선택** | `COND_OwnerOnly` 배열·인덱스 복제 | 소유 목록 및 현재 선택한 Pal의 UI 동기화 |
| **공유 보관함** | FastArray + 구독자 존재 시 프로퍼티 복제 활성화 | 열어 보는 사람이 없을 때 내부 슬롯 복제 제한 |
| **보스** | `ABossMonster::Status` + 던전 Snapshot / Multicast | 현재 전투 상태와 순간 애니메이션·효과를 구분 |
| **던전** | `ASonheimGameState::DungeonStageState` | 늦게 열린 HUD도 현재 실행 상태로 구성 |

복제는 **값을 누가 소유하는지, 어떤 클라이언트가 소비하는지, 나중에 다시 알아야 하는 값인지**를 기준으로 선택했습니다. `COND_OwnerOnly` 같은 복제 조건은 *전송 범위*이고, 데이터 변경을 승인하는 *서버 권한 검증*과는 다른 개념입니다.

## 목차

- [요청과 확정 상태](#요청과-확정-상태)
- [복제 범위와 컬렉션](#복제-범위와-컬렉션)
- [스킬 — FastArray와 UObject 서브오브젝트](#스킬--fastarray와-uobject-서브오브젝트)
- [클라이언트 예측과 서버 결과 반영](#클라이언트-예측과-서버-결과-반영)
- [지속 상태와 순간 이벤트](#지속-상태와-순간-이벤트)
- [보스 Wake와 네트워크 관련성](#보스-wake와-네트워크-관련성)
- [포획 Reveal과 실제 소유권](#포획-reveal과-실제-소유권)
- [던전 Snapshot과 화면 재구성](#던전-snapshot과-화면-재구성)
- [멀티플레이 확인 항목](#멀티플레이-확인-항목)
- [설계 선택과 비용](#설계-선택과-비용)
- [관련 코드 및 문서](#관련-코드-및-문서)

---

## 요청과 확정 상태

클라이언트가 스킬을 사용하거나 아이템을 이동하면 먼저 **Server RPC로 실행 의도**를 전달합니다. 서버는 해당 시점의 스킬 허용 여부·재사용 대기·아이템·현재 상태를 검사하고 게임플레이 결과를 확정합니다. 성공한 결과는 해당 시스템의 복제 상태를 통해 클라이언트에 반영합니다.

~~~mermaid
flowchart LR
    INPUT["클라이언트 입력"] --> REQUEST["Server RPC<br/>실행 요청"]
    REQUEST --> CHECK["서버 조건 검사"]
    CHECK --> MUTATION["서버에서 상태 변경"]
    MUTATION --> REP["프로퍼티 / FastArray<br/>서브오브젝트 / Snapshot"]
    REP --> CLIENT["클라이언트 현재 상태"]
    CLIENT --> UI["Widget · 연출"]
    INPUT -. "지원되는 조작만<br/>로컬 피드백" .-> UI
~~~

| 실행 요청 | 서버에서 확정하는 결과 | 클라이언트에 다시 보이는 상태 |
|---|---|---|
| `Server_TryCastSkill` | 스킬 허용 여부·시전 시작 | 스킬 Spec, `UBaseSkill` 상태, Montage 연출 |
| `ServerSwapItems` | 유효한 슬롯 간 아이템 교체 | OwnerOnly `RepItems` → 인벤토리 화면 |
| `ServerRPC_SwitchPalSlot` | 선택 Pal 인덱스 | OwnerOnly `CurrentPalIndex` → 선택 UI |
| `Server_AttemptCapture` | 포획 확률 판정·적용 시점 | Reveal 이벤트 → 실제 Pal 보유 목록 |
| 던전 입구/레버 상호작용 | 입장·분기·스테이지 진행 | `GameState`의 던전 실행 Snapshot |

클라이언트의 **사전 확인과 RPC 호출이 곧 성공을 뜻하지는 않습니다.** 예를 들어 `USonheimSkillComponent::TryCastSkillById`는 비권한 클라이언트에서 요청을 보낸 뒤 반환하지만, 실제 `UBaseSkill::Activate`와 상태 변경은 서버 경로에서 수행합니다.

RPC는 통신 수단이고, 어떤 상태 변경을 허용할지는 해당 게임플레이 함수가 검사합니다.

---

## 복제 범위와 컬렉션

### 플레이어별 상태와 월드 공유 상태

| 값 | 서버 코드에서 설정한 복제 조건 | 소비 범위 |
|---|---|---|
| `UInventoryComponent::RepItems` | `COND_OwnerOnly`, FastArray | 소유 플레이어의 인벤토리 슬롯 |
| `UInventoryComponent::EquippedSlots` | `COND_OwnerOnly` | 소유 플레이어의 장비 목록 |
| `UInventoryComponent::CurrentWeaponSlot` | 일반 `DOREPLIFETIME` | Actor를 복제받는 클라이언트의 무기 표현 |
| `UMonsterDexComponent::Dex` | `COND_OwnerOnly`, FastArray | 소유 플레이어의 도감 기록 |
| `UQuestComponent::Quests` | `COND_OwnerOnly`, FastArray | 소유 플레이어의 퀘스트 기록 |
| `UPalInventoryComponent::OwnedPals` | `COND_OwnerOnly`, 배열 | 소유 Pal 목록 |
| `UPalInventoryComponent::CurrentPalIndex` | `COND_OwnerOnly` | 현재 선택한 Pal |
| `USonheimSkillComponent::SkillSpecs` | 일반 `DOREPLIFETIME`, FastArray | 해당 캐릭터를 복제받는 클라이언트 |
| `ABossMonster::Status` | 일반 `DOREPLIFETIME` | 해당 보스를 복제받는 클라이언트 |
| `ASonheimGameState::DungeonStageState` | 일반 `DOREPLIFETIME` | 던전의 현재 실행 상태를 소비하는 클라이언트 |

**일반 프로퍼티 복제는 항상 모든 접속자에게 전송된다는 뜻이 아닙니다.** 실제 수신 여부에는 소유 Actor의 네트워크 관련성(Relevancy)과 채널 상태도 영향을 줍니다.

### 인벤토리 FastArray와 로컬 배열

`UInventoryComponent`는 게임플레이·UI에서 사용하는 `InventoryItems`와, 복제용 `FRepInventoryList::Items`를 분리합니다.

~~~cpp
// InventoryComponent.cpp — 서버의 복제 조건
DOREPLIFETIME_CONDITION(UInventoryComponent, RepItems, COND_OwnerOnly);
DOREPLIFETIME_CONDITION(UInventoryComponent, EquippedSlots, COND_OwnerOnly);
DOREPLIFETIME(UInventoryComponent, CurrentWeaponSlot);
~~~

~~~text
서버 InventoryItems
    ↓ 변경한 슬롯을 MarkItemDirty
FRepInventoryList (SlotIndex · ItemID · Count)
    ↓ OwnerOnly FastArray Delta
클라이언트 OnRep_RepItems
    ↓ RebuildInventoryArrayFromRep
InventoryItems 로컬 미러 → OnInventoryChanged → UI
~~~

동일 슬롯의 수량 변경은 `MarkItemDirty`, 슬롯 삽입·삭제 및 전체 재구성에는 `MarkArrayDirty`를 사용합니다. 전체 배열을 매 변경마다 다시 보내는 대신 **바뀐 엔트리 또는 배열 구조 변경을 복제 시스템에 알립니다.**

이 복제 방식은 UI에 표시할 로컬 배열과 서버가 소유한 실제 아이템 상태를 구분한 선택입니다.

### 공유 Container의 복제 활성화

보관함은 개인 인벤토리와 달리 월드에 있는 공유 Actor입니다. `UContainerComponent`도 슬롯 기반 FastArray를 사용하지만, `Subscribers`가 비어 있으면 `RepContainerItems` 프로퍼티의 복제를 비활성화합니다.

~~~cpp
// ContainerComponent.cpp — PreReplication
const bool bActive = Subscribers.Num() > 0;
DOREPLIFETIME_ACTIVE_OVERRIDE(
    UContainerComponent,
    RepContainerItems,
    bActive);
~~~

Viewer가 등록되면 `ForceNetUpdate`로 복제 갱신을 요청합니다. 단, 이 구현은 **구독자가 한 명이라도 있는지를 기준으로 프로퍼티 전체를 활성화**합니다. 클라이언트 연결별로 서로 다른 전송 대상 목록을 적용하는 방식은 아닙니다.

---

## 스킬 — FastArray와 UObject 서브오브젝트

스킬 네트워크 상태는 **두 경로로 복제**됩니다. 따라서 `UBaseSkill`을 복제하지 않고 Spec만 보낸다는 설명은 현재 구현과 맞지 않습니다.

### 1. SkillSpecs — 스킬 요약 상태

`USonheimSkillComponent`는 `FSonheimSkillSpecItem`을 FastArray로 복제합니다.

~~~cpp
// SonheimSkillComponent.h — 필드 발췌
struct FSonheimSkillSpecItem : public FFastArraySerializerItem
{
    int32 SkillId = 0;
    int32 Level = 1;
    bool bIsCasting = false;
    float CooldownEndTime = 0.0f;
};
~~~

서버에서 스킬 활성화·쿨타임 시작·취소가 발생할 때 해당 엔트리를 변경하고 `SkillSpecs.MarkItemDirty`를 호출합니다. 이 상태는 **어떤 스킬이 시전 중인지, 쿨타임 종료 시각이 언제인지** 등을 나타냅니다.

### 2. UBaseSkill — 실행 객체의 복제 프로퍼티

`UBaseSkill`은 필요할 때 `EnsureSkillInstance`가 생성하는 UObject입니다. 생성 시 `BaseSkill_<ID>`처럼 안정적인 이름을 지정하고, `IsSupportedForNetworking`·`IsNameStableForNetworking`을 구현합니다.

~~~cpp
// AreaObject.cpp — ReplicateSubobjects 내부
for (const auto& Pair : m_SkillComponent->GetSkillInstances())
{
    if (UBaseSkill* Skill = Pair.Value)
    {
        bWroteSomething |= Channel->ReplicateSubobject(
            Skill, *Bunch, *RepFlags);
    }
}
~~~

`AAreaObject::GetSubobjectsWithStableNamesForNetworking`에서도 이 인스턴스를 등록합니다. `UBaseSkill::GetLifetimeReplicatedProps`는 다음 값을 복제합니다.

~~~cpp
DOREPLIFETIME(UBaseSkill, m_CurrentPhase);
DOREPLIFETIME(UBaseSkill, m_CurrentCoolTime);
DOREPLIFETIME(UBaseSkill, m_TargetPos);
DOREPLIFETIME(UBaseSkill, m_NextSkillID);
~~~

| 경로 | 복제 내용 | 의미 |
|---|---|---|
| `SkillSpecs` FastArray | `SkillId`, `Level`, `bIsCasting`, `CooldownEndTime` | 스킬별 시전 여부·종료 시각 등 요약 상태 |
| `UBaseSkill` 서브오브젝트 | `m_CurrentPhase`, `m_CurrentCoolTime`, `m_TargetPos`, `m_NextSkillID` | 실행 객체의 Phase·쿨타임·대상 위치·연계 스킬 상태 |

**실제 스킬 판정·비용 소비·효과 적용은 서버에서 실행**합니다. 상태 복제와 별개로 시전 애니메이션 등은 `AAreaObject::MultiCast_CastSkill` 경로를 사용합니다.

이 구조에는 스킬 요약 상태와 실행 객체 상태가 일부 겹치는 비용이 있습니다. 둘을 같은 데이터라고 서술하기보다, **서로 다른 복제 경로와 용도를 가진 현재 구현**으로 설명합니다.

---

## 클라이언트 예측과 서버 결과 반영

### 인벤토리 — 구현된 선택적 예측 경로

슬롯 교환에서는 클라이언트가 서버 결과를 기다리는 동안 **예상 슬롯 배열을 UI에 먼저 표시할 수 있는 경로**를 구현했습니다.

~~~cpp
// InventoryComponent.cpp — SwapItems의 클라이언트 경로
if (bEnableClientPrediction)
{
    PerformClientPrediction_SwapItems(FromIndex, ToIndex);
}
ServerSwapItems(FromIndex, ToIndex);
~~~

`PerformClientPrediction_SwapItems`는 `InventoryItems` 원본 배열을 직접 바꾸지 않고 복사한 `PredictedInventory`를 Swap한 뒤 `OnInventoryChanged`로 표시합니다. 이후 서버 `RepItems`가 도착하면 `OnRep_RepItems`에서 로컬 배열을 재구성하고 UI에 확정 결과를 전달합니다.

**중요:** `bEnableClientPrediction`은 `EditDefaultsOnly`이고 C++ 기본값은 `false`입니다. 즉 **예측 경로가 구현되어 있다는 사실과 실제 실행 설정에서 활성화되어 있다는 주장은 구분**해야 합니다. Blueprint 기본값을 별도로 확인하지 않은 상태에서 모든 인벤토리 조작이 즉시 예측된다고 단정하지 않습니다.

무기 슬롯 전환의 예측 역시 `CurrentWeaponSlot`을 직접 바꾸기보다 `OnWeaponChanged`로 우선 표시를 갱신하고, 실제 슬롯은 서버 복제로 확정합니다.

### Pal 선택 — 화면 피드백 우선 반영

`UPalInventoryComponent::SwitchPalSlot`은 클라이언트에서 현재 `OwnedPals` 수와 `CurrentPalIndex`로 예상 인덱스를 계산하고 `OnSelectedPalChanged`를 먼저 호출합니다.

~~~text
Pal 전환 입력
   ↓
클라이언트 예상 인덱스 → HUD 표시
   ↓ ServerRPC_SwitchPalSlot(Direction)
서버 CurrentPalIndex 확정
   ↓ OwnerOnly Replication
OnRep_CurrentPalIndex → HUD 최종 반영
~~~

클라이언트는 실제 `CurrentPalIndex` 값을 예측값으로 덮어쓰지 않습니다. 서버가 보낸 값이 오면 `OnRep_CurrentPalIndex`로 최종 선택 UI를 반영합니다.

이러한 예측은 **게임플레이 아이템 획득이나 포획 성공을 클라이언트가 미리 확정하는 방식과 다릅니다.**

---

## 지속 상태와 순간 이벤트

화면이 뒤늦게 생성되거나 Actor가 처음 복제될 때도 알아야 하는 값은 현재 상태로 유지합니다. 반면 이미 지나간 연출의 시작 신호는 RPC를 사용할 수 있습니다.

| 복구해야 할 현재 상태 | 순간 연출 이벤트 |
|---|---|
| HP·인벤토리·장비·Pal 선택 | 스킬 사용 몽타주·일부 VFX/SFX |
| 보스 `FBossStatus`의 Phase·Action·Break | 보스 몽타주 시작·섹션 이동·타격 효과 |
| 던전 `FDungeonStageRuntimeState`의 Stage·목표·분기·시간 | 진행 알림·지역명 출력 |
| 포획 완료 뒤 `OwnedPals`와 `PartnerOwner` | 포획 Reveal 애니메이션 |

지속 상태에 속한 정보까지 Multicast 호출 이력에만 의존하면, 늦게 relevant해진 클라이언트가 현재 진행 상황을 복구하기 어렵습니다.

**Reliable RPC는 네트워크 관련성과 무관하게 미래의 모든 수신자에게 연출을 재생해 주는 저장소가 아닙니다.** RPC로 전달하는 사건과 복제로 유지하는 상태는 다른 목적입니다.

---

## 보스 Wake와 네트워크 관련성

던전 진입 직후 플레이어 이동과 보스 Wake가 가까운 시점에 발생하면, 서버가 아직 해당 클라이언트에 대해 보스를 relevant Actor로 보고 있지 않을 수 있습니다. 이 경우 보스의 NetMulticast 몽타주 시작 요청을 해당 클라이언트가 받지 못할 수 있습니다.

~~~text
서버: 던전 입장·플레이어 이동
               ↓
서버: 보스 Wake Multicast
               ↓
보스 Actor가 아직 해당 Client에 relevant하지 않음
               ↓
클라이언트: 그 순간의 Montage 시작을 놓칠 수 있음
~~~

현재 코드에서는 두 성격의 데이터를 분리합니다.

- **상태 복제:** `ABossMonster::Status`는 `DOREPLIFETIME`이며 `OnRep_Status`에서 상태에 맞는 외형을 갱신합니다. 던전 UI용 Phase·Action·Break 등의 요약 상태는 `GameState` Snapshot에도 포함됩니다.
- **순간 연출:** `MulticastPlayMontage`, `MulticastJumpToSection`, `MulticastStopMontage`와 효과 Multicast가 있습니다. `BeginPlay`에서 `Sleeping` 상태의 로컬 몽타주를 재생하는 초기 처리도 있습니다.

따라서 **보스의 현재 상태가 복구되는 것과, 놓친 Wake 몽타주의 해당 시점 연출까지 재생되는 것은 동일하지 않습니다.** 상태 복제만으로 모든 진행 중 몽타주가 자동 복구된다고 설명하지 않습니다.

이 문제는 Reliable 여부만 바꾸는 대신 **Actor relevancy, 초기 상태 수신, 몽타주 이벤트 시점**을 함께 확인해야 하는 사례입니다.

---

## 포획 Reveal과 실제 소유권

포획은 **서버의 성공 판정 → 모두에게 Reveal 연출 → 서버의 최종 소유권 적용** 순서로 처리합니다.

~~~mermaid
sequenceDiagram
    participant C as Client
    participant S as Server
    participant M as Monster
    C->>S: Capture Attempt
    S->>S: 확률·성공 여부 판정
    S-->>C: Multicast_BeginCaptureReveal(Params)
    C->>C: 포획 Reveal 표시
    S->>S: Reveal 시간 대기
    S->>S: 보유 공간 재검증
    S->>M: SetPartnerOwner / AddPal 또는 실패 복구
    M-->>C: 소유 Pal 상태 동기화
~~~

`FPalCaptureRevealParams`는 Segment 수·시간·실패 지점·성공 여부를 포함하며 서버가 구성합니다. 모든 클라이언트가 각자 무작위 성공 여부를 결정하지 않습니다.

연출 시작 시점에 보유 Pal 공간이 있더라도, 대기 중 다른 Pal을 획득했을 수 있습니다. `ApplyCaptureOutcome`은 실제 `PartnerOwner`·`OwnedPals` 변경 직전에 **최대 보유 수를 재검사**합니다. 이때 더 이상 공간이 없으면 실패로 전환하고 Monster를 다시 활성화합니다.

여기서 Multicast는 포획 **연출을 시작하는 신호**, 소유권과 보유 목록은 **서버 확정 상태**입니다.

---

## 던전 Snapshot과 화면 재구성

`UDungeonStageRuntimeSubsystem`은 이벤트·목표·분기·보상 처리 결과를 `FDungeonStageRuntimeState`로 모아 `ASonheimGameState::PublishDungeonStageState`에 전달합니다.

~~~text
서버 Dungeon Runtime
         ↓ Publish
ASonheimGameState::DungeonStageState
         ↓ Replication / OnRep
UDungeonStagePresenter::OnSnapshot
         ↓ 화면 설정과 조합
FDungeonStageViewData
         ↓
HUD · 미니맵 · 결과 Widget
~~~

| Snapshot 정보 | 화면 및 월드 사용 |
|---|---|
| `RunId` · `Revision` · `RunStatus` | 실행 구분·새 상태 반영·HUD/Result 전환 |
| `StageId` · `CurrentCount / RequiredCount` | 현재 목표와 진행도 |
| `SelectedBranchId` · `RunTags` · `SealedBarriers` | 분기·상호작용·장벽 상태 |
| `StageDeadlineServerTime` · `RunStartedServerTime` | 서버 시간 기준 타이머 |
| `Participants` · `BossActionId` · `BossPhase` · `BossBreak` | 파티·보스 상태 |
| `Rewards` · `ClearCount` · `BestSeconds` | 보상·결과·기록 표시 |

새 상태가 이전 `DungeonStageState`와 다를 때 `Revision`을 증가시키고, Listen Server 호스트에도 `OnRep_DungeonStageState`를 명시적으로 호출해 변경 Delegate를 전달합니다.

클라이언트 Presenter는 다음 조건으로 **같은 실행에서 이미 처리한 Revision 이하의 Snapshot**을 제외합니다.

~~~cpp
// DungeonStagePresenter.cpp
if (Snapshot.RunId == Latest.RunId &&
    Snapshot.Revision <= Latest.Revision) return;
~~~

던전 HUD가 제거됐다가 재생성되더라도 **마지막으로 전달받은 현재 ViewData를 다시 적용**합니다. 진행 시간을 위젯 생성 시점부터 세지 않고 동기화된 서버 시각을 사용하므로 표시를 이어갈 수 있습니다.

상태와 화면 데이터를 조합하는 구현은 [[11. UI Architecture & Client Presentation|11_Client_State_Presentation_Pipeline]]에서 설명합니다.

---

## 멀티플레이 확인 항목

아래는 복제 조건과 UI 동작을 확인할 때 사용할 수 있는 **검증 시나리오**입니다. 실제 PIE 테스트를 새로 실행한 결과를 의미하지 않습니다.

| 시나리오 | 확인할 결과 |
|---|---|
| Listen Server + 원격 클라이언트에서 인벤토리 교체 | 소유 클라이언트의 슬롯이 서버 결과와 일치 |
| 원격 클라이언트의 무기 변경 | 다른 화면에서 무기 슬롯 전환 표현이 갱신되는지 |
| Pal 슬롯 전환 | 즉시 UI 피드백과 서버 `CurrentPalIndex` 최종 값이 일치 |
| 공유 보관함 두 명 사용 후 한 명 종료 | 구독자 유무에 따른 `RepContainerItems` 활성화와 UI 반영 |
| 던전 중 HUD 제거·재생성 | 동일 `RunId`·`Revision`의 목표·타이머 복구 |
| 던전 입장 직후 보스 Wake | 현재 `FBossStatus` 동기화와 몽타주 시작 누락 여부를 별도로 확인 |

> **멀티플레이 증빙 캡처 자리 — Server / Client 상태 비교**  
> `Doc/Media/Wiki/12_Multiplayer_Synchronization/12_server_client_state_comparison.png`  
> 동일한 던전 실행에서 Listen Server와 원격 클라이언트 화면을 나란히 두고, `RunId`·`Revision`·`StageId`·`CurrentCount / RequiredCount` 로그를 함께 표시한 캡처를 추가합니다. 플레이 화면만으로 OwnerOnly 복제 여부를 증명할 수는 없으므로, 가능하면 인벤토리·Pal·스킬의 실제 복제 결과도 로그나 디버거로 대조합니다.

---

## 설계 선택과 비용

| 선택 | 적용 효과 | 고려할 비용·한계 |
|---|---|---|
| **서버 권위 + Server RPC** | 공유 게임플레이 결과를 서버에서 결정 | 네트워크 왕복 지연·요청별 검증 필요 |
| **OwnerOnly 복제** | 플레이어 전용 정보를 필요한 수신자에 전송 | Actor 소유권·접속자 변경 관리 |
| **FastArray** | 슬롯·스킬·퀘스트 등 변경된 엔트리 동기화 | Dirty 표시·배열 구조 변경·클라이언트 미러 관리 |
| **UObject 서브오브젝트 복제** | 스킬 실행 객체의 상태 필드 전달 | Actor Channel·서브오브젝트 등록·중복 상태 관리 |
| **조건부 Container 복제** | 구독자가 없을 때 내부 아이템 복제 제한 | 구독자별 독립 전송 필터는 아님 |
| **선택적 로컬 예측** | Pal 선택 등 입력 피드백 지연 감소 | 잘못된 예상 표시를 서버 값으로 되돌려야 함 |
| **던전 Snapshot** | 늦은 UI 생성에서도 현재 단계·결과 복구 | Snapshot 필드 및 ViewData 매핑 관리 |
| **상태 / 순간 RPC 분리** | 현재 상태와 연출 이벤트의 역할 구분 | Relevancy 변화로 놓친 연출의 재동기화 문제 |

Sonheim에서는 모든 기능에 하나의 네트워크 패턴을 강제하지 않았습니다. **지속적으로 알아야 하는 값은 상태 복제로 유지하고, 입력 반응이 필요한 곳에만 로컬 표시 예측을 적용하며, 순간 연출은 별도 이벤트로 처리**합니다.

## 관련 코드 및 문서

- [InventoryComponent.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/AreaObject/Player/Utility/InventoryComponent.cpp) — 인벤토리 OwnerOnly FastArray·로컬 미러·예측
- [PalInventoryComponent.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/AreaObject/Player/Utility/PalInventoryComponent.cpp) — 소유 배열·선택 예측 및 복제
- [ContainerComponent.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/GameObject/Buildings/Utility/ContainerComponent.cpp) — 공유 보관함의 프로퍼티 활성화
- [SonheimSkillComponent.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/AreaObject/Skill/SonheimSkillComponent.cpp) — 스킬 Spec FastArray·Server RPC
- [AreaObject.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/AreaObject/Base/AreaObject.cpp) — `GetSubobjectsWithStableNamesForNetworking`·`ReplicateSubobjects`
- [BaseSkill.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/AreaObject/Skill/Base/BaseSkill.cpp) — `UBaseSkill` 복제 프로퍼티
- [BossMonster.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/AreaObject/Monster/Boss/BossMonster.cpp) — 보스 상태 및 Multicast 연출
- [PalCaptureComponent.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/AreaObject/Player/Utility/PalCaptureComponent.cpp) — 포획 서버 판정·Reveal·소유권 변경
- [SonheimGameState.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/GameManager/SonheimGameState.cpp) — 던전 Snapshot 복제·Revision
- [DungeonStagePresenter.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/UI/Dungeon/DungeonStagePresenter.cpp) — Snapshot 비교·UI 표시 데이터
- [[02. Gameplay Architecture|02_Gameplay_Architecture]] — PlayerState·Pawn·Subsystem 책임
- [[05. Combat, Skill & Animation|05_Combat_Skill_Animation]] — 서버 스킬 실행·공격 판정
- [[07. Inventory & Crafting|07_Inventory_Crafting]] — 인벤토리·제작 상태 관리
- [[08. Pal Capture & Partner Lifecycle|08_Pal_Capture_Partner_Lifecycle]] — 포획·소환 처리
- [[11. UI Architecture & Client Presentation|11_Client_State_Presentation_Pipeline]] — Snapshot → Presenter → ViewData
