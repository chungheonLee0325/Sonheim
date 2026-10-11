# 06. World Interaction Systems

Sonheim은 **아이템·보관함·제작대·던전 레버·입구·퀘스트 NPC에 같은 탐지와 입력 흐름**을 사용합니다. 플레이어의 `UInteractionComponent`는 카메라에서 대상을 찾고 즉시 입력 또는 Hold를 처리합니다. 대상 Actor는 `IInteractableInterface`로 **표시할 안내, 입력 방식, 실제 수행할 동작**을 제공합니다.

## 실제 게임 동작

https://github.com/user-attachments/assets/4ecbc1bc-8c72-4493-8a66-a7b2a0918a36

*기존 게임 영상: 아이템 획득, 보관함, 제작대, 던전 입구·레버가 같은 감지·입력 흐름을 사용하면서 서로 다른 결과로 이어집니다.*

| 공통 부분 | 대상별로 달라지는 부분 |
|---|---|
| 카메라 Trace로 대상 탐색 | 아이템 이름·보관함 열기·제작 작업 등 안내 문구 |
| 탐지/해제 이벤트와 화면 안내 | 즉시 실행 / Hold 시간 / 취소 가능 여부 |
| 입력·Hold 진행도 관리 | 아이템 획득, UI 열기, 제작 기여, 이벤트 전달 |
| Server RPC로 실행 요청 | 대상 Actor·Subsystem의 상태 검사와 실제 결과 적용 |

**확장 지점은 Player의 입력 분기문이 아니라 대상 Actor의 인터페이스 구현**입니다. 전투로 자원을 채집하는 방식은 직접 사용 입력과 다르므로 Combat/Damage 경로를 유지합니다.

## 목차

