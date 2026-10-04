# 07. Inventory & Crafting

Inventory, Container, Equipment, Crafting은 모두 Item을 다루지만 **소유 범위와 상호작용 방식이 다릅니다.**

Sonheim에서는 하나의 Item 흐름을 공유하되:

- 개인 보관은 Player Inventory
- 공유 보관은 Container
- 장비는 Stat / Skill Source
- 제작은 여러 Player가 참여하는 Shared Workflow

로 역할을 나눴습니다.

---

## 시스템 관계

~~~mermaid
flowchart LR
    ITEM["Item / Resource"]
    INV["Player Inventory"]
    EQUIP["Equipment"]
    STAT["Stat Bonus"]
    SKILL["Skill Grant"]

    BOX["World Container"]
    CRAFT["Crafting Station"]
    RESULT["Crafted Item"]

    ITEM --> INV
    BOX <--> INV
    INV --> EQUIP
    EQUIP --> STAT
    EQUIP --> SKILL

    INV -->|"Material"| CRAFT
    CRAFT -->|"Completed"| RESULT
    RESULT --> INV
~~~

Inventory를 단순 슬롯 UI가 아니라 **Item 상태가 다른 gameplay system으로 들어가는 중심 경계**로 사용합니다.

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

## 2. Inventory 변화와 “Item 획득”을 같은 Event로 취급하지 않는다

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

## 3. Equipment는 Inventory의 끝이 아니라 Stat / Skill의 Source

장비를 옮기는 것은 슬롯 변경만으로 끝나지 않습니다.

~~~text
Equip Item
   ↓
Equipped Slot
   ├─ Stat Modifier 적용
   ├─ Weapon Mesh 갱신
   ├─ Weapon HUD 갱신
   └─ Skill Grant 교체
~~~

Inventory Component는 현재 Weapon이 부여한 Skill Source를 <code>ActiveWeaponGrantId</code>로 추적합니다.

Weapon이 교체되면 Skill Component의 <code>ReplaceGrant()</code>를 사용해 **해당 장비가 제공한 Skill set만 교체**합니다.

이 때문에 Inventory / Stat / Combat이 서로 직접 뒤엉키기보다 Equipment change를 경계로 연결됩니다.

---

## 4. Drag & Drop은 즉시 보이고 최종 상태는 다시 맞춘다

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

## 6. Crafting은 Recipe보다 “Shared Work State”가 핵심

Crafting Station은 단순히 재료를 Item으로 교환하는 메뉴가 아닙니다.

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

즉 Recipe는 **무엇을 만드는가**, ActiveWork는 **현재 여러 Player가 어디까지 작업했는가**를 표현합니다.

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

별도의 “도움 버튼 / 수령 버튼 / 메뉴 버튼”을 World Actor에 각각 만들지 않고 **현재 authoritative work state가 Interaction 의미를 결정**합니다.

---

## 8. Recipe 선택 구간만 독점하고 실제 작업은 협력 가능

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

Station 전체를 한 Player에게 잠그지 않고 **실제로 경쟁 상태가 생기는 구간만 잠근 것**입니다.

---

## 9. Crafting Lifecycle은 Server의 하나의 상태 머신으로 진행

~~~mermaid
flowchart LR
    SELECT["Recipe Select"]
    VALIDATE["Material / Units 검증"]
    CONSUME["Material 소비"]
    WORK["ActiveWork"]
    ADD["Player Work 누적"]
    DONE["Unit 완료"]
    COLLECT["Collect"]
    INV["Inventory 지급"]

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

## 10. UI와 Server가 재료 계산 규칙을 따로 복사하지 않는다

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

## 13. Dungeon Reward도 같은 Inventory API를 사용

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
