# 06. World Interaction Systems

`UInteractionComponent`가 아이템·보관함·제작대·던전 레버를 동일한 Trace·입력·Hold 흐름으로 처리합니다. 대상 Actor는 `IInteractableInterface`에서 안내 문구, 사용 조건, 실제 실행 동작을 구현합니다.

## 게임 화면

https://github.com/user-attachments/assets/4ecbc1bc-8c72-4493-8a66-a7b2a0918a36

아이템·보관함·제작대·던전 입구·레버에서 **동일한 탐지·입력 UI를 사용하면서 대상마다 서로 다른 동작**이 실행됩니다.

## 핵심 설계

| 공통 처리 — `UInteractionComponent` | 대상별 구현 — `IInteractableInterface` |
|---|---|
| 카메라 Trace로 상호작용 Actor 탐색 | `CanInteract` — 현재 사용 가능 여부 |
| 현재 대상 변경과 입력 처리 | `OnDetected`·`GetInteractionName` — 대상별 안내 |
| 즉시 실행·Hold 진행·취소 입력 | `GetHoldDuration`·`CanHoldCancel`·`GetInteractionModeCode` |
| 서버에 실행 요청 전달 | `Interact` — 아이템 획득·UI 열기·제작·던전 이벤트 등 |

새 상호작용 Actor는 공통 인터페이스만 구현하면 기존 입력 경로를 사용합니다.

## 목차

