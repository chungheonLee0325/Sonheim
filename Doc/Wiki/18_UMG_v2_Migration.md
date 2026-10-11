# 18. UMG v2 Migration & Editor Verification

Sonheim의 기존 gameplay state를 유지하면서 **화면 입력 수명·알림 정책·키 안내를 공통화하고 UMG를 v2로 개편**했습니다. UI 구현에서 C++ `BindWidget` 계약을 유지하고, AgentMcp로 Widget Blueprint를 저작·검증한 적용 사례입니다.

도감의 기록·보상 규칙은 [[16. MonsterDex System|16_MonsterDex_System]], 퀘스트 단계·NPC 상호작용은 [[17. Quest System|17_Quest_System]]에서 다룹니다. 이 문서는 **Client Presentation과 Editor 작업 흐름**에 집중합니다.

## 요약

- `UUIStackSubsystem`: Screen Stack 최상위 항목으로 cursor, `SetInputMode`, 게임 입력 차단, 메뉴 `IMC` 관리.
- `UNoticeSubsystem`: `Feed / Headline / Title` 슬롯과 `Stack / TakeTurns / ReplaceShowing` 표시 정책.
- `UInputHintSubsystem`: Enhanced Input의 액션 매핑과 마지막 입력 장치로 키보드·게임패드 안내를 전환.
- `USlotWidget::AddReadOnly`: 도감·퀘스트·던전 보상의 공용 아이템 표시.
- AgentMcp: `BindWidget` 확인 → Widget Tree 수정 → Compile/Save → PIE → 화면 캡처와 검증.

## 대표 화면

| Dungeon HUD v1 | Dungeon HUD v2 |
|---|---|
| ![기존 던전 HUD](../Media/Wiki/16_UMG_Migration_with_Claude/16_dungeon_hud_before.jpg) | ![v2 던전 HUD](../Media/Wiki/16_UMG_Migration_with_Claude/16_dungeon_hud_after.jpg) |

| 기존 알림 | Feed v2 |
|---|---|
| ![기존 알림](../Media/Wiki/16_UMG_Migration_with_Claude/16_toast_before_feed.jpg) | ![Feed v2](../Media/Wiki/16_UMG_Migration_with_Claude/16_toast_after_feed_stack.jpg) |

## 목차

