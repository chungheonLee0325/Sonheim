# 18. UMG v2 Migration & Editor Verification

UMG v2에서는 인벤토리·도감·퀘스트·던전·HUD의 화면을 개편하면서 **중첩 메뉴의 입력·커서 정책을 공통화하고, 알림을 용도별 슬롯으로 분리**했습니다. 기존 C++ Widget의 데이터·이벤트 연결을 유지한 채 Widget Blueprint를 수정하고, AgentMcp로 에디터 작업과 실행 화면 확인을 연결했습니다.

이 문서는 **실제 변경 결과와 이를 구현한 UI 공통 구조, 에디터 검증 과정**을 다룹니다. 게임플레이 상태·ViewData 구조는 [[11. UI Architecture & Client Presentation|11_Client_State_Presentation_Pipeline]], 도감·퀘스트 규칙은 [[16. MonsterDex System|16_MonsterDex_System]]과 [[17. Quest System|17_Quest_System]]에서 설명합니다.

## 대표 변경 결과

| Dungeon HUD — 이전 | Dungeon HUD — v2 |
|---|---|
| ![기존 던전 HUD](../Media/Wiki/16_UMG_Migration_with_Claude/16_dungeon_hud_before.jpg) | ![개편한 던전 HUD](../Media/Wiki/16_UMG_Migration_with_Claude/16_dungeon_hud_after.jpg) |

*던전 목표·진행 상태·HUD 정보를 새 레이아웃에 맞춰 재배치했습니다. 던전의 실제 진행 데이터는 기존 Snapshot·Presenter 흐름을 사용합니다.*

| 알림 — 이전 | Feed — v2 |
|---|---|
| ![기존 알림](../Media/Wiki/16_UMG_Migration_with_Claude/16_toast_before_feed.jpg) | ![v2 Feed에 제작 완료와 경고가 쌓이는 모습](../Media/Wiki/16_UMG_Migration_with_Claude/16_toast_after_feed_stack.jpg) |

*기존 알림을 좌측의 카드형 Feed로 개편했습니다. v2 Feed는 여러 알림을 동시에 표시하는 `Stack` 정책을 사용합니다.*

### 변경 사항 한눈에 보기

| 항목 | 적용 내용 |
|---|---|
| **중첩 메뉴 관리** | `UUIStackSubsystem`에서 Screen Stack과 입력 모드·커서·게임플레이 입력 정책 통합 |
| **알림 표시** | `UNoticeSubsystem`에 `Feed / Headline / Title` 슬롯과 대기·교체·누적 정책 구성 |
| **입력 안내** | `UInputHintSubsystem`이 Enhanced Input 매핑과 최근 사용 장치에 따라 키 안내 변경 |
| **Widget Blueprint** | C++ `BindWidget` 계약을 유지하며 인벤토리·도감·퀘스트·던전·HUD 화면 개편 |
| **공통 보상 슬롯** | `USlotWidget::AddReadOnly`로 도감·퀘스트 보상에 기존 아이템 표현 재사용 |
| **에디터 저작·확인** | AgentMcp에서 Widget Tree 점검 → 수정 → Read-back → Compile/Save → PIE 확인 |

## 목차

