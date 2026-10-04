# 11. UI Architecture & Client Presentation

Sonheim의 UI는 모든 화면을 하나의 Manager에 넣지 않고 **화면의 수명·입력 점유 여부·데이터 복잡도**에 따라 관리 방식을 나눴습니다.

- 지속적으로 보이는 Player HUD
- Inventory / Container / Crafting 같은 상호작용 Screen
- 확인이 필요한 local Popup
- 잠깐 나타났다 사라지는 Notice / Toast
- 여러 gameplay state를 조합하는 Dungeon HUD / Minimap / Result

각 UI가 원본 gameplay state를 직접 소유하지 않고, 필요한 방식으로 state를 구독하거나 화면용 데이터로 변환해 소비합니다.

---

## 전체 구조

~~~mermaid
flowchart TB
    subgraph SOURCE["Gameplay / State"]
        COMP["<b>독립 Gameplay State</b><br/>Health · Stamina · Inventory · Equipment"]
        WORLD["<b>상호작용 State</b><br/>Container · Crafting Station"]
        EVENT["<b>일시적 Gameplay Event</b><br/>Level · Capture · Crafting · Region"]
        SNAP["<b>복합 Content State</b><br/>FDungeonStageRuntimeState"]
    end

    subgraph PRESENT["Client Presentation Layer"]
        CTRL["<b>기본 Screen 수명</b><br/>ASonheimPlayerController"]
        NOTICE["<b>Notice Queue</b><br/>UNoticeSubsystem : ULocalPlayerSubsystem"]
        PRESENTER["<b>Content Adapter</b><br/>UDungeonStagePresenter"]
        ROUTER["<b>Content UI Routing</b><br/>UDungeonUIRouterSubsystem"]
    end

    subgraph UMG["UMG / Widget"]
        HUD["<b>지속 HUD</b><br/>UPlayerStatusWidget"]
        SCREEN["<b>Menu / Interaction Screen</b><br/>Inventory · Container · Crafting"]
        POPUP["<b>Local Popup</b><br/>UConfirmWidget"]
        TOAST["<b>Transient Notice</b><br/>Banner · Title"]
        DUNGEON["<b>Complex Content UI</b><br/>Dungeon HUD · Minimap · Result"]
    end

    COMP --> CTRL --> HUD
    COMP --> SCREEN
    WORLD --> SCREEN
    SCREEN --> POPUP

    EVENT --> NOTICE --> TOAST

    SNAP --> PRESENTER
    PRESENTER --> ROUTER --> DUNGEON
~~~

| UI 종류 | 현재 소유/관리 | 데이터 source | 입력 정책 |
|---|---|---|---|
| Player HUD | PlayerController + Widget | Component / PlayerState Delegate | Game input 유지 |
| Inventory / Stat | PlayerController | Inventory / PlayerState | 메뉴 열림 동안 gameplay input 제한 |
| Container | PlayerController + ContainerInteractionWidget | Player Inventory + Container | 상호작용 Screen이 cursor/input 관리 |
| Crafting | CraftingStation + CraftingWidget | Recipe + Inventory + ActiveWork | Recipe 선택 중 UI ownership |
| Confirm Popup | InventoryWidget | ItemID / Count / mode | 부모 Screen 안의 local popup |
| Notice / Toast | UNoticeSubsystem | FNoticeData | non-modal |
| Dungeon HUD / Result | Dungeon Presenter + UI Router | Run Snapshot + Presentation Data | 현재 HUD/Result는 GameOnly |

---

## 1. C++ Widget Contract와 UMG 역할을 분리

Widget의 상태 연결과 입력 처리는 C++ parent에서, 실제 배치와 animation/art는 Widget Blueprint에서 구성합니다.

예를 들어 Inventory의 <code>UInventoryWidget</code>은:

- <code>BindWidget</code>으로 SlotGrid / Equipment Slot / Trash Zone 계약 정의
- Inventory delegate 처리
- Drag & Drop 요청
- ItemID → ItemData 변환

을 담당합니다.

Widget Blueprint는:

- 실제 panel tree
- anchor / padding / size
- texture / font
- animation

을 구성합니다.

~~~text
C++ Parent
 ├─ BindWidget Contract
 ├─ Data / Event 연결
 └─ Input Logic
        ↓
Widget Blueprint
 ├─ Layout
 ├─ Art / Style
 └─ Animation
~~~

AgentMcp의 UMG toolset도 이 경계를 기준으로 **C++ BindWidget contract와 실제 Widget Tree가 일치하는지 검사하면서 Widget Blueprint를 작성/수정**하도록 사용했습니다.