- [화면 입력 상태 통합](#화면-입력-상태-통합)
- [Notice와 키 안내](#notice와-키-안내)
- [Widget Blueprint 개편](#widget-blueprint-개편)
- [AgentMcp 저작·검증](#agentmcp-저작검증)
- [입력·알림 관리의 경계](#입력알림-관리의-경계)
- [관련 코드](#관련-코드)

---

## 화면 입력 상태 통합

Inventory/Stat, Container, Crafting, MonsterDex, QuestDialog, QuestJournal이 독립적으로 입력 모드와 커서를 변경하면 중첩 화면을 닫는 순서에 따라 복원 상태가 달라질 수 있습니다. `UUIStackSubsystem : ULocalPlayerSubsystem`이 일반 Screen의 stack과 입력 정책을 맡도록 변경했습니다.

~~~mermaid
flowchart TB
  A["Open / Close / CloseTop"] --> B["UUIStackSubsystem<br/>FUIStackEntry"]
  C["DefaultGame.ini<br/>FUIScreenRule"] --> B
  B --> D["ApplyInputState"]
  D --> E["Cursor / InputMode"]
  D --> F["Gameplay Block"]
  D --> G["IMC_Menu On/Off"]
  B --> H["RefreshTabBar"]
~~~

`FUIScreenRule`은 Screen ID와 Widget Class, ZOrder, `GameOnly / GameAndUI / UIOnly`, 커서, 게임플레이 입력 차단 여부를 정의합니다. `Open`은 동일 화면을 상단으로 이동할 수 있으며, 여러 Widget으로 구성된 인벤토리/능력치를 하나의 Entry로 여닫습니다.

`Close` 후 남은 **상단 Screen**을 기준으로 `ApplyInputState`를 다시 호출합니다. Stack이 비면 GameOnly/커서 숨김으로 돌아갑니다. `Prune`은 Viewport에서 이탈한 위젯을 정리합니다.

<details>
<summary>FUIScreenRule / ApplyInputState의 주요 입력 상태 처리 보기</summary>

~~~cpp
// UIStackSubsystem.h
struct FUIScreenRule
{
    FName Id;
    TSoftClassPtr<UUserWidget> WidgetClass;
    int32 ZOrder = 0;
    EUIStackInputMode InputMode = EUIStackInputMode::GameAndUI;
    bool bShowMouse = true;
    bool bBlockGameInput = true;
};

// UIStackSubsystem.cpp — ApplyInputState 내부 발췌
PC->SetShowMouseCursor(Rule.bShowMouse);
Hints->SetMenuInput(Rule.bBlockGameInput);
~~~

</details>

Dungeon HUD/Result의 비동기 위젯 수명은 기존 `UDungeonUIRouterSubsystem`이 담당하며 일반 Screen Stack과 분리됩니다.

## Notice와 키 안내

### Notice: 메시지 역할에 따른 슬롯 분리

`UNoticeSubsystem`은 동일 `FNoticeData`를 받더라도 메시지의 종류에 따라 다른 위젯 및 큐 정책으로 보여줍니다. 현재 `DefaultGame.ini` 설정은 다음과 같습니다.

| Slot | 용도 | 표시 정책 |
|---|---|---|
| Feed | 퀘스트·제작·경고·보상 진행 | `Stack` · 좌측 카드 |
| Headline | 레벨업·첫 포획·보스 처치 | `TakeTurns` · 중앙 단독 알림 |
| Title | 지역명·장소 진입 | `ReplaceShowing` · 상단 최신 제목 |

`Channel`로 생산자를 구분해 Dungeon 종료 시 Dungeon이 만든 대기/표시 알림만 회수합니다. 표시 위치·스타일·모션은 각 `UNoticeWidget` Blueprint에서 소유합니다. 기존 단일 스타일로 여러 메시지를 그리던 때보다 **알림의 우선도와 표시 영역이 명확**해졌습니다.

| 이전 도감 등록 | 현재 Headline |
|---|---|
| ![도감 알림 전](../Media/Wiki/16_UMG_Migration_with_Claude/16_toast_before_dex.jpg) | ![도감 알림 후](../Media/Wiki/16_UMG_Migration_with_Claude/16_toast_after_dex_title.jpg) |

![레벨업 Headline](../Media/Wiki/16_UMG_Migration_with_Claude/16_toast_after_levelup.jpg)

| 지역명 이전 | 현재 Title |
|---|---|
| ![지역명 이전](../Media/Wiki/16_UMG_Migration_with_Claude/16_toast_before_title.jpg) | ![지역명 이후](../Media/Wiki/16_UMG_Migration_with_Claude/16_toast_after_title_blur.jpg) |

### Enhanced Input 기반 Key Hint

`UInputHintSubsystem`은 하드코딩한 Q/E/F 대신 실제 InputAction 매핑을 읽어 `WBP_KeyHint`가 표시하도록 합니다. Slate 입력 프로세서가 마지막 입력 장치를 감지하고, `ControlMappingsRebuiltDelegate`가 반영되면 다시 글리프를 선택합니다. `IMC_Menu`는 화면에서 게임플레이 입력을 차단할 때 활성화됩니다.

| 키보드 | 게임패드 |
|---|---|
| ![키보드 안내](../Media/Wiki/16_UMG_Migration_with_Claude/16_keyhint_keyboard.jpg) | ![패드 안내](../Media/Wiki/16_UMG_Migration_with_Claude/16_keyhint_gamepad.jpg) |

Q/E와 LB/RB를 통한 **메뉴 탭 전환은 구현**했지만, 메뉴 내부 슬롯·버튼 포커스 탐색은 후속 작업입니다.

## Widget Blueprint 개편

C++ Widget은 `BindWidget`의 이름·타입 계약과 데이터/입력 처리를 맡고, Widget Blueprint는 시각적 트리·레이아웃·애니메이션을 구성합니다.

- 기존 이벤트 그래프·애니메이션이 연결된 `BP_Slot`, 필드 HUD는 **위젯 트리 전체 재생성 대신 제자리 속성 수정**을 사용했습니다.
- 인벤토리·도감·퀘스트·던전 일부 화면은 `BindWidget`을 확인하면서 Widget Tree를 다시 구성했습니다.
- 도감·퀘스트·던전 보상은 텍스트 칩을 줄이고 `USlotWidget::AddReadOnly`로 아이템 아이콘·수량·희귀도·툴팁을 재사용했습니다.
- 던전 선택 목표의 보상은 UI 문구가 아닌 Dungeon Definition의 보상 정보를 조회합니다.
- Pretendard / Black Han Sans, SVG 기반 UI 아이콘 27종, StringTable 문구 51개를 적용·정리했습니다.

<details>
<summary>Inventory · Quest Journal · Dungeon Result · Field HUD 전후 비교 보기</summary>

| Inventory v1 | Inventory v2 |
|---|---|
| ![인벤토리 이전](../Media/Wiki/16_UMG_Migration_with_Claude/16_inventory_before.jpg) | ![인벤토리 개편](../Media/Wiki/16_UMG_Migration_with_Claude/16_inventory_after.jpg) |

| Quest Journal v1 | Quest Journal v2 |
|---|---|
| ![퀘스트 UI 이전](../Media/Wiki/16_UMG_Migration_with_Claude/16_journal_before.jpg) | ![퀘스트 UI 이후](../Media/Wiki/16_UMG_Migration_with_Claude/16_journal_after.jpg) |

| Dungeon Result v1 | Dungeon Result v2 |
|---|---|
| ![던전 결과 전](../Media/Wiki/16_UMG_Migration_with_Claude/16_dungeon_result_before.jpg) | ![던전 결과 후](../Media/Wiki/16_UMG_Migration_with_Claude/16_dungeon_result_after.jpg) |

| Field HUD v1 | Field HUD v2 |
|---|---|
| ![필드 HUD 전](../Media/Wiki/16_UMG_Migration_with_Claude/16_hud_before.jpg) | ![필드 HUD 후](../Media/Wiki/16_UMG_Migration_with_Claude/16_hud_after.jpg) |

![UI Icon Set 27종](../Media/Wiki/16_UMG_Migration_with_Claude/16_icons.png)

</details>

## AgentMcp 저작·검증

사용자가 화면 레퍼런스와 기능 방향을 정하고, Claude Code와 AgentMcp를 통해 Widget Tree/Blueprint를 수정·검증한 작업입니다. AgentMcp 플러그인의 내부 도구 구현은 [[14. Unreal Editor Automation & Verification|14_Unreal_Editor_Automation_Verification]]에서 다룹니다.

~~~mermaid
flowchart LR
  R["UI Reference"] --> S["Style / Widget Kit"]
  S --> A["Python Authoring"]
  A --> M["AgentMcp<br/>Widget Blueprint"]
  M --> B["BindWidget Read-back<br/>Compile / Save"]
  B --> P["PIE<br/>실제 Gameplay 경로"]
  P --> V["Viewport / Verification"]
  V -. "수정" .-> A
~~~

- **저작:** 색/서체/프레임·알림 슬롯 기준을 정한 뒤 UI Kit와 저작 스크립트로 화면을 구성했습니다.
- **계약 확인:** `BindWidget`을 점검하여 C++-Blueprint 연결을 유지했습니다.
- **게임 실행 확인:** 도감 등록·퀘스트 대화·던전 완료 화면에서 Widget 상태와 키 안내를 확인했습니다.
- **저작 산출물:** C++ 및 Widget Blueprint 변경, 화면 캡처를 PR에 포함했습니다. 저작 과정에 사용한 UI Python 스크립트는 현재 PR 추적 파일에 포함되지 않습니다.

## 입력·알림 관리의 경계

| 대상 | 실제 구현 | 관리 경계 |
|---|---|---|
| 일반 Screen | `UUIStackSubsystem::Open / Close / ApplyInputState` | 최상단 Screen의 InputMode·커서·게임 입력 차단 |
| Dungeon HUD / Result | `UDungeonUIRouterSubsystem` | 던전 전용 ViewData 및 비동기 위젯 수명 |
| Notice | `UNoticeSubsystem::Push / Clear` | Slot별 표시 정책과 Channel별 제거 |
| 입력 글리프 | `UInputHintSubsystem::GlyphFor` | Enhanced Input 매핑과 키보드·패드 전환 |

메뉴 탭 Q/E와 LB/RB 전환은 구현됐으며, **메뉴 내부 버튼·슬롯 간 게임패드 포커스 탐색은 별도 범위**입니다.

## 관련 코드

- [UIStackSubsystem.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/UI/System/UIStackSubsystem.cpp)
- [InputHintSubsystem.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/UI/Input/InputHintSubsystem.cpp)
- [NoticeSubsystem.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/UI/Notice/NoticeSubsystem.cpp)
- [QuestWidget.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/UI/Quest/QuestWidget.cpp)
- [MonsterDexWidget.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/UI/MonsterDex/MonsterDexWidget.cpp)
- [DefaultGame.ini](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Config/DefaultGame.ini)
- [[11. UI Architecture & Client Presentation|11_Client_State_Presentation_Pipeline]]
- [[16. MonsterDex System|16_MonsterDex_System]]
- [[17. Quest System|17_Quest_System]]
