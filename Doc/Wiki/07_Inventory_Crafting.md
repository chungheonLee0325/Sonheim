# 07. Inventory & Crafting

> **핵심 구현 범위**
>
> 같은 Item 데이터를 사용하는데 왜 Player Inventory, World Container, Crafting Station은 서로 다른 네트워크 구조를 가져야 하는가?

먼저 세 상태의 차이를 보면 전체 문서를 이해하기 쉽습니다.

| 시스템 | 소유 범위 | 누가 보는가 | 핵심 네트워크 정책 |
|---|---|---|---|
| Player Inventory | 개인 | Owner | Owner-only FastArray |
| Container | World shared | 열어본 Player | Subscriber-based FastArray |
| Crafting Station | World shared workflow | 주변/참여 Player | Replicated work state + Server lock |

---

## 시연 영상

기존 Inventory / Chest 시연 영상입니다. Drag & Drop과 Container 상호작용이 실제 UI에서 어떻게 연결되는지 확인할 수 있습니다.

https://github.com/user-attachments/assets/c594e8a3-2840-456c-ae04-cabaaeb4d8ca

---

## Part 1. Player Inventory

### 1.1 실제 Replication 구조

```cpp
USTRUCT(BlueprintType)
struct FRepInventoryEntry : public FFastArraySerializerItem
{
    GENERATED_BODY()

    int32 SlotIndex = 0;
    int32 ItemID = 0;
    int32 Count = 0;
};

USTRUCT(BlueprintType)
struct FRepInventoryList : public FFastArraySerializer
{
    GENERATED_BODY()

    TArray<FRepInventoryEntry> Items;

    bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
    {
        return FFastArraySerializer::FastArrayDeltaSerialize<
            FRepInventoryEntry,
            FRepInventoryList>(Items, DeltaParms, *this);
    }
};
```

실제 Component는 network list와 local mirror를 나눕니다.

```cpp
UPROPERTY(ReplicatedUsing=OnRep_RepItems)
FRepInventoryList RepItems;

UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
TArray<FInventoryItem> InventoryItems;
```

---

### 1.2 왜 두 배열인가

Server에서는 `InventoryItems`를 gameplay API가 사용하고, 변경 slot만 FastArray entry에 반영합니다.

Client는 `OnRep_RepItems`에서 local mirror를 다시 구성합니다.

```text
Server InventoryItems
       ↓ Dirty Entry
RepItems (FastArray)
       ↓ Owner-only replication
Client OnRep_RepItems
       ↓
Client InventoryItems mirror
       ↓
UI Delegate
```

---

### 1.3 Owner-only

다른 Player의 개인 Inventory 전체를 받을 필요가 없습니다.

따라서 `RepItems`는 owner-only 조건으로 복제합니다.

“FastArray를 사용했다”보다 중요한 것은 **데이터를 누가 소비하는가에 맞춰 replication scope를 제한했다는 점**입니다.

---

## Part 2. Client Prediction

### 2.1 Slot Swap

```cpp
bool UInventoryComponent::SwapItems(int32 FromIndex, int32 ToIndex)
{
    if (GetOwnerRole() == ROLE_Authority)
    {
        InventoryItems.Swap(FromIndex, ToIndex);
        UpdateRepEntryAtIndex(FromIndex);
        UpdateRepEntryAtIndex(ToIndex);
        BroadcastInventoryChanged();
        return true;
    }

    if (bEnableClientPrediction)
        PerformClientPrediction_SwapItems(FromIndex, ToIndex);

    ServerSwapItems(FromIndex, ToIndex);
    return true;
}
```

흐름:

```text
Drag
 ↓
Client Prediction
 ↓
Server RPC
 ↓
Server authoritative mutation
 ↓
FastArray replication
 ↓
OnRep reconciliation
```

Prediction은 authority를 대신하지 않고 round-trip latency를 가리는 역할만 합니다.

---

## Part 3. “상태 변경”과 “획득 이벤트”를 분리한다

Inventory slot이 바뀌었다고 항상 “아이템 획득” popup을 띄우면 안 됩니다.

예:

- Field pickup → 획득 알림 O
- Equip 해제 후 Inventory 복귀 → 획득 알림 X
- Slot swap → 획득 알림 X

그래서:

- `OnInventoryChanged`: 구조 변경
- `OnItemAdded`: 직접 획득 의미가 있을 때

를 분리합니다.

`AddItem(..., bool IsDirectAcquisition)`이 이 semantic 차이를 전달합니다.

---

## Part 4. Equipment는 다른 시스템의 Source다

장비 변경은 Inventory 내부만의 일이 아닙니다.

```text
Equip Item
  ├─ EquippedSlots
  ├─ StatBonus
  ├─ Weapon Mesh
  ├─ HUD
  └─ Skill Grant
```

Inventory Component는 현재 무기가 부여한 Skill source를 `ActiveWeaponGrantId`로 추적합니다.

무기가 바뀌면 Skill Component의 `ReplaceGrant`와 연결합니다.

---

## Part 5. Shared Container