---

## 2. 지속 HUD — Delegate로 독립 상태를 구독

HP / Stamina / Level처럼 owner와 lifecycle이 명확한 값은 별도 Presentation Model을 만들지 않습니다.

PlayerController가 HUD를 생성한 뒤 component delegate를 연결합니다.

~~~text
HealthComponent.OnHealthChanged ─┐
StaminaComponent.OnChanged      ├─→ UPlayerStatusWidget
LevelComponent.OnLevelChanged   ┘
~~~

Widget이 Tick으로 gameplay object를 계속 polling하지 않고 상태 owner가 변화를 알립니다.

이 방식은 **독립된 값 하나를 바로 표현하는 HUD**에 적합합니다.

---

## 3. Inventory / Container / Crafting — Screen이 gameplay data를 조합

### Inventory

~~~mermaid
flowchart LR
    INV["<b>Authoritative / Local Mirror</b><br/>UInventoryComponent"]
    EVENT["<b>변경 Delegate</b><br/>Inventory / Equipment"]
    WIDGET["<b>Screen Logic</b><br/>UInventoryWidget"]
    DATA["<b>정적 Item Definition</b><br/>FItemData"]
    SLOT["<b>Reusable Item View</b><br/>USlotWidget"]

    INV --> EVENT --> WIDGET
    DATA --> WIDGET
    WIDGET --> SLOT
~~~

Inventory Widget은 ItemID/Count만으로 화면을 그리지 않고 GameInstance의 <code>FItemData</code>를 조회해 icon/name/rarity 같은 정적 정보를 결합합니다.

실제 Item state는 InventoryComponent가 유지하고 Widget은 화면용 Slot 상태로 변환합니다.

### Container

<code>UContainerInteractionWidget</code> 안에:

- Player용 <code>UInventoryWidget</code>
- Container용 <code>UContainerWidget</code>

을 함께 배치합니다.

Player Inventory UI를 다시 만들지 않고 같은 Slot/Drag & Drop 구조를 shared Container 화면에서 재사용합니다.

### Crafting

Crafting UI는 세 source를 결합합니다.

~~~text
Recipe DataTable
 + Player Inventory
 + Crafting Station ActiveWork
        ↓
UCraftingWidget / Queue Widget
~~~

Recipe 변경처럼 구조가 바뀌는 경우와 수량/진행도처럼 자주 바뀌는 값을 분리해 갱신합니다.

세부 Item/UI 흐름은 [[07. Inventory & Crafting|07_Inventory_Crafting]]에서 설명합니다.

---

## 4. Popup / Modal — 현재는 화면별 local ownership

버리기/폐기처럼 추가 확인이 필요한 경우 Inventory Widget이 <code>UConfirmWidget</code>을 직접 생성합니다.

~~~text
Trash / Discard Drop
      ↓
UInventoryWidget
      ↓ CreateWidget
UConfirmWidget
      ↓ Setup(ItemID, MaxCount, Mode)
      ↓ OnConfirm
UInventoryWidget
      ↓
ServerDrop / ServerDiscard
~~~

Popup은 자신이 gameplay mutation을 수행하지 않고 결과를 delegate로 부모 화면에 돌려줍니다.

### 현재 modal 관리 범위

현재 프로젝트에는 **모든 modal을 stack으로 관리하는 범용 UI Manager가 구현되어 있지는 않습니다.**

Inventory / PlayerStat menu는 PlayerController의 menu state가 gameplay input을 막고 cursor를 관리하며, Container/Crafting도 각 화면 lifecycle에서 cursor와 close 처리를 수행합니다.

Dungeon UI Registry에는:

~~~cpp
enum class EDungeonUILayer : uint8
{
    Screen,
    Modal,
    HUD
};

enum class EDungeonUIInputMode : uint8
{
    GameOnly,
    GameAndUI,
    UIOnly
};
~~~

처럼 Layer/Input 정책을 표현하는 schema가 있지만, 현재 Dungeon HUD/Result 경로는 다른 Inventory input과 충돌하지 않도록 **GameOnly + non-modal만 허용**합니다.

즉 현재 구조는:

~~~text
공통 Notice
→ 중앙화 완료

Dungeon Screen routing
→ LocalPlayer 단위로 분리

일반 Menu / Modal stack
→ 화면별 관리가 남아 있음
~~~

입니다.

UI 종류가 더 늘어 여러 modal의 우선순위·Back navigation·focus restore를 공통으로 처리해야 한다면 이 부분이 다음 일반화 지점입니다.

---

## 5. Notice / Toast — LocalPlayer 단위 공통 시스템

