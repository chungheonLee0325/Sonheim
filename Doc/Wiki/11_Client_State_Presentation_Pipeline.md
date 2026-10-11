# 11. UI Architecture & Client Presentation

Sonheim UI는 **Persistent HUD, Interaction Screen, Transient Notice, Complex Content UI**의 네 흐름으로 구성됩니다.

## 요약

- Persistent HUD는 Health/Stamina/Level 같은 독립 state를 Component Delegate로 구독합니다.
- Inventory/Container/Crafting은 Screen Widget이 state와 static definition을 조합하고, <code>UUIStackSubsystem</code>이 화면 전환과 입력 상태를 관리합니다.
- <code>UNoticeSubsystem</code>이 Feed / Headline / Title 슬롯별 queue 정책과 channel, style을 LocalPlayer 단위로 관리합니다.
- Dungeon UI는 Snapshot → Presenter → ViewData → UI Router로 복합 gameplay state를 화면 모델로 변환합니다.
- Dungeon UI Router가 async Widget load, state reconstruction, 기존 HUD visibility 등 dungeon UI lifecycle을 관리합니다.
- 도감·퀘스트의 기록과 진행은 서버 권위 <code>PlayerState</code> 컴포넌트가 소유하고, 소유 클라이언트로 FastArray 복제한 뒤 화면에 반영합니다.

## Runtime Demo

#
## Screen Stack & Input

여러 Screen이 각자 <code>SetInputMode</code>와 마우스 상태를 변경하면 종료 순서에 따라 입력 복원 상태가 달라질 수 있습니다. 현재 Inventory/Stat, Container, Crafting, MonsterDex, QuestDialog, QuestJournal은 <code>UUIStackSubsystem : ULocalPlayerSubsystem</code>을 사용합니다.

- <code>FUIScreenRule</code>은 화면 ID, Widget Class, ZOrder, InputMode, cursor 표시, 게임 입력 차단 정책을 정의합니다. 규칙은 <code>DefaultGame.ini</code>에서 설정합니다.
- <code>Open</code>은 화면을 생성하거나 기존 화면을 스택 맨 위로 올리고, <code>Close</code> / <code>CloseTop</code>은 제거 후 남은 최상위 화면의 입력 규칙을 다시 적용합니다.
- <code>FUIStackEntry::Widgets</code>로 인벤토리와 능력치처럼 함께 여닫는 여러 Widget을 하나의 Screen으로 다룹니다. 이미 Viewport에서 사라진 화면은 <code>Prune</code>으로 정리합니다.
- <code>ApplyInputState</code>는 <code>SetShowMouseCursor</code>, <code>SetInputMode</code>, 메뉴 전용 Enhanced Input Mapping Context 설정을 한곳에서 수행합니다. <code>IsGameplayBlocked</code>는 게임플레이 입력 분기에서 사용합니다.

메뉴 상단 탭은 <code>TopTab</code> / <code>StepTab</code>으로 유효한 화면을 순환합니다. 키보드 Q/E 및 패드 LB/RB 표시는 <code>UInputHintSubsystem</code>에서 액션 매핑을 읽습니다. Slate 입력을 통해 마지막 입력 장치를 추적하고, <code>ControlMappingsRebuiltDelegate</code>에서 글리프를 갱신합니다.

**경계:** 위젯 안의 방향키·스틱 기반 포커스 탐색은 이 PR의 구현 범위에 포함되지 않았습니다. Dungeon HUD/Result는 별도의 <code>UDungeonUIRouterSubsystem</code>을 유지합니다.

## MonsterDex & Quest Presentation

<code>ASonheimPlayerState::ReportDeed</code>는 Kill, Capture, Craft, Reach, ClearDungeon 이벤트를 서버에서 알립니다. <code>UMonsterDexComponent</code>와 <code>UQuestComponent</code>는 필요한 행동을 각각 집계합니다.

~~~mermaid
flowchart LR
    E["PlayerDeed<br/>Kill · Capture · Craft · Reach · ClearDungeon"]
    PS["PlayerState<br/>ReportDeed / OnDeed"]
    D["MonsterDexComponent<br/>FMonsterDexList"]
    Q["QuestComponent<br/>FQuestList"]
    C["Owner Client<br/>FastArray Delta"]
    V["Dex / Quest View<br/>Journal · Tracker · Notice"]
    E --> PS --> D --> C --> V
    PS --> Q --> C