### 5.1 구조는 비슷하지만 전송 조건이 다르다

Container도 FastArray입니다.

```cpp
USTRUCT(BlueprintType)
struct FRepContainerEntry : public FFastArraySerializerItem
{
    GENERATED_BODY()

    int32 SlotIndex = 0;
    int32 ItemID = 0;
    int32 Count = 0;
};
```

하지만 owner-only가 아닙니다.

Container는 여러 Player가 볼 수 있는 World state입니다.

---

### 5.2 Subscriber가 있을 때만 내부 Item을 복제한다

```cpp
void UContainerComponent::PreReplication(
    IRepChangedPropertyTracker& ChangedPropertyTracker)
{
    Super::PreReplication(ChangedPropertyTracker);

    const bool bActive = Subscribers.Num() > 0;

    DOREPLIFETIME_ACTIVE_OVERRIDE(
        UContainerComponent,
        RepContainerItems,
        bActive);
}
```

Player가 UI를 열면:

```cpp
ServerSubscribeViewer(APlayerController* Viewer);
```

닫으면:

```cpp
ServerUnsubscribeViewer(APlayerController* Viewer);
```

를 호출합니다.

즉 Container Actor 자체가 존재한다고 내부 Inventory까지 항상 전송하지 않습니다.

---

## Part 6. Crafting Station

### 6.1 한 개의 Interaction이 상태에 따라 다른 의미를 갖는다

```text
F Interaction
   ↓
완료 결과 존재? → Collect
   ↓ no
작업 진행 중?   → Add Work
   ↓ no
Idle             → Recipe UI Open
```

Crafting Station은 현재 authoritative state를 보고 Input 의미를 결정합니다.

---

### 6.2 왜 `UIOwner`가 필요한가

Recipe를 선택하는 순간 두 Player가 서로 다른 작업을 동시에 시작하면 `ActiveWork`와 재료 소모가 충돌할 수 있습니다.

```cpp
if (UIOwner && UIOwner != Player)
    return;

UIOwner = Player;
```

이 lock은 Station 전체를 잠그지 않습니다.

다른 Player는 작업이 시작된 후:

- Help
- Collect

를 계속할 수 있습니다.

즉 동시성 충돌이 생기는 **Recipe 선택/시작 구간만 exclusive**하게 만듭니다.

---

## Part 7. Server-authoritative Crafting Lifecycle

```text
Recipe Select
 ↓
ServerStartWork
 ↓
Server 재료 검증
 ↓
재료 소모
 ↓
ActiveWork 생성
 ↓
ServerAddWork
 ↓
Unit 완료
 ↓
CompletedToCollect
 ↓
ServerCollectAll
 ↓
Inventory 지급
```

Client UI의 “제작 가능” 표시를 신뢰하지 않고 Server가 다시 계산합니다.

---

### 7.1 Cancel

진행 중 작업을 취소하면:

- 이미 완료된 Unit은 유지
- 미완료 Unit 재료만 환불

합니다.

“작업 전체 rollback”과 “완료 결과 보존”을 구분합니다.

---

## Part 8. Crafting UI는 어떤 데이터를 소유하지 않는다

`UCraftingWidget`은 authoritative crafting state를 만들지 않습니다.

역할은:

- Recipe 표시
- 현재 Inventory 기준 craftable 계산
- Quantity UI
- Server request

입니다.

Recipe가 바뀔 때는 구조적 UI를 갱신하고, Inventory/Quantity가 바뀔 때는 숫자와 enable 상태만 갱신합니다.

Required Material Row는 local pool을 사용합니다.

---

## Trade-offs

### Client Prediction
응답성은 좋아지지만 Server mutation과 prediction rule이 어긋나지 않도록 유지해야 합니다.

### Container Subscription
현재는 “viewer가 하나라도 있는가”를 기준으로 property replication을 활성화합니다. Viewer별 세밀한 per-connection 필터링이 필요한 규모에서는 더 고급 replication graph/policy가 필요할 수 있습니다.

### Crafting UIOwner
간단하고 명확하지만 여러 Player가 각자 독립 Recipe queue를 가질 수 있는 Station 모델에는 적합하지 않습니다.

---

## 연관 문서

- 기본 Server Authority 원칙 → [[2. Gameplay Architecture|02_Gameplay_Architecture]]
- Inventory가 Skill/Stat과 만나는 지점 → [[5. Combat, Skill & Animation|05_Combat_Skill_Animation]], [[4. Player & Character Systems|04_Player_Character_Systems]]
- Dungeon Reward가 Inventory에 들어가는 흐름 → [[9. Branching Dungeon Runtime|09_Branching_Dungeon_Runtime]]

---

## 관련 코드

- [InventoryComponent.h](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Player/Utility/InventoryComponent.h)
- [ContainerComponent.h](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/GameObject/Buildings/Utility/ContainerComponent.h)
- [CraftingStation](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/GameObject/Buildings/Crafting/CraftingStation.h)
- [CraftingWidget](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/UI/Widget/GameObject/Crafting/CraftingWidget.cpp)
