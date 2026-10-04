# 07. Inventory & Crafting

Item은 Player Inventory를 중심으로 **Equipment, Container, Crafting**에 연결됩니다.

- **Player Inventory** — 개인 Item/Slot 상태
- **Container** — 공유 World 보관 상태
- **Equipment** — Stat / Skill 적용 source
- **Crafting** — Recipe와 여러 Player가 공유하는 Work state

---

## 시스템 관계

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

Player Inventory는 Loot, Equipment, Container, Crafting, Dungeon Reward가 공유하는 Item 상태의 중심 경계입니다.

---

## 시연 영상

기존 Inventory / Chest 시연 영상입니다. Drag & Drop과 Container 간 Item 이동이 같은 Inventory UI 흐름에서 동작하는 모습을 확인할 수 있습니다.

https://github.com/user-attachments/assets/c594e8a3-2840-456c-ae04-cabaaeb4d8ca

---

## 1. Player Inventory는 Slot 기반 Item 상태를 소유

<code>UInventoryComponent</code>는 gameplay에서 사용하는 local array와 replication용 FastArray를 구분합니다.

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

실제 gameplay API는 <code>InventoryItems</code>를 사용하고, 변경된 slot만 <code>RepItems</code>에 반영합니다.

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

네트워크 동기화 방식 자체는 [[12. Multiplayer Synchronization|12_Multiplayer_Synchronization]]에서 분리해 설명합니다.

---

## 2. Inventory 변경과 Item 획득 Event 분리

Inventory 내용이 바뀌는 모든 경우에 획득 Popup을 띄우면 잘못된 UX가 됩니다.

| 상황 | Inventory Changed | Item Acquired |
|---|---:|---:|
| Field Item 획득 | O | O |
| 장비 해제 후 Inventory 복귀 | O | X |
| Slot Swap | O | X |
| Container에서 이동 | O | 상황에 따라 구분 |

그래서:

- <code>OnInventoryChanged</code> — 구조적 상태 변화
- <code>OnItemAdded</code> — Player가 새 Item을 직접 획득한 의미

를 분리합니다.

<code>AddItem(..., bool IsDirectAcquisition)</code>이 이 의미 차이를 전달합니다.

**데이터 변화와 Player-facing Event를 같은 것으로 보지 않은 사례**입니다.

---

## 3. Equipment 변경 → Stat / Skill 적용

장비를 옮기는 것은 슬롯 변경만으로 끝나지 않습니다.

~~~text
Equip Item
   ↓
Equipped Slot
   ├─ Stat Modifier 적용
   ├─ Weapon Mesh / Animation 갱신
   ├─ Weapon HUD 갱신
   └─ Skill Grant 교체
~~~

무기 데이터에는 실제로 <code>WeaponType</code>과 <code>SkillID</code>가 함께 들어 있습니다.

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

현재 활성 Weapon Slot이 바뀌면 Inventory Component가 Item의 <code>SkillID</code>를 읽고 Skill Component의 <code>ReplaceGrant()</code>로 **그 무기가 제공하는 공격 Skill을 교체**합니다.

예를 들면:

- **곡괭이** — 장착한 도구에 맞는 근접/채굴 공격 Skill과 Mesh/Animation을 사용해 Resource를 공격
- **샷건** — Shotgun 계열 Skill로 전환되고, WeaponType에 맞는 Animation/Crosshair와 탄약 정보가 함께 연결

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

따라서 Player 입력 코드에 “곡괭이면 Mining, 샷건이면 Shotgun” 같은 무기별 분기문을 추가하기보다 **장비 데이터가 현재 사용할 Skill을 선택**합니다.

Inventory Component는 현재 Weapon이 부여한 Skill Source를 <code>ActiveWeaponGrantId</code>로 추적하므로 무기를 바꿀 때 다른 Source가 제공한 Skill까지 제거하지 않습니다.

---

## 4. Drag & Drop Prediction / Reconciliation

Slot Swap은 조작감이 중요한 UI이므로 Client가 local mirror를 먼저 바꿀 수 있습니다.

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

Prediction은 Inventory 결과를 Client에게 맡기는 것이 아니라 **round-trip 동안 보여줄 화면 반응을 먼저 적용하는 것**입니다.

Prediction / Owner-only FastArray의 세부 네트워크 선택은 [[12. Multiplayer Synchronization|12_Multiplayer_Synchronization]]에서 다룹니다.

---

## 5. Container는 같은 Item 구조를 공유하지만 소유 모델이 다르다

Player Inventory는 한 Player의 상태지만 Container는 World에 놓인 공유 보관함입니다.

그래서 Container도 Slot 기반 FastArray를 사용하되, **아무도 보고 있지 않을 때 내부 Item 상태를 계속 활성화하지 않습니다.**

Player가 Container를 열면 viewer로 등록되고 닫으면 해제됩니다.

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

이 구조로 개인 Inventory와 World Container가 같은 Item 모델을 공유하면서도 서로 다른 ownership을 유지합니다.

---

## 6. Recipe Definition과 Shared Work State

Crafting Station은 Recipe 정의와 실행 중인 <code>FActiveCraftWork</code>를 분리해 제작 상태를 관리합니다.