~~~

- 도감은 종별 처치·포획 횟수와 수령한 보상 단계를 <code>FMonsterDexEntry</code>에 보관합니다. 첫 등록 시 Headline, 보상 조건 달성 시 Feed 알림을 보냅니다.
- 퀘스트는 <code>DT_Quest</code>의 단계·목표 정의를 기준으로 <code>FQuestRecord</code>의 현재 단계와 진행 수치를 갱신합니다. 수락·완료 보고는 서버에서 선행 조건과 NPC 이름/거리 등을 확인합니다.
- 두 기록은 <code>FFastArraySerializer</code>와 <code>COND_OwnerOnly</code>로 복제하며, 변경 이벤트를 구독하는 UI가 화면을 갱신합니다. 정의 데이터는 <code>UMonsterDexSubsystem</code> / <code>UQuestSubsystem</code>에서 읽고 호스트 저장소에 기록합니다.

도감의 3D 미리보기와 퀘스트 대화창·추적기, 보상 슬롯 표현 및 실제 캡처는 [16. 도감·퀘스트와 UMG v2 통합](16_UMG_Migration_with_Claude.md)에 정리했습니다.

---

## Notice / Toast

*아래 영상은 v2 개편 이전의 Banner queue 시연입니다. 현재 슬롯 구성은 [16. 도감·퀘스트와 UMG v2 통합](16_UMG_Migration_with_Claude.md)에서 확인할 수 있습니다.*


https://github.com/user-attachments/assets/bc9e4305-62f6-43bb-b1fa-2b109a06eb5b

*Lever 상호작용과 Wave 완료/Branch 변경에서 발생한 Dungeon Banner가 `ShortcutUnlocked → GroupClear → Branch.Shortcut` 순으로 표시되는 `TakeTurns` queue 시연.*

## 목차

