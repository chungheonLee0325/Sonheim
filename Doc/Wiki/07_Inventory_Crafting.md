# 07. Inventory & Crafting

**플레이어 인벤토리를 아이템 상태의 중심으로 두고, 장비·공유 보관함·협동 제작이 같은 아이템 처리 API를 사용**합니다. 인벤토리는 슬롯 단위 FastArray로 동기화하고, 제작대는 레시피 정의와 실제 작업 진행·완료 수량을 분리해 여러 플레이어의 작업을 누적합니다.

## 실제 게임 화면

### 인벤토리·보관함

https://github.com/user-attachments/assets/c594e8a3-2840-456c-ae04-cabaaeb4d8ca

드래그 앤 드롭으로 슬롯을 교환하고, 플레이어 인벤토리와 월드 보관함 사이에서 아이템을 이동하는 동작입니다.

### 협동 제작

https://github.com/user-attachments/assets/df9a30d1-8d52-4b86-9f18-790474fdbda2

레시피·제작 수량 선택, 재료 소비, 공동 작업 진행, 완성품 수령과 인벤토리 반영까지 확인할 수 있습니다.

## 핵심 구현

| 기능 | 상태·실행 책임 | 구현 |
|---|---|---|
| **인벤토리** | `UInventoryComponent` | 스택·슬롯·장비·Drop/Discard 및 OwnerOnly FastArray |
| **아이템 이벤트** | `OnInventoryChanged / OnItemAdded` | 슬롯 변경과 실제 아이템 획득을 분리 |
| **장비** | `UInventoryComponent` → Stat/Skill/Player | 활성 무기 슬롯에 맞는 Modifier·Skill Grant·Mesh 전환 |
| **공유 보관함** | `UContainerComponent` | 월드 아이템 목록·구독자 유무에 따른 복제 활성화 |
| **제작 정의** | `FCraftingRecipe` DataTable | 결과 아이템·재료·단위당 작업량 |
| **제작 진행** | `ACraftingStation::ActiveWork` | 서버 재료 소비·작업 누적·부분 완료·수령·미완료 취소 |
| **화면 구성** | `UInventoryWidget / UCraftingWidget` | 실제 아이템 상태 + DataTable 표시 정보·재료 행 재사용 |

## 목차