- [탐지부터 서버 실행까지](#탐지부터-서버-실행까지)
- [인터페이스 계약과 대상별 구현](#인터페이스-계약과-대상별-구현)
- [즉시 입력·Hold·취소 처리](#즉시-입력hold취소-처리)
- [서버 권한과 검증 경계](#서버-권한과-검증-경계)
- [아이템 정의와 월드 생성 옵션](#아이템-정의와-월드-생성-옵션)
- [기존 시스템과의 연동](#기존-시스템과의-연동)
- [선택 기준과 비용](#선택-기준과-비용)
- [관련 코드 및 문서](#관련-코드-및-문서)

---

## 탐지부터 서버 실행까지

~~~mermaid
flowchart TB
    CAMERA["로컬 카메라<br/>ECC_Visibility Trace"] --> DETECT["UInteractionComponent<br/>PerformDetection"]
    DETECT --> FILTER["IInteractableInterface 구현<br/>CanInteract · 가장 가까운 대상"]
    FILTER --> PROMPT["UpdateInteractable<br/>OnDetected · DetectWidget"]
    PROMPT --> INPUT["입력 · Hold 진행"]
    INPUT --> RPC["Server_TryInteract<br/>대상 Actor 전달"]
    RPC --> EXEC["IInteractableInterface::Execute_Interact"]
    EXEC --> RESULT["대상별 게임플레이 처리"]

    DETECT -. "별도 긴 거리 Trace" .-> MONSTER["포획 가능 몬스터<br/>HP 바 표시"]
~~~

### 로컬 플레이어의 대상 탐지

`UInteractionComponent::BeginPlay`는 **로컬 플레이어에서만** 0.1초 간격의 탐지 Timer를 시작합니다. 게임플레이를 차단하는 일반 메뉴가 열려 있으면 `UUIStackSubsystem::IsGameplayBlocked`를 확인하고 현재 상호작용 안내를 해제합니다.

탐지는 같은 카메라 방향에서 두 범위를 사용합니다.

| 용도 | 범위·처리 | 결과 |
|---|---|---|
| **상호작용 대상** | 카메라 전방 500 Unreal Unit까지 `LineTraceMultiByChannel(ECC_Visibility)` | 인터페이스 구현 및 `CanInteract` 확인 후 가장 가까운 대상 선택 |
| **몬스터 정보** | 전방 2,000 Unreal Unit까지 별도 `LineTraceSingleByChannel` | 포획 가능 몬스터라면 HP 바 표시 |
| **대상 변경** | `UpdateInteractable` | 이전 대상 `OnDetected(false)`, 새 대상 `OnDetected(true)`, `OnInteractableChanged` 전달 |

몬스터 HP 바 탐지는 상호작용 대상 선택과 같은 Timer에서 실행되지만, **`IInteractableInterface`를 사용하는 아이템·레버 입력과는 별도 처리**입니다.

### 입력에서 실제 결과까지

`TryInteract`의 핵심 경로는 다음과 같습니다.

~~~cpp
// InteractionComponent.cpp — 주요 실행 경로 발췌
if (GetOwnerRole() != ROLE_Authority)
{
    Server_TryInteract(CurrentInteractable);
    return;
}

if (!CurrentInteractable ||
    !IInteractableInterface::Execute_CanInteract(CurrentInteractable))
{
    return;
}

IInteractableInterface::Execute_Interact(
    CurrentInteractable, OwnerPlayer);
~~~

비권한 클라이언트는 대상 Actor를 인자로 서버에 요청합니다. `Server_TryInteract_Implementation`은 받은 대상을 `CurrentInteractable`로 설정한 뒤 `TryInteract`를 호출합니다. 대상별 `Interact_Implementation`은 각자의 게임플레이 처리를 수행합니다.

이 구조에서 **클라이언트의 로컬 탐지 성공은 서버의 게임플레이 성공을 의미하지 않습니다.** 서버가 실행 중인 콘텐츠 상태와 실제 결과를 최종 판단해야 합니다. 공통 RPC와 각 대상의 검증 범위는 아래에서 구분합니다.

---

## 인터페이스 계약과 대상별 구현

### 공통 인터페이스

`IInteractableInterface`는 `BlueprintNativeEvent`로 선언되어 C++의 `_Implementation` 또는 Blueprint에서 대상별 행동을 구현할 수 있습니다.

| 인터페이스 함수 | 사용 목적 |
|---|---|
| `CanInteract` | 현재 상호작용 가능한 상태인지 확인 |
| `OnDetected` | 탐지·해제에 따른 안내/강조 표시 |
| `GetInteractionName` · `GetInteractableType` | 안내용 이름·대상 종류 제공 |
| `GetHoldDuration` | 0 이하이면 즉시 실행, 양수이면 Hold |
| `UpdateHoldProgressUI` | 인터페이스를 구현한 대상의 Hold 게이지 갱신 |
| `Interact` | 실제 동작 실행 지점 |
| `CanHoldCancel` · `GetCancelHoldDuration` · `ExecuteCancel` | 별도 취소 Hold 지원 |
| `GetInteractionModeCode` | 제작대처럼 의미가 바뀌는 대상의 Hold 모드 식별 |

플레이어는 개별 Actor가 제작대인지 레버인지 검사하지 않고 같은 인터페이스 함수를 호출합니다. 다만 실제 실행 방식과 서버 검증은 **대상 클래스마다 다르게 구현**되어 있습니다.

### 서로 다른 오브젝트, 같은 입력

| 대상 | 사용자에게 보이는 행동 | 실제 `Interact` 경로 |
|---|---|---|
| **`ABaseItem`** | 아이템 이름, 즉시/홀드 획득 | `CanBeCollectedBy → OnCollected → Reward` |
| **`ABaseContainer`** | 보관함 열기 | `OpenContainer` → 사용자 설정·Container UI 열기 |
| **`ACraftingStation`** | 레시피 선택 / 제작 / 취득 | 현재 `ActiveWork`·`CompletedToCollect`에 따라 UI·작업·수령 처리 |
| **`ADungeonShortcutSwitch`** | 지름길 레버 | `UDungeonStageRuntimeSubsystem::TryInteractSwitch` |
| **`ADungeonTestArea`** | 던전 입구 | `UDungeonStageRuntimeSubsystem::TryStart` |
| **`AQuestGiver`** | 퀘스트 대화·보고 | `UQuestComponent::Interact` → 해당 플레이어에게 대화창 열기 |
| **`AQuestBuildSite`** | 퀘스트 재료 전달 | `UQuestComponent::Interact` → 목표 갱신 |

#### 아이템 — 획득 결과는 서버에서 확정

`ABaseItem::Interact_Implementation`은 `CanInteract`와 `CanBeCollectedBy`를 확인합니다. `CanBeCollectedBy`에는 **서버 권한·유효한 플레이어·미수집 상태** 검사가 있고, `OnCollected`가 보상을 적용하고 월드 아이템을 제거합니다.

~~~text
Interact
   ↓ CanBeCollectedBy
OnCollected
   ↓ ApplyEffect
ASonheimPlayer::Reward
   ↓
월드 아이템 Destroy
~~~

서버에서 획득 시각 효과를 `Multicast_OnCollected`로 전달하며, 실제 아이템 획득은 시각 효과 RPC가 아닌 서버 `Reward` 처리에 의해 결정됩니다.

#### 보관함 — 공유 상태와 UI 소유자

`ABaseContainer::CanInteract`는 열림 상태와 `CurrentUser`를 확인합니다. `OpenContainer`는 서버에서 `bIsOpen`·`CurrentUser`를 정하고 해당 플레이어에게 `Client_OpenContainerUI`를 호출합니다.

Container의 아이템 복제는 `UContainerComponent`에서 관리하고, 사용자가 생기면 구독자 등록과 `ForceNetUpdate`로 갱신을 요청합니다. **아이템 데이터와 화면을 누가 열었는지는 서로 다른 상태**입니다.

#### 제작대 — 진행 상태에 따라 행동 변경

`ACraftingStation`은 하나의 Actor에서 다음 동작을 제공합니다.

~~~mermaid
flowchart TB
    INTERACT["Crafting Station Interact"] --> CHECK{"현재 상태"}
    CHECK -->|"수령 가능"| COLLECT["ServerCollectAll<br/>완료 결과 수령"]
    CHECK -->|"진행 중"| WORK["ServerAddWork<br/>작업량 추가"]
    CHECK -->|"대기"| OPEN["ServerRequestOpenUI<br/>레시피 UI"]
    WORK --> AGAIN["다시 상호작용해 다음 작업/수령"]
    COLLECT --> AGAIN
~~~

실제 `Interact_Implementation`은 **진행 중인 작업을 먼저 검사**하고, 수령 가능한 결과가 있어도 `ManualCollectDelay`가 지나야 수령하도록 처리합니다. 수령 직후 `ManualOpenUIDelay`도 확인해 한 입력에서 곧바로 다음 UI가 열리지 않도록 합니다.

별도로 `UIOwner`가 있으면 다른 플레이어가 레시피 선택 UI를 열지 못하게 제한하지만, 작업을 돕는 입력은 진행 상태에 맞게 처리됩니다. 제작의 재료 소비·공동 작업·환불은 [[07. Inventory & Crafting|07_Inventory_Crafting]]에서 설명합니다.

#### 던전 레버·입구 — 이벤트 전달과 실행 규칙 분리

~~~cpp
// DungeonShortcutSwitch.cpp — 주요 경로
void ADungeonShortcutSwitch::Interact_Implementation(
    ASonheimPlayer* Player)
{
    if (HasAuthority())
        GetWorld()->GetSubsystem<UDungeonStageRuntimeSubsystem>()
            ->TryInteractSwitch(this, Player, TestArea, SourceId);
}
~~~

레버는 **`SourceId`와 상호작용 사실을 전달**하고 다음 스테이지를 직접 선택하지 않습니다. `TryInteractSwitch`는 실행 상태·Run Owner·거리·현재 스테이지의 EventRule을 확인합니다. 성공하면 `ActorInteracted` 이벤트가 기록되고, 실제 분기는 Dungeon Definition의 조건·전환 규칙에 따라 결정됩니다.

입구도 `TryStart`에 위임해 거리·요구 레벨·실행 중 여부·Catalog 정의를 검사합니다.

실제 분기와 Stage Graph는 [[09. Branching Dungeon Runtime|09_Branching_Dungeon_Runtime]]에서 확인할 수 있습니다.

#### 퀘스트 NPC·건설 대상 — 동일한 입력 재사용

`AQuestGiver`는 상호작용 시 서버에서 퀘스트의 `Talk / Deliver` 목표를 갱신하고, 플레이어에게 대화창을 엽니다. `AQuestBuildSite`는 자재 전달에 같은 `UQuestComponent::Interact`를 사용하며, 완료된 건설 상태는 `bBuilt` 복제로 모든 클라이언트의 월드 표현을 갱신합니다.

**퀘스트 수락부터 완료까지 실제 게임 영상**은 [[17. Quest System|17_Quest_System]]에 있습니다. 이 문서에서는 퀘스트 목표 집계 대신 **기존 공통 입력을 신규 콘텐츠가 재사용하는 경계**에 집중합니다.

---

## 즉시 입력·Hold·취소 처리

### Hold 입력은 플레이어 컴포넌트가 처리

`UInteractionComponent::StartHoldInteraction`은 대상의 `GetHoldDuration`을 읽습니다.

~~~text
HoldDuration <= 0
    → TryInteract 즉시 실행

HoldDuration > 0
    → 0.01초 Timer로 로컬 진행도 갱신
    → 대상 UpdateHoldProgressUI
    → 완료 시 TryInteract → Server RPC
~~~

인터페이스 구현체가 각각 별도의 입력 Timer를 관리하는 것이 아니라 **플레이어의 Interaction Component가 Hold 진행을 소유**합니다. 작업 수행 시간의 확정과 완료 결과는 대상의 서버 로직이 관리합니다.

제작대처럼 같은 행동을 계속 반복할 수 있는 경우에는 **Hold 완료 뒤 같은 모드가 유지되는 동안 `TryInteract`를 다시 실행**할 수 있습니다. 따라서 모든 Hold가 정확히 한 번 실행된다고 설명하지 않습니다.

### 취소 Hold

`EHoldPurpose::Interact`와 `EHoldPurpose::Cancel`은 서로 다른 입력 목적입니다. 취소는 `CanHoldCancel`이 허용할 때 `GetCancelHoldDuration`의 시간 동안 진행한 다음 `ExecuteCancel`을 호출합니다.

제작대의 `ExecuteCancel_Implementation`은 비권한 클라이언트에서는 `ASonheimPlayerController::Server_Crafting_CancelUnfinished` RPC를 이용하고, 서버에서는 `ServerCancelUnfinished`로 남은 작업을 취소합니다. 완료되어 수령 가능한 결과는 별도 상태로 유지합니다.

### 작업 모드 변경 시 Hold 중단

제작대는 현재 상태를 `GetInteractionModeCode`로 반환합니다.

| 코드 | 제작대 상태 |
|---|---|
| `1` | `bHasActiveWork` — 제작 중 |
| `2` | `CompletedToCollect > 0` — 수령 가능 |
| `3` | 그 외 — 레시피 선택/대기 |

Hold를 시작할 때 `HoldInitialModeCode`를 저장하고, 진행 중 코드가 달라지면 `StopHoldInteraction`으로 중단합니다.

~~~cpp
// InteractionComponent.cpp — Hold Timer 내부
const int32 CurrentMode =
    IInteractableInterface::Execute_GetInteractionModeCode(CurrentInteractable);

if (CurrentMode != HoldInitialModeCode)
{
    StopHoldInteraction(Purpose);
    return;
}
~~~

예를 들어 대기 상태에서 레시피 UI로 진입한 뒤 **이전 Hold가 변경된 행동으로 이어지는 상황을 막기 위한 처리**입니다. 이 검사는 **모드 코드 변화**를 기준으로 하며, 모든 오브젝트의 상태 변화나 대상 교체를 포괄하는 검증은 아닙니다.

---

## 서버 권한과 검증 경계

**탐지·안내·Hold UI는 로컬에서 처리하고, 아이템 지급·제작·던전 진행·퀘스트 기록은 서버의 게임플레이 함수에서 결정**합니다.

| 단계 | 현재 처리 | 주의할 점 |
|---|---|---|
| 로컬 탐지 | 카메라 Trace·`ImplementsInterface`·`CanInteract` | 클라이언트가 보여주는 UI 판단 |
| 상호작용 요청 | `Server_TryInteract(AActor*)` | 클라이언트가 전달한 Actor 참조를 받음 |
| 공통 서버 실행 | `TryInteract`의 `CanInteract` 호출 | 공통 경로에서 거리·인터페이스를 모두 다시 검사하지는 않음 |
| 대상별 실행 | Item/Quest/Dungeon/Crafting 등 개별 구현 | 각 Actor·Subsystem에서 수행하는 실제 검증 범위가 다름 |

구체적으로 `Server_TryInteract_Implementation`은 전달받은 Actor를 `CurrentInteractable`로 설정한 뒤 `TryInteract`를 호출합니다. 이 공통 RPC 경로에는 **서버 기준 재탐지·거리 검사나 인터페이스 구현 여부 재확인이 없습니다.** 따라서 모든 대상이 동일한 수준으로 요청을 검증한다고 설명할 수 없습니다.

반면 던전의 `TryStart / TryInteractSwitch`와 퀘스트 NPC의 `Interact_Implementation`에는 실제 거리·현재 상태 등의 검사가 존재합니다. **공통 서버 요청 경계에 Actor 유효성·인터페이스·거리·입력 목적 검사를 보강**하면 대상마다 검증이 누락될 가능성을 더 줄일 수 있습니다.

UI에서 보이는 Prompt·Hold는 입력 보조이며, **서버 승인이나 게임플레이 결과의 근거가 되지 않습니다.**

---

## 아이템 정의와 월드 생성 옵션

같은 아이템이라도 월드에 어떻게 등장했는지에 따라 사용 방식이 달라집니다.

| 정의 데이터 | 생성 시점 옵션 |
|---|---|
| `FItemData` (DataTable) | `FItemSpawnOptions` |
| 아이템 이름·종류·아이콘·스택·장비 설정 | 상호작용 필요 여부·즉시/홀드·자동 획득 지연·수량·물리·수명 |

~~~cpp
// BaseItem.h — 실제 주요 필드
struct FItemSpawnOptions
{
    bool bRequireInteraction = false;
    EItemInteractionType InteractionType = EItemInteractionType::Instant;
    float HoldDuration = 1.f;
    int32 ItemCount = 1;
    float AutoPickupDelay = 0.f;
    bool bApplyPhysicsOnDrop = false;
    float DropForce = 600.f;
    float LifeTime = 0.f;
};
~~~

`ABaseItem::InitializeItem`은 정의 데이터와 `FItemSpawnOptions`를 이용해 실제 월드 아이템을 초기화합니다. 상호작용이 필요 없는 드롭 아이템은 Overlap을 통한 자동 획득을 사용할 수 있고, 직접 획득해야 하는 아이템은 `Instant/Hold` 입력을 따릅니다.

`MakeDropped`, `MakeInteractable` 등의 Helper는 자주 사용하는 스폰 옵션을 생성합니다. **아이템의 정적 정의(`FItemData`)와 개별 생성 상황에 따른 설정(`FItemSpawnOptions`)을 분리**한 구성입니다.

---

## 기존 시스템과의 연동

### 명시적 상호작용 vs 공격 기반 채집

자원 채집은 `F` 키를 눌러 오브젝트를 사용하는 방식이 아니라 **실제 무기 공격이 자원 Actor에 적중하는 전투 경로**를 이용합니다.

~~~text
UMeleeAttack::ProcessHitDetection
    → FHitResult
    → AAreaObject::CalcDamage
    → Resource::TakeDamage
    → 자원 아이템 드롭
~~~

이처럼 플레이어가 월드에 영향을 주는 행동이라도 **명시적 사용 입력은 `IInteractableInterface`**, **물리적인 Hit 결과는 Combat/Damage Pipeline**으로 분리했습니다. 두 시스템의 목적이 달라 하나의 인터페이스에 모든 상호작용을 합치지 않았습니다.

### 던전 보상 상자는 상호작용 대상이 아님

`ADungeonRewardChest`는 **`IInteractableInterface`를 구현하지 않습니다.** 별도의 서버 아이템 지급이나 상호작용 요청도 수행하지 않습니다. 대신 `ASonheimGameState::DungeonStageState`의 `RunStatus == Succeeded`를 읽고, 클리어 시 뚜껑 개방·효과·사운드를 표시합니다.

~~~text
서버 던전 완료
    ↓ GameState Snapshot 복제
ADungeonRewardChest::RefreshFromState
    ↓
열림 애니메이션 · VFX/SFX
~~~

따라서 던전 확장의 공통 입력 재사용 사례는 **입구·지름길 레버**이고, 보상 상자는 **복제된 현재 상태를 구독해 월드 표현을 갱신하는 별도 사례**입니다.

---

## 선택 기준과 비용

| 선택 | 얻은 효과 | 비용·주의점 |
|---|---|---|
| **Interface 기반 대상 분리** | 플레이어 입력 코드에서 Actor별 분기를 줄임 | 인터페이스가 지나치게 비대해지지 않도록 범위 관리 |
| **대상별 Prompt·Hold 제공** | 신규 Actor가 이름·시간·취소 규칙을 구현 | 개별 Actor가 화면 안내 데이터 일부를 소유 |
| **로컬 Trace + 입력 처리** | 가까운 대상의 안내를 빠르게 갱신 | 서버 RPC에서 요청 Actor를 별도로 검증할 필요 |
| **ModeCode 기반 Hold 중단** | 제작대처럼 동작 의미가 변경되는 경우 연쇄 입력 감소 | Actor가 모드 코드 변경 기준을 정의해야 함 |
| **서버 처리와 로컬 UI 분리** | 게임플레이 결과는 서버가 결정 | 대상별 검사만으로는 공통 검증 누락 위험 |
| **Hit 기반 자원 채집 분리** | 실제 타격과 명시적 사용 입력을 각자 처리 | 월드 행동의 진입 경로가 둘로 나뉨 |

중요한 점은 **플레이어가 어떤 Actor인지 알 필요 없이 입력을 전달하지만, 실제 성공 조건은 콘텐츠별 서버 로직이 결정한다는 것**입니다. 공통 계약은 탐지·입력과 대상별 실행을 연결하는 역할을 맡습니다.

## 관련 코드 및 문서

- [InteractableInterface.h](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/GameObject/InteractableInterface.h) — 공통 계약
- [InteractionComponent.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/AreaObject/Player/Utility/InteractionComponent.cpp) — 로컬 탐지·Hold·Server RPC
- [BaseItem.h](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/GameObject/Items/BaseItem.h) · [BaseItem.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/GameObject/Items/BaseItem.cpp) — 정의/생성 옵션·아이템 획득
- [BaseContainer.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/GameObject/Buildings/Storage/BaseContainer.cpp) — 공유 보관함 사용자·UI
- [CraftingStation.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/GameObject/Buildings/Crafting/CraftingStation.cpp) — 작업·수령·취소 Hold
- [DungeonShortcutSwitch.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/GameObject/Dungeon/DungeonShortcutSwitch.cpp) · [DungeonTestArea.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/GameObject/Dungeon/DungeonTestArea.cpp) — 던전 입력과 Runtime 위임
- [DungeonStageRuntimeSubsystem.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/GameManager/Dungeon/DungeonStageRuntimeSubsystem.cpp) — 입장·레버 서버 검증
- [QuestGiver.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/GameObject/Quest/QuestGiver.cpp) · [QuestBuildSite.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/GameObject/Quest/QuestBuildSite.cpp) — 퀘스트 상호작용
- [DungeonRewardChest.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/GameObject/Dungeon/DungeonRewardChest.cpp) — 입력이 아닌 Snapshot 기반 월드 표현
- [[02. Gameplay Architecture|02_Gameplay_Architecture]] — Actor·Component·Interface의 책임
- [[05. Combat, Skill & Animation|05_Combat_Skill_Animation]] — 근접 판정·자원 타격
- [[07. Inventory & Crafting|07_Inventory_Crafting]] — 보관함·제작 서버 상태
- [[09. Branching Dungeon Runtime|09_Branching_Dungeon_Runtime]] — 지름길 분기·입장 조건
- [[12. Multiplayer Synchronization|12_Multiplayer_Synchronization]] — Server RPC·동기화 범위
- [[17. Quest System|17_Quest_System]] — NPC 대화·목표 보고·건설 연동