Level Up, Capture 실패, Crafting 완료, Region Title, Dungeon 진행처럼 **현재 state를 저장할 필요는 없지만 짧게 알려야 하는 이벤트**는 <code>UNoticeSubsystem</code>으로 일반화했습니다.

<code>UNoticeSubsystem : ULocalPlayerSubsystem</code>이므로 각 LocalPlayer 화면 수명에 맞게 존재합니다.

### Producer는 내용만 전달

~~~text
Gameplay Producer
  Level / Capture / Crafting / Dungeon / Region
        ↓
FNoticeData
        ↓
Slot + Channel
        ↓
UNoticeSubsystem
~~~

<code>FNoticeData</code>에는:

- Category
- Title
- Detail
- Symbol / Icon
- Tone

만 들어갑니다.

어디에 어떻게 표시할지는 producer가 Widget을 직접 생성하지 않고 Slot 설정이 결정합니다.

---

## 6. Notice Slot은 Queue 정책과 Style을 소유

현재 Slot:

- **Banner** — 진행/완료/경고 같은 카드형 메시지
- **Title** — 지역/방 이름 같은 화면 중앙 title

각 Slot의 config는:

~~~cpp
struct FNoticeSlotSettings
{
    TSoftClassPtr<UNoticeWidget> WidgetClass;
    TSoftObjectPtr<UNoticeStyle> Style;

    int32 ZOrder;
    ENoticeOrder Order;
    int32 MaxWaiting;
};
~~~

을 가집니다.

Queue policy:

### TakeTurns

현재 Notice가 끝난 뒤 다음 Notice를 순서대로 보여줍니다.  
대기 수가 <code>MaxWaiting</code>을 넘으면 오래된 waiting item을 제거합니다.

### ReplaceShowing

새 메시지가 들어오면 현재/대기 메시지를 교체합니다.

예를 들어 빠르게 여러 Region을 통과할 때 “가장 최근 위치”가 더 중요한 Title에 적합합니다.

---

## 7. Channel로 Producer의 Notice 수명을 분리

Notice는 Slot만으로 구분하지 않고 <code>Channel</code>도 함께 받습니다.

예:

~~~text
Banner
 ├─ Level
 ├─ Capture
 ├─ Crafting
 └─ Dungeon

Title
 ├─ Region
 └─ Dungeon
~~~

Dungeon Run이 끝날 때:

~~~cpp
Notices->Clear(TEXT("Dungeon"));
~~~

처럼 **Dungeon이 만든 waiting/showing notice만 회수**할 수 있습니다.

다른 system의 Notice를 함께 지우지 않습니다.

이는 transient UI에도 producer ownership을 부여한 구조입니다.

---

## 8. Notice Widget은 gameplay state를 읽지 않는다

<code>UNoticeWidget</code>은 전달받은 ViewData와 <code>UNoticeStyle</code>만 사용해:

- fade in
- hold
- fade out
- accent / color / icon
- lifetime progress

를 표현합니다.

Widget이 LevelComponent나 CraftingStation을 직접 참조하지 않습니다.

Style도 DataAsset으로 분리해 Widget layout을 바꾸지 않고 색/시간 값을 수정할 수 있습니다.

---

## 9. 전용 Item Popup도 함께 존재

Item 직접 획득은 기존 Player HUD의 별도 feedback 경로도 사용합니다.

~~~text
Inventory.OnItemAdded
      ↓
PlayerController Client RPC
      ↓
PlayerStatusWidget.DisplayItemPopup
      ↓
Widget Blueprint Event
~~~

이 경로는 <code>OnInventoryChanged</code>와 분리되어 있어 Slot Swap이나 장비 해제가 Item 획득 popup으로 보이지 않습니다.

현재는 **Item acquisition feedback은 HUD 전용 경로**, Level/Capture/Crafting/Region/Dungeon 알림은 **NoticeSubsystem**으로 공존합니다.

모든 transient feedback을 억지로 하나로 합치기보다 실제 사용 범위를 기준으로 유지하고 있습니다.

---

## 10. 복합 콘텐츠 UI — Snapshot을 바로 Widget에 넘기지 않는다

Dungeon은 Stage 하나만 표시하는 UI가 아닙니다.

- Stage / Objective
- Optional Objective
- Branch
- Party
- Timer
- Boss
- Minimap / Marker
- Reward / Result

를 한 화면에서 조합합니다.

그래서 다음 계층을 사용합니다.