- [인터페이스를 이용한 다형성](#인터페이스를-이용한-다형성)
- [공통 탐지·입력에서 대상별 실행까지](#공통-탐지입력에서-대상별-실행까지)
- [같은 Interact 호출, 다른 결과](#같은-interact-호출-다른-결과)
- [Hold와 취소를 공통화한 방식](#hold와-취소를-공통화한-방식)
- [콘텐츠 확장 사례](#콘텐츠-확장-사례)
- [설계 선택과 비용](#설계-선택과-비용)
- [관련 코드 및 문서](#관련-코드-및-문서)

---

## 인터페이스를 이용한 다형성

### 공통 계약

`IInteractableInterface`는 `UINTERFACE`와 `BlueprintNativeEvent`로 정의합니다. 각 Actor는 자신에게 필요한 `_Implementation`을 구현하며, 호출자는 **구체 클래스가 아닌 인터페이스**를 대상으로 실행합니다.

~~~cpp
// InteractableInterface.h — 주요 함수
UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
bool CanInteract() const;

UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
void OnDetected(bool bDetected);

UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
void Interact(ASonheimPlayer* Player);

UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
FString GetInteractionName() const;

UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
float GetHoldDuration() const;
~~~

인터페이스에는 Hold 게이지 표시, 취소 가능 여부, 취소 실행, 현재 상호작용 모드를 조회하는 함수도 있습니다. **플레이어 컴포넌트가 호출 순서를 정하고, 각 대상이 반환값과 실제 동작을 결정**하는 계약입니다.

### 대상 클래스와 무관한 호출

~~~cpp
// InteractionComponent.cpp — 실제 공통 호출
if (!CurrentInteractable ||
    !IInteractableInterface::Execute_CanInteract(CurrentInteractable))
{
    return;
}

IInteractableInterface::Execute_Interact(
    CurrentInteractable, OwnerPlayer);
~~~

`CurrentInteractable`은 `AActor*`입니다. `ABaseItem`, `ABaseContainer`, `ACraftingStation`처럼 서로 다른 클래스라도 `IInteractableInterface`를 구현하면 `Execute_Interact`의 인자로 전달할 수 있습니다.

~~~mermaid
flowchart TB
    INPUT["UInteractionComponent<br/>현재 대상: AActor*"] --> CALL["IInteractableInterface::Execute_Interact"]
    CALL --> ITEM["ABaseItem<br/>아이템 획득"]
    CALL --> CONTAINER["ABaseContainer<br/>보관함 열기"]
    CALL --> CRAFT["ACraftingStation<br/>레시피·제작·수령"]
    CALL --> LEVER["ADungeonShortcutSwitch<br/>던전 이벤트"]
    CALL --> NPC["AQuestGiver<br/>퀘스트 대화"]
~~~

`Execute_Interact`는 Unreal 인터페이스 이벤트로 대상 Actor의 C++ `Interact_Implementation` 또는 Blueprint 구현을 호출합니다. 플레이어 코드에는 대상 클래스별 `Cast` 분기가 없습니다.

---

## 공통 탐지·입력에서 대상별 실행까지

### 대상 탐색과 UI 연결

`UInteractionComponent::PerformDetection`은 로컬 플레이어에서 0.1초 간격으로 실행됩니다. 카메라 전방 **500 Unreal Unit**의 `ECC_Visibility` Multi Trace로 감지한 Actor에서 다음 조건을 확인합니다.

1. `ImplementsInterface(UInteractableInterface::StaticClass())`
2. `IInteractableInterface::Execute_CanInteract(Actor)`
3. 후보 중 카메라에서 가장 가까운 Actor 선택

선택이 바뀌면 `UpdateInteractable`가 이전 대상에 `OnDetected(false)`, 새 대상에 `OnDetected(true)`를 호출하고 `OnInteractableChanged`를 전달합니다.

~~~text
카메라 Trace
  → 인터페이스 구현 여부 · CanInteract 검사
  → CurrentInteractable 선택
  → OnDetected(true) · 대상 안내 표시
  → 사용자 입력 / Hold
  → TryInteract
  → Server_TryInteract (클라이언트 요청)
  → Execute_Interact
  → 해당 Actor의 Interact_Implementation
~~~

**안내 UI도 대상별 구현을 재사용**합니다. 예를 들어 `ABaseItem::OnDetected_Implementation`은 감지 위젯과 아이템 하이라이트를, `AQuestGiver::OnDetected_Implementation`은 현재 퀘스트 상태에 맞는 대화·보고 안내를 갱신합니다.

### 서버 실행 경로

비권한 클라이언트에서 `TryInteract`를 호출하면 `Server_TryInteract(CurrentInteractable)`로 대상 Actor를 전달합니다. 서버는 `CanInteract`를 검사하고 `Execute_Interact`를 호출합니다. 이후 실제 아이템 지급·던전 입장·퀘스트 진행 같은 상태 변경은 대상별 게임플레이 코드가 처리합니다.

공통 컴포넌트는 **어떤 Actor를 실행할지**를 연결하고, 각 Actor 또는 위임받은 Subsystem은 **그 요청을 어떻게 처리할지**를 구현합니다.

---

## 같은 Interact 호출, 다른 결과

### 아이템 — 서버에서 보상 지급

~~~cpp
// BaseItem.cpp
void ABaseItem::Interact_Implementation(ASonheimPlayer* Player)
{
    if (CanInteract_Implementation() && CanBeCollectedBy(Player))
    {
        OnCollected(Player);
        Multicast_OnCollected();
    }
}
~~~

`OnCollected`는 아이템 보상을 적용한 뒤 월드 Actor를 제거합니다. 공통 `Interact` 호출을 **아이템 획득 동작**으로 구현한 사례입니다.

### 보관함 — 사용자를 정하고 UI 열기

~~~cpp
// BaseContainer.cpp
void ABaseContainer::Interact_Implementation(ASonheimPlayer* Player)
{
    if (!CanInteract_Implementation() || !Player)
        return;

    OpenContainer(Player);
}
~~~

`OpenContainer`는 서버에서 현재 사용자와 열림 상태를 관리하고 해당 플레이어에게 보관함 UI 열기를 요청합니다. 같은 `Interact` 호출을 **공유 보관함 화면 열기**로 구현했습니다.

### 제작대 — 같은 Actor에서도 상태별 동작 분기

`ACraftingStation::Interact_Implementation`은 실행 시점의 제작 상태에 따라 동작을 선택합니다.

| 현재 상태 | 호출 결과 |
|---|---|
| 진행 중인 작업이 없음·수령 결과 없음 | `ServerRequestOpenUI` — 레시피 선택 |
| 제작 진행 중 | `ServerAddWork` — 작업량 추가 |
| 수령 가능한 결과 존재 | 조건 충족 시 `ServerCollectAll` — 결과 수령 |

진행 중에는 `ManualCollectDelay`를 확인해 작업 직후 자동으로 수령되는 상황을 제한합니다. 즉, **다형성으로 대상 Actor를 선택한 뒤, 해당 Actor 내부에서 다시 상태에 따라 행동을 결정**합니다.

레시피 선택부터 여러 플레이어의 작업량 누적·완성품 수령까지는 [기존 협동 제작 플레이 영상](https://github.com/user-attachments/assets/df9a30d1-8d52-4b86-9f18-790474fdbda2)에서 볼 수 있습니다. 제작 재료·취소·수령의 상세 규칙은 [07. Inventory & Crafting](07_Inventory_Crafting.md)에서 다룹니다.

### 던전 레버 — 실행을 Runtime에 위임

~~~cpp
// DungeonShortcutSwitch.cpp
void ADungeonShortcutSwitch::Interact_Implementation(
    ASonheimPlayer* Player)
{
    if (HasAuthority())
        GetWorld()->GetSubsystem<UDungeonStageRuntimeSubsystem>()
            ->TryInteractSwitch(this, Player, TestArea, SourceId);
}
~~~

레버는 `Interact`를 구현하지만 스테이지 전환 조건을 직접 해석하지 않습니다. `SourceId`와 요청을 `UDungeonStageRuntimeSubsystem`에 전달하고, 실제 분기는 던전 정의의 EventRule을 따릅니다.

**같은 인터페이스, 다른 내부 책임**을 유지하면서 콘텐츠를 확장한 사례입니다.

---

## Hold와 취소를 공통화한 방식

### 대상이 시간을 결정하고 컴포넌트가 진행

`UInteractionComponent::StartHoldInteraction`은 `GetHoldDuration`을 호출해 실행 방식을 결정합니다.

~~~text
GetHoldDuration() <= 0  →  TryInteract 즉시 호출
GetHoldDuration() > 0   →  Hold Timer 시작
                           ↓
                         UpdateHoldProgressUI
                           ↓
                         완료 시 TryInteract
~~~

각 Actor는 자신의 Hold 시간과 진행 위젯 갱신 방법을 구현합니다. `ABaseItem`은 아이템 생성 옵션의 `Instant / Hold`, `ABaseContainer`는 Container 정의의 Hold 설정, `ACraftingStation`은 제작 작업의 Hold 시간을 사용합니다.

플레이어는 대상 종류와 관계없이 **하나의 Hold Timer와 입력 흐름**을 유지합니다.

### 취소 입력도 같은 계약

`CanHoldCancel`이 참이면 `GetCancelHoldDuration`을 조회하고, 별도 취소 Hold가 완료되면 `ExecuteCancel`을 호출합니다.

제작대는 이를 통해 진행 중인 미완료 작업 취소를 처리합니다. 취소 여부와 실제 처리 방식은 제작대에 있고, 입력 진행 관리는 공통 컴포넌트에 있습니다.

### Hold 중 동작이 바뀌는 경우

제작대의 행동은 `레시피 선택 → 제작 → 수령`처럼 변합니다. `GetInteractionModeCode`는 해당 시점의 동작 모드를 반환합니다.

~~~cpp
// CraftingStation.cpp
int32 ACraftingStation::GetInteractionModeCode_Implementation() const
{
    if (bHasActiveWork) return 1;
    if (CompletedToCollect > 0) return 2;
    return 3;
}
~~~

`UInteractionComponent`는 Hold 시작 시 이 값을 저장해 두고, 진행 중 값이 바뀌면 Hold를 종료합니다.

~~~cpp
if (CurrentMode != HoldInitialModeCode)
{
    StopHoldInteraction(Purpose);
    return;
}
~~~

Hold 도중 제작이 완료되어 입력 의미가 '제작'에서 '수령'으로 바뀌면 기존 Hold를 종료합니다. 같은 ModeCode인 동안에는 작업량을 계속 가산할 수 있습니다.

---

## 콘텐츠 확장 사례

| 추가한 Actor | 구현한 `Interact`의 역할 |
|---|---|
| `ADungeonTestArea` | `UDungeonStageRuntimeSubsystem::TryStart`로 던전 입장 요청 |
| `ADungeonShortcutSwitch` | `TryInteractSwitch`로 지름길 이벤트 전달 |
| `AQuestGiver` | `UQuestComponent::Interact` 처리 후 퀘스트 대화창 열기 |
| `AQuestBuildSite` | `UQuestComponent::Interact`로 자재 전달·목표 진행 |

공통 인터페이스는 입력 진입점만 제공합니다. 던전 스테이지 전환과 퀘스트 목표 집계는 각 Runtime·Component가 수행합니다.

---

## 설계 선택과 비용

| 선택 | 이점 | 관리할 부분 |
|---|---|---|
| **`IInteractableInterface` 다형성** | 대상이 늘어나도 플레이어의 타입별 분기 불필요 | 각 Actor의 인터페이스 구현·사용 조건 관리 |
| **`BlueprintNativeEvent`** | C++과 Blueprint 대상에서 공통 호출 형태 유지 | `Execute_` 경로 및 `_Implementation` 연결 |
| **공통 탐지·Hold 컴포넌트** | 대상마다 카메라 Trace·입력 Timer를 재구현하지 않음 | 대상 교체·Hold 중단·입력 상태 처리 |
| **대상별 상태 처리** | 하나의 `Interact`로 서로 다른 결과 실행 | Actor별 서버 상태 검사와 요청 처리 |
| **ModeCode** | Hold 중 동작 의미 변경 시 연쇄 입력 차단 | 상태가 바뀔 때 ModeCode도 일관되게 변경 |

## 관련 코드 및 문서

- [InteractableInterface.h](../../Sonheim/Source/Sonheim/GameObject/InteractableInterface.h) — `BlueprintNativeEvent` 공통 계약
- [InteractionComponent.cpp](../../Sonheim/Source/Sonheim/AreaObject/Player/Utility/InteractionComponent.cpp) — 카메라 탐지·공통 입력·Hold·`Execute_Interact`
- [BaseItem.cpp](../../Sonheim/Source/Sonheim/GameObject/Items/BaseItem.cpp) — 아이템 획득 구현
- [BaseContainer.cpp](../../Sonheim/Source/Sonheim/GameObject/Buildings/Storage/BaseContainer.cpp) — 보관함 열기 구현
- [CraftingStation.cpp](../../Sonheim/Source/Sonheim/GameObject/Buildings/Crafting/CraftingStation.cpp) — 제작 상태별 `Interact` 및 Hold 모드
- [DungeonShortcutSwitch.cpp](../../Sonheim/Source/Sonheim/GameObject/Dungeon/DungeonShortcutSwitch.cpp) · [DungeonTestArea.cpp](../../Sonheim/Source/Sonheim/GameObject/Dungeon/DungeonTestArea.cpp) — 던전 입력 확장
- [QuestGiver.cpp](../../Sonheim/Source/Sonheim/GameObject/Quest/QuestGiver.cpp) · [QuestBuildSite.cpp](../../Sonheim/Source/Sonheim/GameObject/Quest/QuestBuildSite.cpp) — 퀘스트 입력 확장
- [02. Gameplay Architecture](02_Gameplay_Architecture.md) — Actor·Component·Interface 책임
- [07. Inventory & Crafting](07_Inventory_Crafting.md) — 제작의 세부 실행 규칙
- [09. Branching Dungeon Runtime](09_Branching_Dungeon_Runtime.md) — 레버와 스테이지 분기
- [17. Quest System](17_Quest_System.md) — NPC 대화와 목표 진행