- [인벤토리 상태와 FastArray](#인벤토리-상태와-fastarray)
- [슬롯 변경과 아이템 획득 이벤트](#슬롯-변경과-아이템-획득-이벤트)
- [장비 전환과 다른 시스템 연결](#장비-전환과-다른-시스템-연결)
- [월드 공유 보관함](#월드-공유-보관함)
- [협동 제작의 정의·실행 상태](#협동-제작의-정의실행-상태)
- [제작 시작·진행·부분 완료](#제작-시작진행부분-완료)
- [수령과 미완료 제작 취소](#수령과-미완료-제작-취소)
- [인벤토리·제작 UI 갱신](#인벤토리제작-ui-갱신)
- [설계 선택과 비용](#설계-선택과-비용)
- [관련 코드 및 문서](#관련-코드-및-문서)

---

## 인벤토리 상태와 FastArray

`UInventoryComponent`는 서버 게임플레이에서 사용하는 `InventoryItems`와 복제용 `RepItems`를 분리합니다. 복제 엔트리에는 **슬롯 위치·아이템 ID·수량**만 담습니다.

~~~cpp
// InventoryComponent.h — 실제 구조
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

~~~mermaid
flowchart TB
    GAME["서버 UInventoryComponent<br/>InventoryItems"] --> DIRTY["RepItems<br/>MarkItemDirty / MarkArrayDirty"]
    DIRTY --> CLIENT["OwnerOnly FastArray"]
    CLIENT --> ONREP["OnRep_RepItems"]
    ONREP --> MIRROR["RebuildInventoryArrayFromRep<br/>InventoryItems 로컬 미러"]
    MIRROR --> EVENT["OnInventoryChanged"]
    EVENT --> WIDGET["UInventoryWidget · USlotWidget"]
    DATA["FItemData<br/>이름·아이콘·장비 정의"] --> WIDGET
~~~

| 변경 | 복제 갱신 방식 |
|---|---|
| 기존 스택의 수량 변경 | 해당 `FRepInventoryEntry`에 `MarkItemDirty` |
| 슬롯 추가·삭제 | `MarkArrayDirty` 또는 변경한 엔트리 Dirty 처리 |
| 전체 복제 배열 재구성 | `SyncRepFromInventoryArray`에서 재생성 후 Dirty 처리 |
| 클라이언트 수신 | 슬롯 인덱스로 정렬해 `InventoryItems` 재구성, UI 이벤트 전달 |

~~~cpp
// InventoryComponent.cpp — 복제 조건
DOREPLIFETIME_CONDITION(UInventoryComponent, RepItems, COND_OwnerOnly);
DOREPLIFETIME_CONDITION(UInventoryComponent, EquippedSlots, COND_OwnerOnly);
DOREPLIFETIME(UInventoryComponent, CurrentWeaponSlot);
~~~

`RepItems`와 `EquippedSlots`는 소유자 전용이고, 현재 무기 슬롯은 일반 복제입니다. **아이템 슬롯의 복제 범위와 다른 플레이어에게 보일 무기 상태는 구분**합니다.

FastArray의 Delta 전송과 예측·보정 경로는 [12. Multiplayer Synchronization](12_Multiplayer_Synchronization.md)에서 다룹니다.

---

## 슬롯 변경과 아이템 획득 이벤트

슬롯을 교환하거나 장비를 해제하는 것과 **새 아이템을 획득하는 것**은 UI에서 다른 의미입니다.

| 상황 | `OnInventoryChanged` | `OnItemAdded` |
|---|---|---|
| 월드 아이템 획득 | 슬롯·수량 변경 | 직접 획득 수량 알림 |
| 아이템 스택 병합·슬롯 교환 | 변경 알림 | 별도 획득 알림 없음 |
| 장비 해제 후 가방에 복귀 | 변경 알림 | `IsDirectAcquisition=false`로 획득 알림 제외 |
| 제작 완료품 수령 | 인벤토리 변경 | `AddItem`의 직접 획득 경로 사용 |

`UInventoryComponent::AddItem(ItemID, ItemCount, IsDirectAcquisition)`은 서버에서 스택을 채우거나 새 슬롯에 추가하고, `IsDirectAcquisition`일 때만 `OnItemAdded`를 발생시킵니다. 획득 가능한 수량이 슬롯 한도를 넘으면 잔여 아이템을 월드에 드롭합니다.

슬롯 교환·장비 해제는 `OnInventoryChanged`만 사용하고, 직접 획득 시에만 `OnItemAdded`로 알림을 발생시킵니다.

### 클라이언트 조작 피드백

드래그 앤 드롭의 슬롯 교환은 `SwapItems`에서 서버 RPC를 통해 확정합니다. 선택적으로 `PerformClientPrediction_SwapItems`가 복사한 `PredictedInventory`를 먼저 UI에 전달할 수 있습니다.

~~~text
슬롯 드래그
  → 선택적 PredictedInventory 변경·UI 표시
  → ServerSwapItems
  → 서버 InventoryItems 확정
  → OwnerOnly RepItems
  → OnRep_RepItems → 로컬 배열·UI 갱신
~~~

`bEnableClientPrediction`은 `EditDefaultsOnly`이며 **C++ 기본값은 `false`**입니다. 구현된 예측 경로가 실제 게임에서 활성화되는지는 해당 컴포넌트의 Blueprint 기본값에 따릅니다. 최종 아이템 수량·슬롯은 서버 복제 결과를 사용합니다.

---

## 장비 전환과 다른 시스템 연결

`UInventoryComponent`는 장비를 장착할 때 슬롯 종류를 검사하고 `EquippedSlots`를 갱신합니다. `ApplyEquipState`와 `NotifyWeaponSlot`을 통해 선택 무기의 변경을 각 시스템에 전달합니다.

~~~text
장비 장착 / 활성 무기 슬롯 변경
    ↓ UInventoryComponent
    ├─ EquippedSlots · CurrentWeaponSlot
    ├─ UStatBonusComponent
    │    └─ 활성 무기 슬롯의 Modifier 적용
    ├─ USonheimSkillComponent
    │    └─ ReplaceGrant(ActiveWeaponGrantId, Skills)
    └─ OnWeaponChanged
         └─ ASonheimPlayer 무기 Mesh · Animation · HUD
~~~

무기별 `FItemData::EquipmentData.SkillID`를 조회해 `ReplaceGrant`로 스킬을 교체합니다. 다른 장비나 기본 보유 스킬이 제공하는 Grant는 분리해 관리합니다.

**곡괭이 ↔ Shotgun 전환 시 외형과 공격 방식이 함께 바뀌는 실제 영상**, Modifier 적용과 PlayerState/Pawn의 연결은 [04. Player & Character Systems](04_Player_Character_Systems.md)에서 볼 수 있습니다. 이 문서에서는 **인벤토리의 슬롯/아이템 API가 장비 전환을 시작한다는 경계**만 다룹니다.

---

## 월드 공유 보관함

`UContainerComponent`도 아이템 ID·수량·슬롯 위치를 가진 `FRepContainerList`를 사용합니다. 다만 `PlayerState`의 개인 인벤토리가 아니라 **월드 Actor의 공유 상태**입니다.

~~~text
보관함 열기
    ↓ ServerSubscribeViewer
Subscribers.Add(Viewer)
    ↓ ForceNetUpdate
ContainerItems → RepContainerItems
    ↓
Container UI에서 인벤토리와 보관함을 함께 표시

보관함 닫기
    ↓ ServerUnsubscribeViewer
Subscribers.Remove(Viewer)
~~~

서버의 `PreReplication`은 구독자 수를 기준으로 내부 슬롯 복제를 켜고 끕니다.

~~~cpp
// ContainerComponent.cpp
const bool bActive = Subscribers.Num() > 0;
DOREPLIFETIME_ACTIVE_OVERRIDE(
    UContainerComponent, RepContainerItems, bActive);
~~~

구독자가 없다면 보관함 내부 슬롯의 프로퍼티 복제를 비활성화합니다. 구독 중에는 Actor 관련성 및 복제 조건에 따라 상태를 전달합니다. **현재 구현은 구독자 유무에 따라 프로퍼티 전체를 활성화하며, 연결별 별도 수신자 필터를 구성하지 않습니다.**

보관함 UI도 인벤토리의 `USlotWidget`과 슬롯 이동 이벤트를 사용합니다.

---

## 협동 제작의 정의·실행 상태

제작대는 **레시피의 정적 정의**와 **현재 진행 중인 서버 상태**를 분리합니다.

### FCraftingRecipe — DataTable의 제작 정의

~~~cpp
// CraftingStation.h — 주요 필드
struct FCraftingRecipe : public FTableRowBase
{
    FText DisplayName;
    int32 ResultItemID = 0;
    int32 ResultCount = 1;
    int32 WorkRequired = 100;
    TMap<int32, int32> RequiredMaterials;
};
~~~

### FActiveCraftWork — 현재 제작 작업

~~~cpp
// CraftingStation.h — 주요 필드
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

| 정의/상태 | 의미 |
|---|---|
| `FCraftingRecipe` | 결과 아이템·개수, 필요 재료, 1개당 작업량 |
| `FActiveCraftWork` | 서버가 승인한 제작 수량, 완료 수량, 현재 단위의 누적 작업량 |
| `bHasActiveWork` | 아직 진행할 제작 단위가 있는지 |
| `CompletedToCollect` | 완성됐으나 플레이어가 수령하지 않은 아이템 총수량 |
| `UIOwner` | 레시피 선택 UI를 사용 중인 플레이어 |

제작대는 **하나의 `ActiveWork`에 같은 레시피의 여러 제작 단위를 순차적으로 누적**합니다. 서로 다른 레시피를 동시에 실행하는 독립 큐가 아닙니다.

---

## 제작 시작·진행·부분 완료

### 1. 레시피 선택과 재료 소비

`UIOwner`는 **레시피 선택 UI의 사용권**을 나타냅니다. `ServerRequestOpenUI`는 다른 사용자가 이미 열었다면 진입시키지 않습니다. 실제 `ActiveWork`가 시작된 뒤에는 다른 플레이어도 제작 작업에 참여할 수 있습니다.

~~~mermaid
flowchart TB
    SELECT["레시피 · 제작 수량 선택"] --> START["ServerStartWork"]
    START --> VALIDATE["Recipe · Units 검사<br/>ComputeMaxCraftable"]
    VALIDATE --> CONSUME["InventoryResourceProvider<br/>ConsumeItems"]
    CONSUME --> ACTIVE["FActiveCraftWork<br/>UnitsTotal · WorkPerUnit"]
    ACTIVE --> INPUT["플레이어 상호작용<br/>ServerAddWork"]
    INPUT --> ACCUM["WorkAccumulated += WorkDelta"]
    ACCUM --> CHECK{"WorkAccumulated ≥ WorkPerUnit?"}
    CHECK -->|"예"| FINISH["UnitsDone += 1<br/>CompletedToCollect += ResultPerUnit"]
    FINISH --> MORE{"남은 제작 단위?"}
    MORE -->|"예"| INPUT
    MORE -->|"아니오"| READY["bHasActiveWork = false<br/>완성품 수령 대기"]
    CHECK -->|"아니오"| INPUT
~~~

`ServerStartWork`는 레시피와 제작 수량을 확인하고, `UInventoryResourceProvider::ComputeMaxCraftable`로 **현재 서버 인벤토리의 재료 기준 최대 수량**을 계산합니다. 실제 소비 성공 횟수만 `ActiveWork.UnitsTotal`로 설정합니다.

~~~cpp
// InventoryResourceProvider.cpp — 실제 처리
if (!HasItems(Inv, Required)) return false;
for (const auto& P : Required)
{
    Inv->RemoveItem(P.Key, P.Value);
}
return true;
~~~

`HasItems`·`ComputeMaxCraftable`·`ConsumeItems`를 UI와 서버 제작 경로에서 공통으로 활용합니다.

### 2. 서로 다른 플레이어의 작업량을 하나로 누적

제작대의 `Interact`는 작업 중일 때 `ResolvePlayerWorkSpeed_Internal(Player)`를 호출해 **해당 플레이어의 WorkSpeed 스탯**을 구하고 `ServerAddWork`에 전달합니다.

`ServerAddWork`가 공유 `ActiveWork.WorkAccumulated`에 작업량을 더하고, `WorkPerUnit` 이상일 때마다 단위 하나를 완료합니다. 한 번의 작업량이 여러 단위의 작업량을 넘을 수 있으므로 **`while` 루프로 단위 완료를 처리**합니다.

`ActiveWork`·`bHasActiveWork`·`CompletedToCollect`는 `ACraftingStation`의 복제 상태이며, 변경 시 `OnWorkChanged`·`OnCompletedChanged`가 진행 UI를 갱신합니다.

### 3. 완료된 결과는 작업 중에도 별도 보관

예를 들어 한 번에 5개 제작을 시작한 뒤 2개를 완료했다면 다음과 같습니다.

~~~text
UnitsTotal         5
UnitsDone          2
CompletedToCollect 2 × ResultPerUnit
WorkAccumulated    현재 3번째 단위의 작업량
~~~

**완료 수량은 다음 단위의 작업 진행과 분리**됩니다. 진행 중에도 `ManualCollectDelay`가 지난 뒤 다시 상호작용하면 완성품을 수령할 수 있습니다. 수령 직후 `Client_StopInteractionHold`로 연속 입력을 정리합니다.

상호작용 Hold 자체의 다형성과 입력 처리 방식은 [06. World Interaction Systems](06_World_Interaction_Systems.md)에서 설명합니다.

---

## 수령과 미완료 제작 취소

### 완료 아이템 수령

`ServerCollectAll`은 `CompletedToCollect`만큼의 결과 아이템을 플레이어 인벤토리에 `AddItem`하고, `EPlayerDeed::Craft` 이벤트를 기록합니다.

이후 **수령한 결과 수량을 완료 단위 수로 환산**해 `ActiveWork.UnitsDone / UnitsTotal`을 정리합니다. 진행 중인 나머지 작업은 유지하고, 더 이상 진행 중인 작업이 없다면 완료 작업 상태를 초기화합니다.

### 취소 — 미완료 단위만 환불

`ServerCancelUnfinished`는 이미 완료된 결과를 유지하면서 제작을 종료합니다.

~~~cpp
// CraftingStation.cpp — 환불 대상 계산
const int32 RemainUnits =
    ActiveWork.UnitsTotal - ActiveWork.UnitsDone;

for (const auto& P : R->RequiredMaterials)
{
    const int32 RefundCount = P.Value * RemainUnits;
    if (RefundCount > 0)
    {
        Inv->AddItem(P.Key, RefundCount);
    }
}
~~~

예를 들어 5개를 시작해 2개를 완성했다면 취소 시 남은 3개분의 재료를 환불하고, 완료된 2개는 `CompletedToCollect`에 유지합니다. 재료는 제작 시작 때 소비하며, 미완료 단위에 대해서만 환불합니다.

---

## 인벤토리·제작 UI 갱신

### 슬롯 표현 재사용

`UInventoryWidget`은 `OnInventoryChanged`로 일반 슬롯을, `OnEquipmentChanged`로 장비 슬롯을 갱신합니다.

`USlotWidget`은 인벤토리·장비·보관함·제작 재료의 아이템 표현을 공유합니다. 더 자세한 Widget 갱신·성능 측정은 [11. Client State Presentation](11_Client_State_Presentation_Pipeline.md)과 [20. UI Performance](20_UI_Performance_Profiling.md)를 참고할 수 있습니다.

아이템 ID·수량은 `UInventoryComponent`에서, 이름·아이콘은 `FItemData`에서 읽습니다. Drop/Discard 확인은 `UConfirmWidget`의 결과를 `UInventoryWidget`이 받아 서버 요청으로 전달합니다.

### 제작 레시피 변경과 수량 변경의 갱신 구분

`UCraftingWidget::RefreshDetail`은 레시피 변경 시에만 `RebuildStaticForRecipe`를 호출합니다.

| 갱신 시점 | 갱신 대상 |
|---|---|
| **레시피 변경** | 결과 아이템 이름·아이콘, 재료 목록/아이콘 |
| **수량·인벤토리 변경** | 필요·보유 수량, 제작 가능 최대 수량, 버튼 활성화 |
| **작업 진행·완료** | `OnWorkChanged` · `OnCompletedChanged`로 진행·수령 가능 상태 |

재료 행은 `RequiredRowPool`이 소유하며, **필요한 재료 종류 수보다 부족할 때만 위젯을 추가 생성**합니다. 사용하지 않는 행은 `Collapsed`로 보관하고 이후 다른 레시피에서 재사용합니다.

재료가 4종인 레시피에서 생성한 Row는 다음 레시피가 2종·3종을 요구할 때 기존 Row를 재사용합니다.

작업 진행·수량 변경에는 Row를 다시 만들지 않고 숫자·버튼·진행도만 갱신합니다.

---

## 설계 선택과 비용

| 선택 | 구현 효과 | 관리 비용 |
|---|---|---|
| **인벤토리 슬롯 FastArray** | 슬롯별 변경을 소유 클라이언트에 전달 | Dirty 표시·클라이언트 미러 갱신 |
| **획득 이벤트 분리** | 슬롯 조작과 획득 알림을 구분 | 직접 획득 여부를 호출부에서 지정 |
| **장비 경로 연결** | 동일 아이템이 스탯·스킬·외형 변경에 사용 | 장비 이벤트의 적용 순서·동기화 |
| **Container 구독 상태** | Viewer가 없을 때 슬롯 복제 비활성화 | 구독자별 개별 페이로드 필터는 사용하지 않음 |
| **레시피와 ActiveWork 분리** | 데이터 정의 변경 없이 공유 작업·부분 완료 관리 | 작업·완료·수령 대기 상태 구분 |
| **재료 선소비와 미완료 환불** | 여러 작업자가 하나의 진행 상태 공유 | 제작 단위·완료 단위·환불 수량 일관성 |
| **제작 UI 정적/동적 갱신·Row Pool** | 레시피 변경 외에는 위젯 재생성 감소 | 캐시된 재료 ID·행 가시성 관리 |

## 관련 코드 및 문서

- [InventoryComponent.cpp](../../Sonheim/Source/Sonheim/AreaObject/Player/Utility/InventoryComponent.cpp) · [InventoryComponent.h](../../Sonheim/Source/Sonheim/AreaObject/Player/Utility/InventoryComponent.h) — 인벤토리·장비·FastArray·이벤트
- [ContainerComponent.cpp](../../Sonheim/Source/Sonheim/GameObject/Buildings/Utility/ContainerComponent.cpp) — 공유 보관함·Viewer
- [CraftingStation.cpp](../../Sonheim/Source/Sonheim/GameObject/Buildings/Crafting/CraftingStation.cpp) · [CraftingStation.h](../../Sonheim/Source/Sonheim/GameObject/Buildings/Crafting/CraftingStation.h) — 제작 서버 상태·레시피·부분 수령·취소
- [InventoryResourceProvider.cpp](../../Sonheim/Source/Sonheim/Utilities/InventoryResourceProvider.cpp) — 제작 수량 계산·재료 소비
- [CraftingWidget.cpp](../../Sonheim/Source/Sonheim/UI/Widget/GameObject/Crafting/CraftingWidget.cpp) — 정적/동적 UI 갱신·RequiredRowPool
- [InventoryWidget.cpp](../../Sonheim/Source/Sonheim/UI/Widget/Player/Inventory/InventoryWidget.cpp) — 슬롯 표시·Drop/Discard
- [04. Player & Character Systems](04_Player_Character_Systems.md) — 장비·스탯·스킬 전환 영상과 구현
- [06. World Interaction Systems](06_World_Interaction_Systems.md) — 제작대·보관함 입력
- [09. Branching Dungeon Runtime](09_Branching_Dungeon_Runtime.md) — 던전 보상에서 기존 인벤토리 사용
- [11. UI Architecture & Client Presentation](11_Client_State_Presentation_Pipeline.md) — 입력·화면 수명 및 Delegate
- [12. Multiplayer Synchronization](12_Multiplayer_Synchronization.md) — FastArray·OwnerOnly·예측