~~~mermaid
flowchart LR
    R["<b>Server Content State</b><br/>UDungeonStageRuntimeSubsystem"]
    G["<b>Replicated Snapshot</b><br/>ASonheimGameState<br/>FDungeonStageRuntimeState"]
    P["<b>Client Adapter</b><br/>UDungeonStagePresenter"]
    D["<b>Presentation Definition</b><br/>UDungeonPresentationDataAsset"]
    V["<b>화면용 Model</b><br/>FDungeonStageViewData"]
    U["<b>Widget Lifecycle</b><br/>UDungeonUIRouterSubsystem"]
    W["<b>UMG</b><br/>HUD · Minimap · Result"]

    R --> G --> P
    D --> P
    P --> V --> U --> W
~~~

Widget이 GameplayTag, ItemID, Stage transition을 직접 해석하지 않습니다.

---

## 11. Presenter가 Runtime ID를 화면 정보로 변환

예:

~~~text
StageId
= Dungeon.ForgottenRuins.Stage.GuardRoom

Objective Group
= Dungeon.ForgottenRuins.Group.Guards

Runtime Count
= 2 / 4
~~~

Presenter는 Definition / Presentation Data와 결합해:

~~~text
"경비실"
"경비병을 처치하세요"
2 / 4
Room Icon
World Marker
~~~

같은 ViewData로 변환합니다.

Gameplay rule과 UI text/icon/map을 같은 data structure에 넣지 않습니다.

---

## 12. Snapshot은 Revision으로 최신 상태만 적용

같은 Run의 오래된 Snapshot이 뒤늦게 들어와 화면을 되돌리지 않도록 <code>Revision</code>을 비교합니다.

~~~cpp
if (Snapshot.RunId == Latest.RunId &&
    Snapshot.Revision <= Latest.Revision)
{
    return;
}
~~~

새 Run에서는 새 기준으로 시작하고, 같은 Run에서는 더 높은 Revision만 적용합니다.

---

## 13. 이미 독립적으로 복제되는 값은 Snapshot에 중복하지 않는다

Party HP는 Dungeon Snapshot에 다시 복제하지 않습니다.

~~~text
Dungeon Snapshot
 └─ Participants
        ↓
Participant Pawn
 └─ HealthComponent
        ↓
OnHealthChanged
        ↓
Presenter Refresh
~~~

- Dungeon progression → Snapshot
- Health 같은 독립 Actor state → 기존 Component / Delegate

를 병행합니다.

복합 UI를 만든다는 이유로 모든 state를 하나의 거대한 packet/model에 중복하지 않습니다.

---

## 14. Minimap / Marker도 ViewData로 전달

Presentation Data가 Map Texture, World Bounds, Stage Room Rect를 정의하고 Presenter가 현재 Stage/Objectives와 결합합니다.

~~~text
Presentation Map Data
 + Current Stage
 + Objective Marker Target
 + Player / Party Position
        ↓
Minimap ViewData
        ↓
UMG
~~~

Widget은 “현재 Shortcut Actor를 찾아라” 같은 gameplay query를 직접 수행하지 않습니다.

---

## 15. Timer / Boss Progress는 Server Time 기준

Stage countdown과 Boss action progress를 Client가 수신한 순간부터 새로 재지 않습니다.

Snapshot의:

- RunStartedServerTime
- StageDeadlineServerTime
- BossActionStartServerTime
- BossActionEndServerTime

을 synchronized server clock과 비교합니다.

HUD가 늦게 생성되거나 재생성돼도 같은 현재 progress를 복원합니다.

---

## 16. Result도 같은 Run Snapshot의 다른 View

Run 종료 시 별도 Result용 gameplay state를 다시 만들지 않습니다.

최종 Snapshot의:

- Reward
- Elapsed
- Defeated / Captured
- Branch
- Grade
- Best Record
- Fail Reason

을 Result ViewData로 변환합니다.

~~~text
Running Snapshot
→ Dungeon HUD

Terminal Snapshot
→ Result Popup / Screen
~~~

진행 화면과 결과 화면이 같은 authoritative Run의 서로 다른 presentation입니다.

---

## 17. LocalPlayer UI Router가 Dungeon Widget 수명을 관리

<code>UDungeonUIRouterSubsystem : ULocalPlayerSubsystem</code>은 gameplay rule을 해석하지 않고:

- HUD / Result Widget 선택
- Widget class async load
- 최신 ViewData cache
- 현재 Widget 교체
- Dungeon 중 기존 HUD hide / restore
- Dungeon Notice channel cleanup

을 담당합니다.

Async load가 늦게 끝났을 때 이전 lifecycle의 Widget을 다시 만들지 않도록 <code>Generation</code> guard도 사용합니다.