Recipe는:

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

를 정의하고, 실행 중 상태는 별도의 <code>FActiveCraftWork</code>가 가집니다.

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

Recipe는 **무엇을 만드는가**, ActiveWork는 **현재 몇 Unit을 어느 정도 진행했는가**를 표현합니다.

---

## 7. 같은 Interaction이 Station 상태에 따라 다른 행동을 한다

Crafting Station은 <code>IInteractableInterface</code>를 구현하고 현재 상태에 따라 같은 Interaction 입력의 의미를 바꿉니다.

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

현재 authoritative work state에 따라 같은 Interaction 입력이 Recipe UI, Work 추가, Collect로 연결됩니다.

---

## 8. Recipe 시작 구간의 UI Ownership과 협력 작업

여러 Player가 동시에 Recipe UI를 열고 서로 다른 작업을 시작하면:

- 어떤 Recipe가 ActiveWork인지
- 누구의 재료를 소모할지

가 충돌합니다.

그래서 <code>UIOwner</code>는 **Recipe 선택/작업 시작 구간만 exclusive**하게 만듭니다.

~~~cpp
if (UIOwner && UIOwner != Player)
    return;

UIOwner = Player;
~~~

작업이 시작된 뒤에는 다른 Player도:

- Work 추가
- Assist
- 완료 결과 Collect

에 참여할 수 있습니다.

<code>UIOwner</code>는 Recipe 선택과 작업 시작 구간에만 적용되고, ActiveWork가 시작된 뒤에는 다른 Player도 Work 추가와 Collect에 참여할 수 있습니다.

---

## 9. Crafting Lifecycle은 Server의 하나의 상태 머신으로 진행

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

Client UI에 표시된 “제작 가능 수량”을 최종 판정으로 사용하지 않고, <code>ServerStartWork()</code>에서 다시 계산합니다.

---

## 10. 공통 Resource Provider로 재료 계산 공유

Crafting UI도:

- 필요한 Item
- 현재 보유 수량
- 최대 제작 가능 수량

을 계산해야 합니다.

이 계산을 Widget에 복사하지 않고 <code>UInventoryResourceProvider</code>가 공통 read/consume 규칙을 제공합니다.

~~~cpp
static int32 ComputeMaxCraftable(
    UInventoryComponent* Inv,
    const TMap<int32, int32>& Required);

static bool ConsumeItems(
    UInventoryComponent* Inv,
    const TMap<int32, int32>& Required);
~~~

UI는 같은 규칙으로 예상값을 표시하고, Server는 최종 상태에서 다시 검증합니다.

---

## 11. 완료된 Unit과 미완료 Unit의 수명을 분리

Work가 진행되면서 한 Unit이 완료될 때마다:

- <code>UnitsDone</code> 증가
- <code>CompletedToCollect</code> 증가

합니다.

완성된 결과는 작업이 아직 계속 중이어도 수령할 수 있습니다.

Cancel 시에도 이미 완료된 결과를 지우지 않습니다.

~~~text
UnitsTotal = 5
UnitsDone  = 2

Cancel
  ↓
완료 2개 → 유지 / 수령 가능
미완료 3개 → 재료 환불
~~~

“작업 전체 취소”와 “이미 만들어진 결과 취소”를 구분합니다.

---

## 12. Crafting UI는 정적 구조와 자주 바뀌는 상태를 나눈다

Recipe를 바꿀 때는:

- 결과 Item
- Icon
- Material Row 구성

을 갱신합니다.

Inventory 수량이나 제작 개수만 바뀔 때는:

- 보유 수량
- 필요 수량
- 가능/불가능 상태
- Button enable

같은 값만 갱신합니다.

Required Material Row는 local pool을 사용해 Recipe 변경마다 Widget을 새로 생성하지 않습니다.

Crafting Queue도 <code>OnWorkChanged</code>, <code>OnCompletedChanged</code>로 이산 상태를 갱신하고, 연속적인 progress 표현만 현재 work progress를 사용합니다.

---

## 13. Inventory / Crafting UI Data Flow

Inventory/Crafting UI의 실제 데이터 소유자는 Widget이 아닙니다.

### Inventory Screen

~~~mermaid
flowchart LR
    SERVER["<b>Authoritative Inventory</b><br/>UInventoryComponent"]
    REP["<b>Client Mirror</b><br/>InventoryItems"]
    EVENT["<b>변경 알림</b><br/>OnInventoryChanged<br/>OnEquipmentChanged"]
    SCREEN["<b>Screen Controller</b><br/>ASonheimPlayerController"]
    INVUI["<b>Inventory View</b><br/>UInventoryWidget"]
    SLOT["<b>재사용 Slot</b><br/>USlotWidget"]
    DATA["<b>정적 Item Data</b><br/>FItemData / GameInstance"]

    SERVER --> REP
    REP --> EVENT
    EVENT --> INVUI
    SCREEN --> INVUI
    DATA --> INVUI
    INVUI --> SLOT
~~~

PlayerController가 Inventory Screen을 열 때 <code>UInventoryWidget</code>을 생성하고 <code>UInventoryComponent</code>를 주입합니다.

