# 07. Inventory & Crafting

Item은 Player Inventory를 중심으로 **Equipment, Shared Container, Crafting, Reward**에 연결됩니다.

## 요약

- **Player Inventory**는 Slot 기반 Item state를 소유하고 변경된 entry를 FastArray로 동기화합니다.
- **Equipment**는 Item의 `SkillID`와 Stat source를 통해 현재 공격 Skill, 능력치, Weapon Mesh/HUD를 갱신합니다.
- **Container**는 같은 Item/Slot 모델을 사용하면서 World에 존재하는 shared ownership과 viewer lifecycle을 가집니다.
- **Crafting**은 Recipe Definition과 `FActiveCraftWork`를 분리하고, Server가 재료 소비·공동 작업·완료 결과를 관리합니다.
- **UI**는 runtime state와 Item/Recipe definition을 조합하고 `USlotWidget`을 Inventory·Equipment·Container·Crafting에서 재사용합니다.

## Runtime Demo

### Inventory / Container

Drag & Drop과 Container 간 Item 이동이 같은 Slot 기반 UI 흐름에서 동작합니다.

https://github.com/user-attachments/assets/c594e8a3-2840-456c-ae04-cabaaeb4d8ca

### Collaborative Crafting

https://github.com/user-attachments/assets/df9a30d1-8d52-4b86-9f18-790474fdbda2

*Recipe 선택과 재료 확인부터 Shared Work 진행, 완료 결과 수령, Inventory 반영까지 이어지는 제작 흐름.*

## 목차