- [Runtime Demo](#runtime-demo)
- [System Overview](#system-overview)
- [Base HUD & Interaction Screen](#base-hud--interaction-screen)
- [Screen Stack & Input](#screen-stack--input)
- [MonsterDex & Quest Presentation](#monsterdex--quest-presentation)
- [Notice / Toast](#notice--toast)
- [Complex Content UI — Dungeon](#complex-content-ui--dungeon)
- [UI Lifecycle & Reuse](#ui-lifecycle--reuse)
- [Trade-offs](#trade-offs)

---

## System Overview

~~~mermaid
flowchart TB
    subgraph SOURCE["Gameplay / State"]
        COMP["<b>독립 Gameplay State</b><br/>Health · Stamina · Inventory · Equipment"]
        WORLD["<b>상호작용 State</b><br/>Container · Crafting Station"]
        EVENT["<b>Transient Event</b><br/>Level · Capture · Crafting · Region"]
        SNAP["<b>복합 Content State</b><br/>FDungeonStageRuntimeState"]
    end

    subgraph PRESENT["Client Presentation"]
        CTRL["<b>Persistent / Screen Lifetime</b><br/>ASonheimPlayerController"]
        NOTICE["<b>Notice Queue</b><br/>UNoticeSubsystem : ULocalPlayerSubsystem"]
        PRESENTER["<b>Content Adapter</b><br/>UDungeonStagePresenter"]
        ROUTER["<b>Content UI Routing</b><br/>UDungeonUIRouterSubsystem"]
    end

    subgraph UMG["UMG"]
        HUD["<b>Persistent HUD</b><br/>Player Status"]
        SCREEN["<b>Interaction Screen</b><br/>Inventory · Container · Crafting"]
        POPUP["<b>Local Popup</b><br/>UConfirmWidget"]
        TOAST["<b>Notice / Toast</b><br/>Banner · Title"]
        CONTENT["<b>Complex Content UI</b><br/>Dungeon HUD · Minimap · Result"]
    end

    COMP --> CTRL --> HUD
    COMP --> SCREEN
    WORLD --> SCREEN
    SCREEN --> POPUP
    EVENT --> NOTICE --> TOAST
    SNAP --> PRESENTER --> ROUTER --> CONTENT
~~~

| UI 종류 | 상태/수명 관리 | 주요 데이터 source |
|---|---|---|
| Player HUD | PlayerController + Widget | Component / PlayerState Delegate |
| Inventory / Stat | UUIStackSubsystem + Inventory Widget | Inventory / PlayerState |
| Container | UUIStackSubsystem + ContainerInteractionWidget | Player Inventory + Container |
| Crafting | UUIStackSubsystem + CraftingStation / CraftingWidget | Recipe + Inventory + ActiveWork |
| Confirm Popup | InventoryWidget (local context) | ItemID / Count / Action mode |
| Notice / Toast | UNoticeSubsystem | FNoticeData + Slot / Channel |
| Dungeon HUD / Result | Presenter + UI Router | Run Snapshot + Presentation Data + 기존 Component/World State |

---

## Base HUD & Interaction Screen

### C++ Widget Contract와 Widget Blueprint

C++ Widget class는 **데이터 연결·입력 처리·BindWidget 계약**을 담당하고, Widget Blueprint는 **layout·style·animation**을 구성합니다.

예를 들어 <code>UInventoryWidget</code>은:

- <code>BindWidget</code>으로 SlotGrid / Equipment Slot / Trash Zone 계약 정의
- Inventory / Equipment delegate 처리
- Drag & Drop 요청
- ItemID → ItemData 변환

을 담당합니다.

~~~text
C++ Widget
 ├─ BindWidget Contract
 ├─ Data Binding
 └─ Input / Request
        ↓
Widget Blueprint
 ├─ Layout
 ├─ Art / Style
 └─ Animation
~~~

AgentMcp의 UMG toolset도 이 계약을 기준으로 C++ <code>BindWidget</code>과 Widget Tree를 함께 검사하며 Widget Blueprint를 생성·수정합니다.

### Persistent HUD

Health / Stamina / Level처럼 owner와 lifecycle이 명확한 값은 component delegate를 직접 구독합니다.

~~~text
HealthComponent.OnHealthChanged ─┐
StaminaComponent.OnChanged      ├─→ UPlayerStatusWidget
LevelComponent.OnLevelChanged   ┘
~~~

HUD는 gameplay object를 Tick으로 polling하지 않고 상태 owner가 발생시키는 변경 event를 반영합니다.

### Inventory

PlayerController가 Inventory Screen을 열고 <code>UInventoryWidget</code>에 <code>UInventoryComponent</code>를 연결합니다.

~~~mermaid
flowchart LR
    INV["<b>Runtime State</b><br/>UInventoryComponent"]
    EVENT["<b>Change Event</b><br/>Inventory / Equipment Delegate"]
    DATA["<b>Static Definition</b><br/>FItemData"]
    WIDGET["<b>Screen Logic</b><br/>UInventoryWidget"]
    SLOT["<b>Reusable Item View</b><br/>USlotWidget"]

    INV --> EVENT --> WIDGET
    DATA --> WIDGET
    WIDGET --> SLOT
~~~

Widget은:

1. Inventory의 ItemID / Count를 읽고
2. GameInstance의 <code>FItemData</code>에서 이름·아이콘·rarity 같은 정적 정보를 조회한 뒤
3. <code>USlotWidget</code>에 화면용 값을 전달합니다.

원본 Item state는 <code>UInventoryComponent</code>가 유지합니다.

### Container

<code>UContainerInteractionWidget</code>은 Player Inventory와 Container Inventory를 한 화면에 배치합니다.

~~~text
ContainerInteractionWidget
 ├─ PlayerInventoryWidget
 └─ ContainerWidget
~~~

Player Inventory의 Slot/Drag & Drop 구조를 Container 화면에서도 재사용하고, 양쪽 state 사이의 이동 요청만 추가합니다.

### Crafting

Crafting UI는 세 source를 결합합니다.

~~~text
Recipe DataTable
 + Player Inventory
 + Crafting Station ActiveWork
        ↓
UCraftingWidget / CraftingQueueWidget
~~~

Recipe가 바뀔 때는 결과 Item·Icon·Material Row 같은 구조를 갱신하고, Inventory 수량이나 Work progress가 바뀔 때는 필요한 값만 갱신합니다.

Required Material Row는 부모 Widget이 pool을 소유해 필요한 수만 활성화하고 나머지는 <code>Collapsed</code> 상태로 재사용합니다.

세부 Item/Crafting 흐름은 [[07. Inventory & Crafting|07_Inventory_Crafting]]에서 설명합니다.

### Local Confirm Popup

Item Drop / Discard처럼 추가 확인이 필요한 동작은 <code>UInventoryWidget</code>이 <code>UConfirmWidget</code>을 생성해 처리합니다.

~~~text
Drop / Discard Request
      ↓
UInventoryWidget
      ↓
UConfirmWidget
  ItemID · MaxCount · Mode
      ↓ OnConfirm
UInventoryWidget
      ↓
ServerDrop / ServerDiscard
~~~

Confirm Widget은 gameplay mutation을 직접 수행하지 않고 결과를 부모 화면으로 전달합니다.

Inventory/Container/Crafting 화면은 <code>UUIStackSubsystem</code>을 통해 열고 닫으며, 최상위 화면의 규칙이 cursor / input mode / gameplay 차단을 결정합니다. Confirm Popup은 Inventory 안의 로컬 확인 흐름으로 남습니다.

---

## Notice / Toast

Level Up, Capture 결과, Crafting 완료, Region Title, Dungeon 진행처럼 **현재 상태를 저장할 필요 없이 짧게 전달하는 정보**는 <code>UNoticeSubsystem</code>이 관리합니다.

<code>UNoticeSubsystem : ULocalPlayerSubsystem</code>은 LocalPlayer 수명에 맞춰 생성되며 gameplay producer는 <code>FNoticeData</code>만 전달합니다.

~~~text
Gameplay Producer
  Level / Capture / Crafting / Region / Dungeon
        ↓
FNoticeData
        ↓
Slot + Channel
        ↓
UNoticeSubsystem
        ↓
UNoticeWidget
~~~

### Notice Data와 Style

<code>FNoticeData</code>는 다음 정보를 담습니다.

- Category
- Title
- Detail
- Symbol / Icon
- Tone

실제 Widget class와 style, queue 크기, ZOrder는 Slot 설정이 결정합니다.

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

### Slot과 Queue Policy

슬롯은 **메시지가 차지할 화면 영역**과 **동시 표시 정책**을 분리합니다. <code>DefaultGame.ini</code>의 현재 설정은 다음과 같습니다.

| Slot | 역할 | Order | 최대 대기 |
|---|---|---|---|
| <code>Feed</code> | 퀘스트·제작·경고·던전 진행을 좌측 스택으로 표시 | <code>Stack</code> | 별도 대기열 없음 |
| <code>Headline</code> | 레벨업·첫 포획 등 중앙 주요 알림 | <code>TakeTurns</code> | 3 |
| <code>Title</code> | 지역명·공간 진입을 상단에 표시 | <code>ReplaceShowing</code> | 별도 대기열 없음 |

<code>Stack</code>은 위젯에 즉시 추가하며, <code>TakeTurns</code>는 순서대로 보여 주고, <code>ReplaceShowing</code>은 표시 중인 알림을 교체합니다. 제작·도감·퀘스트 등의 생산자는 <code>FNoticeData</code>를 만들어 Slot과 Channel을 지정할 뿐 위젯 배치를 소유하지 않습니다.

### Channel

<code>Channel</code>은 같은 슬롯 안에서 생산자별 알림을 구분합니다. 예를 들어 Dungeon 종료 시:

~~~cpp
Notices->Clear(TEXT("Dungeon"));
~~~

을 호출하면 해당 Channel에 속한 표시·대기 알림만 정리할 수 있습니다. 다른 시스템의 알림은 유지됩니다.

### Notice Widget

<code>UNoticeWidget</code>은 gameplay component를 직접 참조하지 않고 전달받은 ViewData와 <code>UNoticeStyle</code>로:

- fade in
- hold
- fade out
- accent / icon
- lifetime progress

를 표현합니다.

Item 직접 획득 feedback은 기존 Player HUD의 별도 popup 경로도 사용합니다.

~~~text
Inventory.OnItemAdded
      ↓
PlayerController Client RPC
      ↓
PlayerStatusWidget.DisplayItemPopup
~~~

<code>OnInventoryChanged</code>와 <code>OnItemAdded</code>가 분리되어 있어 Slot Swap이나 장비 해제가 Item 획득 popup으로 표시되지 않습니다.

---

## Complex Content UI — Dungeon

Dungeon UI는 Stage, Objective, Party, Boss, Minimap, Reward 등 여러 gameplay state를 한 화면에서 조합합니다.

- Stage / Main & Optional Objective
- Branch / Route
- Party
- Timer
- Boss
- Minimap / Marker
- Reward / Result

그래서 Runtime Snapshot을 Widget에 직접 넘기지 않고 다음 계층을 사용합니다.

~~~mermaid
flowchart LR
    R["<b>Server Content State</b><br/>UDungeonStageRuntimeSubsystem"]
    G["<b>Replicated Snapshot</b><br/>ASonheimGameState<br/>FDungeonStageRuntimeState"]
    P["<b>Client Adapter</b><br/>UDungeonStagePresenter"]
    D["<b>Presentation Definition</b><br/>UDungeonPresentationDataAsset"]
    V["<b>View Model</b><br/>FDungeonStageViewData"]
    U["<b>Widget Lifecycle</b><br/>UDungeonUIRouterSubsystem"]
    W["<b>UMG</b><br/>HUD · Minimap · Result"]

    R --> G --> P
    D --> P
    P --> V --> U --> W
~~~

![Dungeon HUD](../Media/Wiki/11_Client_State_Presentation_Pipeline/11_dungeon_hud.png)

위 화면은 같은 Run의 **Stage / Main·Optional Objective / Timer / Party / World Marker / Minimap**을 한 Client 화면에 조합한 개편 전 캡처입니다. v2 화면은 [Wiki 16](16_UMG_Migration_with_Claude.md)에 있습니다. Dungeon progression은 Snapshot에서, Party HP는 기존 replicated `HealthComponent`에서, text·icon·room 영역은 Presentation Asset에서 가져옵니다.

### Presenter

Presenter는 Runtime ID와 Presentation Data를 결합합니다.

~~~text
StageId
= Dungeon.ForgottenRuins.Stage.GuardRoom

Objective Group
= Dungeon.ForgottenRuins.Group.Guards

Runtime Count
= 2 / 4
        ↓
Presenter
        ↓
"경비실"
"경비병을 처치하세요"
2 / 4
Room Icon
World Marker
~~~

Widget은 GameplayTag, ItemID, Stage transition을 해석하지 않고 화면용 ViewData를 소비합니다.

### Presentation Definition

Runtime Snapshot에는 화면 문구나 Map texture를 직접 넣지 않습니다. `UDungeonPresentationDataAsset`이 Stage title/objective/icon, room 영역, marker와 전체 map bounds 같은 presentation 정의를 소유합니다.

![Dungeon Presentation DataAsset](../Media/Wiki/11_Client_State_Presentation_Pipeline/11_dungeon_presentation_asset.png)

Presenter는 Snapshot의 `StageId`와 Objective 상태를 이 정의에 결합해 ViewData를 만듭니다. 같은 gameplay state를 유지하면서도 화면 표현을 asset에서 조정할 수 있도록 분리한 구조입니다.

### Snapshot Revision

같은 Run의 오래된 Snapshot이 화면을 되돌리지 않도록 <code>Revision</code>을 비교합니다.

~~~cpp
if (Snapshot.RunId == Latest.RunId &&
    Snapshot.Revision <= Latest.Revision)
{
    return;
}
~~~

새 Run에서는 새로운 기준으로 시작하고, 같은 Run에서는 더 높은 Revision만 반영합니다.

### 기존 Component State 병행

Party HP처럼 이미 독립적으로 복제되는 값은 Dungeon Snapshot에 중복 저장하지 않습니다.

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

Dungeon progression은 Snapshot, Health는 기존 Component/Delegate 경로를 사용합니다.

### Minimap / Marker

Minimap은 **Presentation 정의와 현재 world state를 함께 사용**합니다.

Presentation Data는:

- Map Texture
- World Bounds
- Stage Room Rect
- Marker icon / kind

을 정의하고, Presenter는 Current Stage와 열린 Objective의 Marker target을 ViewData로 구성합니다. Local/remote Player 위치와 local camera 방향처럼 매 frame 변하는 값은 Minimap Widget이 실제 Pawn/PlayerState에서 읽어 map 좌표로 투영합니다.

![Multiplayer Minimap](../Media/Wiki/11_Client_State_Presentation_Pipeline/11_minimap_multiplayer.png)

위 화면에서는 Current Stage room highlight, Local Player arrow, Remote Player dot, Lever Objective marker와 room icon이 동시에 표시됩니다. Bounds 밖 Player는 edge로 clamp하지 않고 표시 대상에서 제외합니다.

https://github.com/user-attachments/assets/fa7c4755-16dd-47fb-a558-80b74aa536b2

영상에서는 remote Player의 Dungeon 밖 이동/복귀, 두 Player의 위치 변화와 local camera 회전에 따른 arrow 방향 갱신을 확인할 수 있습니다.

### Server Time 기반 Progress

Stage countdown과 Boss action progress는 Snapshot의:

- RunStartedServerTime
- StageDeadlineServerTime
- BossActionStartServerTime
- BossActionEndServerTime

을 synchronized server clock과 비교해 계산합니다.

HUD가 늦게 생성되거나 재생성돼도 같은 현재 progress를 복원합니다.

### Widget 재생성과 상태 복구

Widget 자체는 Dungeon Run의 상태 owner가 아닙니다. 진행 중 HUD를 제거해도 Presenter와 Snapshot은 유지되고, 이후 UI 갱신이 발생하면 Router가 **새 Widget instance**를 만들어 Latest ViewData를 적용합니다.

https://github.com/user-attachments/assets/6c4f9601-8fbe-4957-87d4-d4ad230f3211

촬영에서는 기존 HUD를 `RemoveFromParent`한 뒤 새 instance가 생성됐고, 같은 RunId / Stage / Objective 2/3 / Optional Objective / Minimap 상태를 다시 구성했습니다. Countdown과 elapsed time도 Widget 생성 시점에서 다시 시작하지 않고 기존 `StageDeadlineServerTime` / `RunStartedServerTime`과 synchronized server clock을 계속 사용했습니다.

### Boss Status

Boss FSM 자체를 Widget이 직접 참조하지 않고, Snapshot에 반영된 **Action / Phase / Break / Capture 가능 상태**를 Presenter가 화면용 정보로 변환합니다.

![Boss Status HUD](../Media/Wiki/10_Boss_Encounter_Runtime/10_boss_status_hud.png)

위 화면에서는 Phase 2, Break 상태, Exhaust 상태와 Capture 안내가 하나의 HUD에서 함께 표현됩니다. 같은 Runtime state를 Boss HUD와 Dungeon Objective가 각각 필요한 형태로 소비합니다.

### Result

Run 종료 시 최종 Snapshot의:

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
→ Result
~~~

진행 HUD와 Result는 같은 Run State의 서로 다른 presentation입니다.

![Dungeon Result](../Media/Wiki/11_Client_State_Presentation_Pipeline/11_dungeon_result.png)

위 결과 화면은 진행 HUD와 **같은 Run**의 terminal Snapshot을 사용합니다. 실제 최종 state의 elapsed time, defeated/captured count, 선택 branch, reward, grade와 기존 best record를 Result ViewData로 변환해 표시합니다.

---

## UI Lifecycle & Reuse

### LocalPlayer UI Router

<code>UDungeonUIRouterSubsystem : ULocalPlayerSubsystem</code>은:

- HUD / Result Widget 선택
- Widget class async load
- 최신 ViewData cache
- 현재 Widget 교체
- Dungeon 중 기존 HUD hide / restore
- Dungeon Notice channel cleanup

을 담당합니다.

Async load 완료 시점이 현재 UI lifecycle과 달라질 수 있으므로 <code>Generation</code> 값을 사용합니다.

~~~text
Attach : Generation N
   ↓ Async Load

Detach / Reattach : Generation N+1
   ↓

Old Callback
Expected N != Current N+1
→ Ignore
~~~

### 기존 HUD Visibility 복원

Dungeon Run 동안 숨긴 Widget은 원래 <code>ESlateVisibility</code> 값을 저장합니다.

Run 종료 시 일괄 <code>Visible</code>로 설정하는 대신 저장한 상태를 복원합니다.

Run 도중 새로 생성된 대상 Widget도 update 시 다시 확인해 숨깁니다.

### Widget 재사용

- **USlotWidget** — Inventory / Equipment / Container / Crafting Item 표현
- **Crafting Material Row** — 부모 Widget의 local pool
- **Floating Damage** — 전투 중 반복 생성량이 큰 global actor pool

각 UI의 생성 빈도와 수명에 맞춰 재사용 단위를 다르게 적용합니다.

### 현재 Screen / Popup Input 관리

현재 구현에서 Screen/Popup의 입력 및 수명 관리는 다음과 같이 나뉩니다.

| 화면 | 관리 위치 |
|---|---|
| Inventory / Stat | UIStackSubsystem (입력) + PlayerController / InventoryWidget (데이터) |
| Container | UIStackSubsystem (입력) + Container interaction screen (콘텐츠) |
| Crafting | UIStackSubsystem (입력) + Crafting screen / Station (콘텐츠) |
| Confirm Popup | InventoryWidget (로컬 Popup) |
| MonsterDex / QuestDialog / QuestJournal | UIStackSubsystem (입력), 각 UI View (콘텐츠) |
| Dungeon HUD / Result | Dungeon UI Router / Registry |

UIStackSubsystem과 Dungeon UI Router는 서로 다른 수명 정책을 담당합니다. Dungeon Registry는 <code>Screen / Modal / HUD</code> layer와 <code>GameOnly / GameAndUI / UIOnly</code> input policy를 표현할 수 있고, 현재 Dungeon HUD/Result는 gameplay input과 함께 동작하도록 GameOnly 경로를 사용합니다.

---

## Trade-offs

| 구조 | 적용 위치 | 이유 |
|---|---|---|
| **Delegate Binding** | HP / Stamina / Inventory | owner가 명확한 독립 state 변경 |
| **C++ Widget + WBP** | 기본 Screen / HUD | 데이터 계약과 visual authoring 분리 |
| **LocalPlayer NoticeSubsystem** | Feed / Headline / Title | transient event의 slot별 order / channel / style 관리 |
| **LocalPlayer UIStackSubsystem** | Inventory / Container / Crafting / Dex / Quest | screen stack 최상위 항목에서 cursor / input mode / 게임 입력 차단 결정 |
| **Presenter → ViewData** | Dungeon | Snapshot·Presentation 정의·기존 Component state를 하나의 화면 모델로 조합 |
| **LocalPlayer UI Router** | Dungeon HUD / Result | async widget load와 content UI lifecycle 관리 |
| **Local Popup Ownership** | Inventory Confirm | 부모 screen context 안에서 확인/결과 반환 |

---

## 연관 문서

- [[02. Gameplay Architecture|02_Gameplay_Architecture]] — Gameplay / Presentation 책임 경계
- [[06. World Interaction Systems|06_World_Interaction_Systems]] — Context Prompt와 Interaction UI
- [[07. Inventory & Crafting|07_Inventory_Crafting]] — Inventory/Container/Crafting의 상태와 UI 흐름
- [[09. Branching Dungeon Runtime|09_Branching_Dungeon_Runtime]] — Dungeon Snapshot의 원본 Runtime
- [[10. Boss Encounter Runtime|10_Boss_Encounter_Runtime]] — Boss Status의 원본
- [[12. Multiplayer Synchronization|12_Multiplayer_Synchronization]] — Client state 동기화
- [[16. 도감·퀘스트와 UMG v2 통합|16_UMG_Migration_with_Claude]] — 최신 화면, FastArray 기반 기록, Editor 저작·검증 적용 사례

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

- [UIStackSubsystem.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/UI/System/UIStackSubsystem.cpp)
- [InputHintSubsystem.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/UI/Input/InputHintSubsystem.cpp)
- [QuestComponent.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/AreaObject/Player/Utility/QuestComponent.cpp)
- [MonsterDexComponent.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/AreaObject/Player/Utility/MonsterDexComponent.cpp)