Widget은:

1. <code>InventoryItems</code>에서 ItemID / Count를 받음
2. GameInstance의 <code>FItemData</code>에서 이름·아이콘·rarity 같은 정적 정보를 조회
3. 화면용 <code>USlotWidget</code>에 전달
4. 이후 <code>OnInventoryChanged / OnEquipmentChanged</code>를 받아 다시 반영

하는 역할입니다.

즉 **replicated runtime state와 정적 Item definition을 UI에서 조합하되, Widget 자체가 원본 gameplay state를 만들지는 않습니다.**

### 같은 Slot Widget을 여러 화면에서 재사용

<code>USlotWidget</code>은 Item slot의 공통 interaction/view 역할을 맡아 여러 화면에서 재사용됩니다.

- Player Inventory grid
- Equipment slot
- Container item
- Crafting recipe/material 표현

에서 같은 Item icon / quantity / drag-drop 기반을 재사용합니다.

Container 화면도 별도 Inventory UI를 다시 만들지 않고 <code>UContainerInteractionWidget</code> 안에:

~~~text
PlayerInventoryWidget
+
ContainerInventoryWidget
~~~

을 배치해 양쪽 Item state를 한 화면에서 연결합니다.

### Crafting UI

Crafting 화면은 두 종류의 source를 결합합니다.

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

Recipe가 바뀔 때만 이름·아이콘·Material row 같은 **정적 구조**를 재구성하고, Inventory 수량이나 Quantity가 바뀔 때는 **동적 값만 갱신**합니다.

Required Material row는 부모 Widget이 pool을 소유해 부족할 때만 생성하고 나머지는 <code>Collapsed</code>로 재사용합니다.

### Confirm Popup

버리기/폐기처럼 추가 확인이 필요한 동작은 <code>UInventoryWidget</code>이 <code>UConfirmWidget</code>을 생성하고:

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

로 결과를 돌려받습니다.

Confirm UI의 수명은 <code>UInventoryWidget</code>이 소유하며, 확인 결과를 부모 화면으로 반환합니다. 프로젝트 전체 Screen/Input 구조는 [[11. UI Architecture & Client Presentation|11_Client_State_Presentation_Pipeline]]에서 정리합니다.

---

## 14. Dungeon Reward도 같은 Inventory API를 사용

Dungeon의 <code>GrantReward</code>는 별도의 Reward Inventory를 만들지 않고 기존 <code>Inventory.AddItem()</code> 경로로 지급합니다.

~~~text
Dungeon Runtime
   ↓ GrantReward
Inventory.AddItem
   ↓
Stacking / Event / Replication
~~~

새 콘텐츠가 추가됐을 때 기존 Inventory 경계를 그대로 재사용한 사례입니다.

---

## 설계 선택과 비용

| 선택 | 얻은 것 | 비용 / 제약 |
|---|---|---|
| **Inventory를 Item 중심 경계로 사용** | Loot / Equipment / Crafting / Reward가 같은 Item API 재사용 | Inventory가 여러 시스템의 orchestration point가 됨 |
| **획득 Event 분리** | Slot 변경과 실제 획득 UX를 구분 | Add 경로에서 semantic flag 관리 필요 |
| **UIOwner 범위 제한** | Recipe 시작 충돌은 막고 이후 협력 작업은 허용 | 여러 독립 queue를 동시에 지원하는 Station에는 부적합 |
| **ActiveWork / Completed 분리** | 작업 중간 수령·부분 취소를 자연스럽게 처리 | Work 상태가 단순 progress float보다 복잡 |
| **공통 Resource Provider** | UI와 Server 계산 규칙 중복 감소 | Inventory API와 Crafting 규칙 사이 공통 계층 유지 필요 |

### 네트워크 구현 범위

개인 Inventory는 Owner-only FastArray, Container는 viewer가 있을 때 replication을 활성화하고, Slot UI에는 제한적 Prediction을 사용합니다.

이 선택의 전송/동기화 관점은 [[12. Multiplayer Synchronization|12_Multiplayer_Synchronization]]에서 별도로 설명합니다.

---

## 연관 문서

- [[05. Combat, Skill & Animation|05_Combat_Skill_Animation]] — Equipment → Skill Grant
- [[06. World Interaction Systems|06_World_Interaction_Systems]] — Container / Crafting Station의 공통 Interaction
- [[09. Branching Dungeon Runtime|09_Branching_Dungeon_Runtime]] — Reward → Inventory 재사용
- [[12. Multiplayer Synchronization|12_Multiplayer_Synchronization]] — FastArray / Prediction / shared state 동기화

---

## 관련 코드

- [InventoryComponent.h](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Player/Utility/InventoryComponent.h)
- [ContainerComponent.h](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/GameObject/Buildings/Utility/ContainerComponent.h)
- [CraftingStation.h](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/GameObject/Buildings/Crafting/CraftingStation.h)
- [InventoryResourceProvider.h](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/Utilities/InventoryResourceProvider.h)
- [CraftingWidget.cpp](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/UI/Widget/GameObject/Crafting/CraftingWidget.cpp)