- [Runtime Demo](#runtime-demo)
- [System Overview](#system-overview)
- [Inventory State Model](#inventory-state-model)
- [Equipment Pipeline](#equipment-pipeline)
- [Shared Container](#shared-container)
- [Crafting Runtime](#crafting-runtime)
- [UI & Presentation](#ui--presentation)
- [Integration](#integration)
- [Trade-offs](#trade-offs)

---

## System Overview

~~~mermaid
flowchart LR
    ITEM["<b>Item Definition</b><br/>FItemData"]
    INV["<b>Player Inventory</b><br/>UInventoryComponent"]
    EQUIP["<b>Equipment State</b><br/>EquippedSlots"]
    STAT["<b>Stat 적용</b><br/>UStatBonusComponent"]
    SKILL["<b>Skill 교체</b><br/>ReplaceGrant"]

    BOX["<b>Shared Container</b><br/>UContainerComponent"]
    CRAFT["<b>Collaborative Crafting</b><br/>ACraftingStation"]
    RESULT["<b>완료 Item</b><br/>CompletedToCollect"]

    ITEM --> INV
    BOX <--> INV
    INV --> EQUIP
    EQUIP --> STAT
    EQUIP --> SKILL

    INV -->|"Material"| CRAFT
    CRAFT -->|"Completed"| RESULT
    RESULT --> INV
~~~

Player Inventory는 Loot, Equipment, Container, Crafting, Dungeon Reward가 공유하는 Item state의 중심 경계입니다.

---

## Inventory State Model

### Slot / Replication State

`UInventoryComponent`는 gameplay에서 사용하는 local array와 replication용 FastArray를 구분합니다.

~~~cpp
struct FRepInventoryEntry : public FFastArraySerializerItem
{
    int32 SlotIndex = 0;
    int32 ItemID = 0;
    int32 Count = 0;
};

struct FRepInventoryList : public FFastArraySerializer
{
    TArray<FRepInventoryEntry> Items;
};
~~~

실제 gameplay API는 `InventoryItems`를 사용하고, 변경된 slot만 `RepItems`에 반영합니다.

~~~text
Gameplay Inventory
      ↓
Changed Slot
      ↓
Replicated Entry
      ↓
Client Mirror
      ↓
UI Delegate
~~~

네트워크 동기화 방식은 [[12. Multiplayer Synchronization|12_Multiplayer_Synchronization]]에서 다룹니다.

### Inventory Change vs Item Acquisition

Inventory state가 바뀌는 경우와 Player가 새 Item을 획득한 경우를 서로 다른 event로 구분합니다.

| 상황 | Inventory Changed | Item Acquired |
|---|---:|---:|
| Field Item 획득 | O | O |
| 장비 해제 후 Inventory 복귀 | O | X |
| Slot Swap | O | X |
| Container에서 이동 | O | 상황에 따라 구분 |

- `OnInventoryChanged` — 구조적 상태 변화
- `OnItemAdded` — Player가 새 Item을 직접 획득한 의미

`AddItem(..., bool IsDirectAcquisition)`이 이 의미 차이를 전달합니다.

이를 통해 Slot Swap이나 장비 해제가 획득 Popup으로 표시되는 것을 막습니다.

### Prediction / Reconciliation

Slot Swap은 입력 직후 local mirror를 먼저 갱신하고 Server 결과로 보정합니다.

~~~text
Drag A → B
   ↓
Local Swap
   ↓
Server Request
   ↓
Authoritative Swap
   ↓
Replication
   ↓
Client Reconciliation
~~~

Prediction은 round-trip 동안의 UI 반응을 앞당기고, 최종 Item state는 Server replication으로 확정합니다.

Owner-only FastArray와 Prediction의 세부 동기화는 [[12. Multiplayer Synchronization|12_Multiplayer_Synchronization]]에서 설명합니다.

---

## Equipment Pipeline

Equipment change는 Inventory Slot에서 끝나지 않고 Stat / Skill / Weapon presentation으로 이어집니다.

~~~text
Equip Item
   ↓
Equipped Slot
   ├─ Stat Modifier 적용
   ├─ Weapon Mesh / HUD 갱신
   └─ Skill Grant 교체
~~~

무기 데이터에는 `WeaponType`과 `SkillID`가 함께 들어 있습니다.

~~~cpp
struct FEquipmentData
{
    EEquipmentKindType EquipKind;
    EWeaponType WeaponType;
    int SkillID = 0;

    bool bUseBullet = false;
    TSet<int> BulletItemID;

    USkeletalMesh* EquipmentMesh;
    TSoftObjectPtr<UAnimBlueprint> EquipmentAnim;
};
~~~

현재 Weapon Slot이 바뀌면 Inventory Component가 Item의 `SkillID`를 읽고 Skill Component의 `ReplaceGrant()`로 해당 장비가 제공하는 공격 Skill을 교체합니다.

예를 들면:

- **곡괭이** — 근접/채굴 공격 Skill과 Weapon Mesh로 Resource를 공격
- **샷건** — Shotgun 계열 Skill로 전환되고 WeaponType에 맞는 Crosshair와 탄약 정보를 사용

~~~text
Weapon Item
   ↓ FEquipmentData.SkillID
Active Weapon Slot
   ↓
ReplaceGrant(ActiveWeaponGrantId, SkillID)
   ↓
Player Skill Set 변경
   ↓
Input은 같은 Cast 경로 사용
~~~

`ActiveWeaponGrantId`는 현재 Weapon이 부여한 Skill source를 추적하므로 다른 source가 제공한 Skill과 분리해 교체할 수 있습니다.

실제 Weapon/Skill 전환은 [[04. Player & Character Systems|04_Player_Character_Systems]]의 Runtime 영상에서 확인할 수 있습니다.

---

## Shared Container

Player Inventory는 한 Player의 상태이고 Container는 World에 놓인 공유 보관 상태입니다.

Container도 Slot 기반 FastArray를 사용하며, viewer가 열려 있는 동안 내부 Item property replication을 활성화합니다.

~~~text
Open Container
   ↓
Subscribe Viewer
   ↓
Container Item State 활성
   ↓
Inventory / Container UI

Close
   ↓
Unsubscribe Viewer
~~~

같은 Item/Slot 모델을 재사용하면서 Player-owned state와 World shared state의 lifecycle을 분리합니다.

---

## Crafting Runtime

### Recipe Definition & Shared Work State

Crafting Station은 **무엇을 만드는지**와 **현재 제작이 어디까지 진행됐는지**를 분리합니다.

Recipe는 DataTable row로 정의합니다.

~~~cpp
struct FCraftingRecipe : public FTableRowBase
{
    FText DisplayName;

    int32 ResultItemID = 0;
    int32 ResultCount = 1;

    int32 WorkRequired = 100;
    TMap<int32, int32> RequiredMaterials;
};
~~~

실행 중인 제작은 별도의 `FActiveCraftWork`가 소유합니다.

~~~cpp
struct FActiveCraftWork
{
    FName RecipeRow;

    int32 ResultItemID = 0;
    int32 UnitsTotal = 0;
    int32 UnitsDone = 0;

    int32 ResultPerUnit = 1;
    int32 WorkPerUnit = 100;
    float WorkAccumulated = 0.f;
};
~~~

Recipe는 결과 Item·필요 Material·Unit당 Work를 정의하고, ActiveWork는 Units와 현재 누적 Work를 저장합니다.

### Interaction State

Crafting Station은 `IInteractableInterface`를 구현하고 현재 Work state에 따라 같은 Interaction 입력을 다른 동작으로 연결합니다.

~~~text
Interact
   ↓
작업 진행 중?
   ├─ Yes → Work 추가 / 완료분 수령
   └─ No
        ↓
수령 대기 결과 있음?
   ├─ Yes → Collect
   └─ No → Recipe UI Open
~~~

World interaction contract 자체는 [[06. World Interaction Systems|06_World_Interaction_Systems]]에서 설명합니다.

### UI Ownership & Collaborative Work

Recipe 선택과 Work 시작 전에는 `UIOwner`가 Station의 recipe selection을 소유합니다.

~~~cpp
if (UIOwner && UIOwner != Player)
    return;

UIOwner = Player;
~~~

ActiveWork가 시작된 뒤에는 다른 Player도:

- Work 추가
- Assist
- 완료 결과 Collect

에 참여할 수 있습니다.

즉 exclusive ownership은 **Recipe 선택 / Work 시작 경계**에 적용되고, 실제 Work state는 여러 Player가 공유합니다.

### Server Crafting Lifecycle

~~~mermaid
flowchart LR
    SELECT["<b>Recipe 선택</b><br/>Crafting UI"]
    VALIDATE["<b>Server 검증</b><br/>Material · Units"]
    CONSUME["<b>재료 소비</b><br/>InventoryResourceProvider"]
    WORK["<b>공유 작업 상태</b><br/>FActiveCraftWork"]
    ADD["<b>협력 작업</b><br/>Player Work 누적"]
    DONE["<b>Unit 완료</b><br/>CompletedToCollect"]
    COLLECT["<b>결과 수령</b><br/>ServerCollectAll"]
    INV["<b>Inventory 반영</b><br/>AddItem"]

    SELECT --> VALIDATE
    VALIDATE --> CONSUME
    CONSUME --> WORK
    WORK --> ADD
    ADD --> DONE
    DONE -->|"남은 Unit"| WORK
    DONE --> COLLECT
    COLLECT --> INV
~~~

실행 순서는 다음과 같습니다.

1. Client가 Recipe와 제작 Unit 수를 선택
2. `ServerStartWork()`가 현재 Inventory 기준으로 Material / Units 재검증
3. `UInventoryResourceProvider`가 필요한 Material 소비
4. `FActiveCraftWork` 생성
5. Player들의 Work를 누적
6. Unit 완료 시 `UnitsDone / CompletedToCollect` 갱신
7. `ServerCollectAll()`에서 결과 Item을 Inventory에 지급

Client UI의 “제작 가능 수량”은 예상값으로 사용하고, 실제 제작 시작 시 Server가 현재 상태에서 다시 계산합니다.

### Resource Consumption

Crafting UI와 Server는 `UInventoryResourceProvider`를 통해 같은 material 계산 규칙을 사용합니다.

~~~cpp
static int32 ComputeMaxCraftable(
    UInventoryComponent* Inv,
    const TMap<int32, int32>& Required);

static bool ConsumeItems(
    UInventoryComponent* Inv,
    const TMap<int32, int32>& Required);
~~~

UI는 현재 Inventory로 예상 제작 수량을 표시하고 Server는 같은 provider를 사용해 실제 소비를 수행합니다.

### Complete / Cancel / Collect

한 Unit이 완료될 때마다:

- `UnitsDone` 증가
- `CompletedToCollect` 증가

합니다.

완료된 결과는 Work가 계속 진행 중이어도 수령할 수 있습니다.

~~~text
UnitsTotal = 5
UnitsDone  = 2

Cancel
  ↓
완료 2개 → 유지 / 수령 가능
미완료 3개 → 재료 환불
~~~

Cancel은 미완료 Unit의 남은 Work와 Material을 정리하고 이미 완료된 결과는 유지합니다.

---

## UI & Presentation

### Inventory / Container UI

Inventory/Crafting UI의 원본 Item state는 Widget이 아니라 `UInventoryComponent`가 소유합니다.

~~~mermaid
flowchart LR
    SERVER["<b>Authoritative Inventory</b><br/>UInventoryComponent"]
    REP["<b>Client Mirror</b><br/>InventoryItems"]
    EVENT["<b>변경 알림</b><br/>OnInventoryChanged<br/>OnEquipmentChanged"]
    SCREEN["<b>Screen Controller</b><br/>ASonheimPlayerController"]
    INVUI["<b>Inventory View</b><br/>UInventoryWidget"]
    SLOT["<b>Reusable Item View</b><br/>USlotWidget"]
    DATA["<b>Static Item Data</b><br/>FItemData / GameInstance"]

    SERVER --> REP
    REP --> EVENT
    EVENT --> INVUI
    SCREEN --> INVUI
    DATA --> INVUI
    INVUI --> SLOT
~~~

PlayerController가 Inventory Screen을 열 때 `UInventoryWidget`에 `UInventoryComponent`를 연결합니다.

Widget은:

1. `InventoryItems`에서 ItemID / Count를 받음
2. `FItemData`에서 이름·아이콘·rarity 같은 정적 정보를 조회
3. `USlotWidget`에 화면용 값을 전달
4. `OnInventoryChanged / OnEquipmentChanged`를 받아 변경 상태를 반영

합니다.

`USlotWidget`은 다음 화면에서 공통 Item interaction/view로 재사용됩니다.

- Player Inventory grid
- Equipment slot
- Container item
- Crafting recipe/material

Container 화면은 `UContainerInteractionWidget` 안에 Player Inventory와 Container Inventory를 함께 배치합니다.

~~~text
ContainerInteractionWidget
 ├─ PlayerInventoryWidget
 └─ ContainerInventoryWidget
~~~

### Crafting UI

Crafting UI는 세 종류의 데이터를 결합합니다.

~~~text
Recipe DataTable
   ├─ 결과 Item
   ├─ 필요 Material
   └─ WorkRequired
        +
Player Inventory
   └─ 현재 보유 수량
        +
Crafting Station
   └─ ActiveWork / CompletedToCollect
        ↓
UCraftingWidget / Queue Widget
~~~

Recipe가 바뀔 때는 결과 Item·Icon·Material Row 같은 정적 구조를 갱신하고, Inventory 수량이나 Work progress 변화에는 필요한 값만 갱신합니다.

Required Material Row는 부모 Widget이 local pool을 소유해 필요한 수만 활성화하고 나머지는 `Collapsed`로 재사용합니다.

Crafting Queue는 `OnWorkChanged`, `OnCompletedChanged`로 이산 상태를 반영하고 연속적인 progress만 현재 Work progress를 사용합니다.

### Confirm Popup

Drop / Discard처럼 추가 확인이 필요한 동작은 `UInventoryWidget`이 `UConfirmWidget`을 생성해 처리합니다.

~~~text
ItemID
MaxCount
Drop / Discard Mode
   ↓
Confirm Widget
   ↓ OnConfirm
InventoryWidget
   ↓
ServerDrop / ServerDiscard
~~~

Confirm Widget은 확인 결과를 부모 Inventory 화면으로 반환하고, 실제 Item mutation은 Server request 경로에서 처리됩니다.

프로젝트 전체 Screen/Input 구조는 [[11. UI Architecture & Client Presentation|11_Client_State_Presentation_Pipeline]]에서 설명합니다.

---

## Integration

### Dungeon Reward

Dungeon의 `GrantReward`는 기존 `Inventory.AddItem()` 경로를 사용합니다.

~~~text
Dungeon Runtime
   ↓ GrantReward
Inventory.AddItem
   ↓
Stacking / Event / Replication
~~~

Dungeon 보상도 Field Loot와 같은 Inventory stacking / event / replication 경계를 재사용합니다.

---

## Trade-offs

| 선택 | 얻은 것 | 비용 / 제약 |
|---|---|---|
| **Inventory를 Item 중심 경계로 사용** | Loot / Equipment / Crafting / Reward가 같은 Item API 재사용 | Inventory가 여러 시스템의 orchestration point가 됨 |
| **획득 Event 분리** | Slot 변경과 실제 획득 UX를 구분 | Add 경로에서 semantic flag 관리 필요 |
| **Limited Prediction** | Slot 조작의 즉각적인 반응 | Server 결과와 local mirror 보정 필요 |
| **Container viewer lifecycle** | 사용 중인 shared state에 replication 집중 | connection별 payload filtering은 별도 문제 |
| **UIOwner 범위 제한** | Recipe 시작 충돌은 막고 이후 협력 작업 허용 | 여러 독립 queue를 동시에 지원하는 Station에는 부적합 |
| **ActiveWork / Completed 분리** | 작업 중간 수령·부분 취소 처리 | Work state가 단순 progress보다 복잡 |
| **공통 Resource Provider** | UI와 Server 계산 규칙 중복 감소 | Inventory API와 Crafting 사이 공통 계층 유지 필요 |

네트워크 전송/동기화 선택은 [[12. Multiplayer Synchronization|12_Multiplayer_Synchronization]]에서 더 자세히 설명합니다.

---

## 연관 문서

- [[04. Player & Character Systems|04_Player_Character_Systems]] — Equipment → Stat / Skill 적용과 Runtime 영상
- [[05. Combat, Skill & Animation|05_Combat_Skill_Animation]] — Equipment → Skill Grant
- [[06. World Interaction Systems|06_World_Interaction_Systems]] — Container / Crafting Station의 Interaction contract
- [[09. Branching Dungeon Runtime|09_Branching_Dungeon_Runtime]] — Reward → Inventory 재사용
- [[11. UI Architecture & Client Presentation|11_Client_State_Presentation_Pipeline]] — Screen / Notice / Content UI lifecycle
- [[12. Multiplayer Synchronization|12_Multiplayer_Synchronization]] — FastArray / Prediction / shared state 동기화

---

## 관련 코드

- [InventoryComponent.h](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Player/Utility/InventoryComponent.h)
- [ContainerComponent.h](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/GameObject/Buildings/Utility/ContainerComponent.h)
- [CraftingStation.h](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/GameObject/Buildings/Crafting/CraftingStation.h)
- [InventoryResourceProvider.h](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/Utilities/InventoryResourceProvider.h)
- [CraftingWidget.cpp](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/UI/Widget/GameObject/Crafting/CraftingWidget.cpp)