~~~text
Attach : Generation N
   ↓ Async Load

Detach / Reattach : Generation N+1
   ↓

Old Callback
Expected N != Current N+1
→ 무시
~~~

---

## 18. 기존 HUD를 숨길 때 원래 Visibility를 보존

Dungeon Run 동안 Island HUD처럼 충돌하는 화면은 Registry의 <code>HiddenDuringRun</code> 목록을 이용해 숨깁니다.

이때 단순히 종료 후 <code>Visible</code>로 바꾸지 않고 **각 Widget이 원래 갖고 있던 ESlateVisibility를 저장했다가 복원**합니다.

Run 도중 새로 생성된 대상 Widget도 다음 update에서 다시 찾아 숨깁니다.

Dungeon UI가 다른 UI의 lifecycle을 영구적으로 덮어쓰지 않도록 한 처리입니다.

---

## UI 재사용 / 최적화 사례

### Slot Widget 재사용
Inventory, Equipment, Container, Crafting에서 같은 Item Slot 기반을 사용합니다.

### Crafting Row local pool
Recipe 변경 때 Material Row를 매번 파괴/생성하지 않고 부모 Widget이 재사용합니다.

### Floating Damage global pool
전투 중 반복 생성되는 Damage Number는 별도 actor pool로 재사용합니다.

화면의 사용 패턴이 다르기 때문에 모든 Widget을 하나의 공통 pooling framework에 넣지 않고 **전역적으로 폭발하는 요소와 화면 내부 반복 요소를 다르게 처리**했습니다.

---

## 설계 선택과 현재 경계

| 선택 | 얻은 것 | 비용 / 현재 경계 |
|---|---|---|
| **Delegate 기반 HUD** | 단순 state 변경을 가볍게 반영 | 여러 source를 합치는 화면에는 부적합 |
| **C++ Widget + WBP layout** | 데이터 계약과 visual authoring 분리 | BindWidget 계약 관리 필요 |
| **LocalPlayer NoticeSubsystem** | producer와 transient UI lifecycle 분리 | 현재 Slot 종류는 Banner/Title 중심 |
| **Channel + Queue Policy** | 시스템별 notice 회수와 순서 제어 | 더 복잡한 priority 정책은 별도 확장 필요 |
| **Screen별 local popup 관리** | 구현이 단순하고 화면 context가 명확 | 범용 modal stack / focus history는 아직 없음 |
| **Presenter → ViewData** | 복합 gameplay schema와 UMG 분리 | mapping code 증가 |
| **LocalPlayer UI Router** | Dungeon Widget/async lifecycle 분리 | 현재 일반 UI 전체의 공통 router는 아님 |

---

## 연관 문서

- [[02. Gameplay Architecture|02_Gameplay_Architecture]] — Gameplay / Presentation 책임 경계
- [[06. World Interaction Systems|06_World_Interaction_Systems]] — Context Prompt와 Interaction UI
- [[07. Inventory & Crafting|07_Inventory_Crafting]] — Inventory/Container/Crafting UI의 실제 데이터 흐름
- [[09. Branching Dungeon Runtime|09_Branching_Dungeon_Runtime]] — Dungeon Snapshot의 원본 Runtime
- [[10. Boss Encounter Runtime|10_Boss_Encounter_Runtime]] — Boss Status의 원본
- [[12. Multiplayer Synchronization|12_Multiplayer_Synchronization]] — Client로 state가 전달되는 방식

---

## 관련 코드

- [SonheimPlayerController.cpp](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Player/SonheimPlayerController.cpp)
- [InventoryWidget.cpp](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/UI/Widget/Player/Inventory/InventoryWidget.cpp)
- [ConfirmWidget.h](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/UI/Widget/Player/Inventory/ConfirmWidget.h)
- [ContainerInteractionWidget.cpp](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/UI/Widget/GameObject/ContainerInteractionWidget.cpp)
- [CraftingWidget.cpp](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/UI/Widget/GameObject/Crafting/CraftingWidget.cpp)
- [NoticeSubsystem.cpp](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/UI/Notice/NoticeSubsystem.cpp)
- [NoticeWidget.h](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/UI/Notice/NoticeWidget.h)
- [DungeonStagePresenter.cpp](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/UI/Dungeon/DungeonStagePresenter.cpp)
- [DungeonUIRouterSubsystem.cpp](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/UI/Dungeon/DungeonUIRouterSubsystem.cpp)
- [DungeonUIRegistryDataAsset.h](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/UI/Dungeon/DungeonUIRegistryDataAsset.h)