- [일반 메뉴의 입력·커서 정책 통합](#일반-메뉴의-입력커서-정책-통합)
- [알림 영역 분리](#알림-영역-분리)
- [Enhanced Input 기반 키 안내](#enhanced-input-기반-키-안내)
- [Widget Blueprint 화면 개편](#widget-blueprint-화면-개편)
- [AgentMcp로 저작·검증](#agentmcp로-저작검증)
- [적용 범위와 관련 코드](#적용-범위와-관련-코드)

---

## 일반 메뉴의 입력·커서 정책 통합

### 화면을 닫는 순서와 무관하게 입력 복원

인벤토리·보관함·제작 등 일반 메뉴의 입력·커서 설정을 `UUIStackSubsystem : ULocalPlayerSubsystem`으로 모았습니다. 각 화면은 `FUIStackEntry`로 관리하고, `Open / Close / CloseTop` 이후 **스택의 최상단 화면에 설정된 규칙**을 다시 적용합니다. 중첩 메뉴를 어느 순서로 닫더라도 남아 있는 화면의 입력 상태를 기준으로 복원하는 구조입니다.

~~~mermaid
flowchart TB
    OPEN["Open · Close · CloseTop"] --> STACK["UUIStackSubsystem<br/>화면 스택"]
    RULE["DefaultGame.ini<br/>FUIScreenRule"] --> STACK
    STACK --> TOP["최상단 화면 조회"]
    TOP --> APPLY["ApplyInputState"]
    APPLY --> CURSOR["커서 표시"]
    APPLY --> MODE["GameOnly / GameAndUI / UIOnly"]
    APPLY --> BLOCK["게임플레이 입력 차단"]
    APPLY --> IMC["메뉴 Input Mapping Context"]
~~~

`FUIScreenRule`은 화면 ID·ZOrder·입력 모드·커서·게임플레이 입력 차단 여부를 설정합니다. 현재 `DefaultGame.ini`에서는 Menu, Container, Crafting, MonsterDex, QuestDialog, QuestJournal에 `GameAndUI`와 `bBlockGameInput=True`를 사용합니다.

~~~cpp
// UIStackSubsystem.cpp — ApplyInputState 핵심 처리 발췌
PC->SetShowMouseCursor(Rule.bShowMouse);
if (UInputHintSubsystem* Hints = GetLocalPlayer()
        ? GetLocalPlayer()->GetSubsystem<UInputHintSubsystem>() : nullptr)
{
    Hints->SetMenuInput(Rule.bBlockGameInput);
}

switch (Rule.InputMode)
{
case EUIStackInputMode::UIOnly:
    PC->SetInputMode(FInputModeUIOnly());
    break;
case EUIStackInputMode::GameAndUI:
    PC->SetInputMode(FInputModeGameAndUI());
    break;
default:
    PC->SetInputMode(FInputModeGameOnly());
}

// SonheimPlayerController.cpp — 게임플레이 입력 측 정책 조회
bool ASonheimPlayerController::IsUIBlockingInput() const
{
    const UUIStackSubsystem* UIStack = GetUIStack();
    return UIStack && UIStack->IsGameplayBlocked();
}
~~~

`GameAndUI`는 UI와 게임 입력을 함께 다룰 수 있는 입력 모드이고, `bBlockGameInput`은 이동·공격·상호작용 등 게임플레이 입력에서 조회하는 **별도의 차단 정책**입니다. 일반 메뉴가 모두 닫히면 `GameOnly`와 커서 숨김으로 복원합니다. 실제 `GameAndUI` 분기에는 커서 캡처 시 표시를 유지하는 설정도 적용합니다.

화면 하나가 여러 위젯으로 구성될 수도 있습니다. 예를 들어 메뉴를 열 때 인벤토리와 능력치 위젯을 함께 생성하지만 **하나의 `FUIStackEntry`**로 등록해 함께 열고 닫습니다. 이미 열린 화면을 다시 요청하면 최상단으로 올리고, Viewport에서 제거된 위젯의 기록은 `Prune`으로 정리합니다.

**중첩 메뉴 종료 후 입력 복원 — 플레이 영상**

https://github.com/user-attachments/assets/f9936033-7251-4304-b1cb-842750c13c59

메뉴를 중첩해서 열고 닫을 때 최상단 화면의 입력 모드와 커서를 적용하고, 모든 메뉴가 닫히면 게임플레이 입력으로 복원하는 동작을 보여줍니다.

던전 HUD·결과 화면은 일반 메뉴 스택과 별도로 `UDungeonUIRouterSubsystem`에서 관리합니다. `HUD / Result`를 비모달 `GameOnly`로 표시해 메뉴 스택의 입력·커서 정책과 분리하고, 던전 상태에 따라 화면을 교체합니다.

---

## 알림 영역 분리

기존 알림 화면에서 **짧게 쌓이는 진행 알림, 중요한 달성 알림, 지역 이름**을 서로 다른 화면 영역으로 분리했습니다. 현재 `DefaultGame.ini`의 슬롯별 정책은 다음과 같습니다.

| 슬롯 | 화면 위치·대표 상황 | 표시 정책 |
|---|---|---|
| **Feed** | 좌측 — 퀘스트 수락·진행, 제작 완료, 경고 | `Stack` — 새 카드를 즉시 추가, 표시 수는 Feed 위젯에서 관리 |
| **Headline** | 중앙 — 레벨업, 몬스터 첫 발견 등 주요 달성 | `TakeTurns` — 하나씩 표시, 대기 최대 3건 |
| **Title** | 화면 상단 — 지역 진입·장소 이름 | `ReplaceShowing` — 최신 제목으로 교체 |

`UNoticeSubsystem::Push(Slot, Channel, Data)`는 메시지의 슬롯을 선택하고 해당 슬롯의 순서 정책을 적용합니다. `Feed`는 `UNoticeFeedWidget`의 행 목록에 카드를 추가하고, `Headline`은 표시 중인 카드가 끝난 뒤 대기 중인 카드를 꺼내며, `Title`은 새 지역 이름으로 표시를 교체합니다. 각 슬롯의 Widget Blueprint·스타일·ZOrder·출력 정책은 `FNoticeSlotSettings`로 설정합니다.

### 도감 첫 발견 장면으로 확인하는 세 알림 영역

| 도감 알림 — 이전 | 도감 첫 발견 — v2 |
|---|---|
| ![기존 도감 등록 알림](../Media/Wiki/16_UMG_Migration_with_Claude/16_toast_before_dex.jpg) | ![Feed·Headline·Title이 함께 표시되는 도감 첫 발견 화면](../Media/Wiki/16_UMG_Migration_with_Claude/16_toast_after_dex_title.jpg) |

*개편 후 화면에는 **좌측 Feed(퀘스트 수락)**, **중앙 Headline(새로운 팰 발견!)**, **상단 Title(지역명)**이 동시에 표시됩니다. 도감 첫 발견의 Headline과 퀘스트 수락의 Feed는 서로 다른 알림입니다.*

![실제 게임 화면의 Feed·Headline·Title 표시 영역](https://github.com/user-attachments/assets/65918dce-7c6b-4792-9b51-833113896b05)

*좌측 Feed, 중앙 Headline, 상단 Title의 배치 영역을 표시했습니다.*

### 레벨업 Headline

![레벨업 Headline 화면](../Media/Wiki/16_UMG_Migration_with_Claude/16_toast_after_levelup.jpg)

*레벨업은 중앙 Headline에서 능력치 변화와 스탯 포인트를 함께 표시합니다.*


### 지역명 Title

| 지역명 — 이전 | 지역명 Title — v2 |
|---|---|
| ![이전 지역명 표시](../Media/Wiki/16_UMG_Migration_with_Claude/16_toast_before_title.jpg) | ![v2 지역명 Title](../Media/Wiki/16_UMG_Migration_with_Claude/16_toast_after_title_blur.jpg) |

### 기능별 알림 수명 관리 — Channel

슬롯이 **어디에·어떤 순서로 표시할지** 결정한다면, `Channel`은 **어느 기능이 만든 알림인지** 구분합니다. 던전 UI는 지역 제목 등을 `Dungeon` 채널로 게시하고, 던전 종료 시 같은 채널의 표시 중인 카드와 대기 항목을 정리합니다.

~~~cpp
// DungeonUIRouterSubsystem.cpp — 채널 관리 코드 발췌
const FName UDungeonUIRouterSubsystem::NoticeChannel(TEXT("Dungeon"));

Notices->Push(ENoticeSlot::Title, NoticeChannel, Data);
// 던전 UI Detach 시
Notices->Clear(NoticeChannel);
~~~

`Clear`는 각 슬롯의 카드와 대기열 중 해당 채널에 속하는 항목만 제거합니다. 퀘스트 수락·레벨업처럼 다른 기능이 게시한 알림은 유지됩니다. 표시 데이터와 UI 연결 구조는 [[11. UI Architecture & Client Presentation|11_Client_State_Presentation_Pipeline]]에서 다룹니다.

---

## Enhanced Input 기반 키 안내

기존에 키 이름을 직접 써 넣는 대신, `UInputHintSubsystem`이 **실제 `UInputAction`에 연결된 키와 최근 사용 입력 장치**를 기준으로 글리프를 선택하도록 변경했습니다.

~~~text
Enhanced Input · InputAction
             +
최근 사용 장치 (키보드·마우스 / 게임패드)
             ↓
UInputHintSubsystem::GlyphFor
             ↓
WBP_KeyHint
~~~

| 키보드·마우스 입력 안내 | 게임패드 입력 안내 |
|---|---|
| ![키보드 입력 안내](../Media/Wiki/16_UMG_Migration_with_Claude/16_keyhint_keyboard.jpg) | ![게임패드 입력 안내](../Media/Wiki/16_UMG_Migration_with_Claude/16_keyhint_gamepad.jpg) |

`UInputHintSubsystem`은 Slate 입력으로 장치 변경을 감지하고, `ControlMappingsRebuiltDelegate`를 통해 Enhanced Input 매핑 변경을 반영합니다. 해당 장치의 키가 아이콘 세트에 있으면 아이콘을, 없으면 키 이름을 표시합니다.

`UUIStackSubsystem::ApplyInputState`가 `UInputHintSubsystem::SetMenuInput`을 호출해 메뉴 전용 `IMC_Menu`를 켜고 끕니다. 메뉴 탭은 키보드 Q/E 또는 게임패드 LB/RB로 전환할 수 있도록 구성했습니다.

---

## Widget Blueprint 화면 개편

기존 C++ `UUserWidget`에서 데이터 조회·Delegate·입력 처리를 담당하고, Widget Blueprint는 **배치·스타일·애니메이션**을 구성하는 방식으로 개편했습니다. 기존 화면은 속성과 레이아웃을 수정해 이벤트 연결을 유지하고, 새 화면이나 구성이 달라진 화면은 C++ `BindWidget` 계약에 맞춰 Widget Tree를 구성했습니다.

### C++ BindWidget과 Widget Tree 연결

도감은 **종 목록 → 선택 상세 → 달성 기록·보상**으로 화면을 나누고, C++에서 필요한 하위 위젯을 이름과 타입으로 연결했습니다. 실제 `MonsterDexWidget.h`에는 다음 멤버가 선언되어 있습니다.

~~~cpp
// MonsterDexWidget.h — 실제 BindWidget 선언 발췌
// UMonsterDexWidget
UPROPERTY(meta=(BindWidget)) TObjectPtr<UDynamicEntryBox> PageGrid;
UPROPERTY(meta=(BindWidget)) TObjectPtr<UDynamicEntryBox> TierBox;

// UMonsterDexTierWidget
UPROPERTY(meta=(BindWidget)) TObjectPtr<UProgressBar> ProgressBar;
UPROPERTY(meta=(BindWidget)) TObjectPtr<UDynamicEntryBox> RewardBox;
UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> ClaimButton;
~~~

`PageGrid / TierBox`는 종 목록과 보상 단계 목록을 구성하며, `ProgressBar / RewardBox / ClaimButton`은 보상 단계의 진행도·아이템·수령 입력에 대응합니다. 에디터에서는 Widget Tree의 해당 이름·타입을 확인하고, Compile/Save 후 실제 게임 화면에서 데이터 표시와 버튼 연결을 확인했습니다.

**WBP_MonsterDex — 종 목록·상세·기록·보상 영역**

![Unreal Editor WBP_MonsterDex의 Widget Tree](https://github.com/user-attachments/assets/7349d43c-daa7-4adf-9d85-2df49ba579f1)

**WBP_MonsterDexTier — 달성 진행도·보상 아이템·수령 버튼**

![Unreal Editor WBP_MonsterDexTier Widget Tree](https://github.com/user-attachments/assets/28149b30-c0df-4826-8905-e5ac27aaec24)

보상 아이템은 `USlotWidget::AddReadOnly`를 호출해 기존 인벤토리 슬롯의 아이콘·수량 표시를 사용합니다. 읽기 전용 슬롯에는 아이템 이동과 드래그 동작을 적용하지 않습니다. 같은 슬롯 표현을 도감과 퀘스트 UI에서 함께 사용합니다.

### 인벤토리·퀘스트 화면

| Inventory — 이전 | Inventory — v2 |
|---|---|
| ![기존 인벤토리](../Media/Wiki/16_UMG_Migration_with_Claude/16_inventory_before.jpg) | ![개편한 인벤토리](../Media/Wiki/16_UMG_Migration_with_Claude/16_inventory_after.jpg) |

인벤토리는 기존 슬롯의 아이템 표시·드래그 앤 드롭과 데이터 갱신 연결을 유지하면서 메뉴 배치와 스타일을 개편했습니다.

| Quest Journal — 이전 | Quest Journal — v2 |
|---|---|
| ![기존 퀘스트 일지](../Media/Wiki/16_UMG_Migration_with_Claude/16_journal_before.jpg) | ![개편한 퀘스트 일지](../Media/Wiki/16_UMG_Migration_with_Claude/16_journal_after.jpg) |

퀘스트 일지는 목록·상세·현재 목표·보상 영역을 재배치했습니다. 퀘스트 진행 데이터는 `QuestView`에서 구성하고, `UQuestScreenWidget` 계열이 화면에 반영합니다.

### 던전 결과·필드 HUD

| Dungeon Result — 이전 | Dungeon Result — v2 |
|---|---|
| ![기존 던전 결과](../Media/Wiki/16_UMG_Migration_with_Claude/16_dungeon_result_before.jpg) | ![개편한 던전 결과](../Media/Wiki/16_UMG_Migration_with_Claude/16_dungeon_result_after.jpg) |

던전 화면은 `UDungeonStagePresenter`의 ViewData를 `UDungeonUIRouterSubsystem`이 HUD·Result Widget에 전달하는 구성을 유지하며 레이아웃을 변경했습니다.

| Field HUD — 이전 | Field HUD — v2 |
|---|---|
| ![기존 필드 HUD](../Media/Wiki/16_UMG_Migration_with_Claude/16_hud_before.jpg) | ![개편한 필드 HUD](../Media/Wiki/16_UMG_Migration_with_Claude/16_hud_after.jpg) |

![공통 UI 아이콘](../Media/Wiki/16_UMG_Migration_with_Claude/16_icons.png)

*화면 개편에 사용한 공통 UI 아이콘 세트입니다.*

---

## AgentMcp로 저작·검증

UMG 개편에서는 Unreal Editor에 연결한 [AgentMcp](https://github.com/chungheonLee0325/AgentMcp)로 Widget Blueprint를 조회·수정했습니다. C++의 `BindWidget` 선언과 에디터 Widget Tree를 대조하고, **수정 후 Read-back → Compile/Save → PIE 확인**으로 실제 변경 결과를 확인했습니다.

~~~mermaid
flowchart LR
    A["기존 Widget / C++ BindWidget 확인"] --> B["Widget Tree·속성·스타일 수정"]
    B --> C["Editor Read-back"]
    C --> D["Compile / Save"]
    D --> E["PIE · 실제 화면 확인"]
    E --> F{"레이아웃·동작 확인"}
    F -->|"수정 필요"| B
    F -->|"확인"| G["캡처 · 문서 반영"]
~~~

| 적용 사례 | 확인한 결과 | 문서 내 근거 |
|---|---|---|
| **도감 화면** | `WBP_MonsterDex / WBP_MonsterDexTier`의 Widget Tree와 C++ `BindWidget` 이름·타입 대조 | 에디터 캡처·선언 코드 |
| **메뉴 전환** | 중첩 메뉴를 열고 닫은 후 입력 모드와 커서 상태 복원 | UI Stack 플레이 영상 |
| **알림 분리** | Feed·Headline·Title의 동시 배치 및 서로 다른 표시 형식 | 표시 영역 마킹 화면·전후 비교 |
| **화면 레이아웃** | 인벤토리·퀘스트·던전 HUD/Result의 게임 내 표시 확인 | 실제 게임 화면 전후 비교 |

에디터 구조를 읽는 단계부터 실제 플레이 화면을 확인하는 단계까지 이어서 작업하고, 그 결과를 각각 **C++ 계약·에디터 캡처·플레이 영상·전후 비교**로 남겼습니다. AgentMcp 도구 자체의 구현과 일반 검증 워크플로는 [[14. Unreal Editor Automation & Verification|14_Unreal_Editor_Automation_Verification]]에서 설명합니다.

---

## 적용 범위와 관련 코드

| 관리 기능 | 실제 담당 | 역할 |
|---|---|---|
| 일반 메뉴·입력 | `UUIStackSubsystem` | Screen Stack과 커서·입력 정책 |
| 던전 HUD·결과 | `UDungeonUIRouterSubsystem` | 비모달 던전 화면·비동기 Widget 수명 |
| 알림 출력 | `UNoticeSubsystem` | 슬롯별 표시 순서와 채널 정리 |
| 키 안내 | `UInputHintSubsystem` | 액션 매핑과 입력 장치별 글리프 |
| C++ / Blueprint 연결 | 각 `UUserWidget`의 `BindWidget` | 게임플레이 데이터·이벤트와 화면 구조 연결 |

- [UIStackSubsystem.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/UI/System/UIStackSubsystem.cpp) — 중첩 메뉴 입력 복원
- [InputHintSubsystem.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/UI/Input/InputHintSubsystem.cpp) — 입력 안내
- [NoticeSubsystem.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/UI/Notice/NoticeSubsystem.cpp) — 알림 슬롯과 표시 정책
- [DungeonUIRouterSubsystem.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/UI/Dungeon/DungeonUIRouterSubsystem.cpp) — 던전 HUD·결과 수명
- [SlotWidget.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/UI/Widget/Player/Inventory/SlotWidget.cpp) — 읽기 전용 아이템 보상 슬롯
- [MonsterDexWidget.h](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/UI/MonsterDex/MonsterDexWidget.h) · [QuestWidget.h](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/UI/Quest/QuestWidget.h) — `BindWidget` 화면 구성
- [DefaultGame.ini](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Config/DefaultGame.ini) — 화면·알림 슬롯 설정
- [[11. UI Architecture & Client Presentation|11_Client_State_Presentation_Pipeline]] — UI 데이터 흐름
- [[14. Unreal Editor Automation & Verification|14_Unreal_Editor_Automation_Verification]] — Editor 도구·검증
- [[16. MonsterDex System|16_MonsterDex_System]] · [[17. Quest System|17_Quest_System]] — 도감·퀘스트 구현
