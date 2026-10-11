# 16. 도감·퀘스트와 UMG v2 통합

기존 멀티플레이 시스템에 **서버 권위 도감·퀘스트 런타임**을 이식하고, 화면 스택·알림·키 안내를 공통화하면서 UMG를 v2로 개편했습니다. 게임 규칙은 C++/DataTable에서 유지하고, UI는 복제된 상태와 화면용 데이터를 소비합니다. Widget Blueprint 저작에는 Claude Code와 [AgentMcp](https://github.com/chungheonLee0325/AgentMcp)를 사용했습니다.

## 요약

| 영역 | 구현 결과 | 핵심 코드 |
|---|---|---|
| 도감 | 종별 처치·포획 기록, 단계별 1회 보상, 저장·Owner 복제, 3D 미리보기 | `UMonsterDexComponent`, `AMonsterDexStage` |
| 퀘스트 | 단계·목표 집계, NPC 수락/완료 보고, 일지·추적기·대화, 저장·Owner 복제 | `UQuestComponent`, `UQuestSubsystem` |
| UI 수명·입력 | Screen Stack, cursor / InputMode 복원, 탭 이동, 입력 장치별 키 안내 | `UUIStackSubsystem`, `UInputHintSubsystem` |
| 알림·표현 | Feed / Headline / Title 3개 슬롯, 보상 슬롯 재사용, 전반적인 UMG v2 | `UNoticeSubsystem`, `USlotWidget` |
| 검증 | UI Stack 6, 도감 3, 퀘스트 8, 던전 HUD 98 항목 | 총 115개 통과(작업 기록 기준) |

## 구현 결과

| 기존 도감 | v2 도감 |
|---|---|
| ![도감 개편 전](../Media/Wiki/16_UMG_Migration_with_Claude/16_dex_before.jpg) | ![도감 개편 후](../Media/Wiki/16_UMG_Migration_with_Claude/16_dex_after.jpg) |

| 기존 퀘스트 일지 | v2 퀘스트 일지 |
|---|---|
| ![퀘스트 일지 개편 전](../Media/Wiki/16_UMG_Migration_with_Claude/16_journal_before.jpg) | ![퀘스트 일지 개편 후](../Media/Wiki/16_UMG_Migration_with_Claude/16_journal_after.jpg) |

| 기존 던전 HUD | v2 던전 HUD |
|---|---|
| ![던전 HUD 개편 전](../Media/Wiki/16_UMG_Migration_with_Claude/16_dungeon_hud_before.jpg) | ![던전 HUD 개편 후](../Media/Wiki/16_UMG_Migration_with_Claude/16_dungeon_hud_after.jpg) |

위 비교는 기존 화면과 v2 화면을 같은 기능 범위에서 캡처한 결과입니다. 게임 실행 시나리오와 검증 범위는 아래에서 별도로 설명합니다.

## 목차

- [도감·퀘스트 런타임](#도감퀘스트-런타임)
- [UI Stack과 입력 상태](#ui-stack과-입력-상태)
- [알림과 UI 표현](#알림과-ui-표현)
- [AgentMcp 기반 UMG 저작](#agentmcp-기반-umg-저작)
- [검증](#검증)
- [트레이드오프와 후속 과제](#트레이드오프와-후속-과제)
- [관련 코드](#관련-코드)

---

## 도감·퀘스트 런타임

### PlayerDeed로 gameplay event 연결

이전 도감·퀘스트 구현을 현재 전투·포획·제작·던전 시스템에 붙이기 위해 `ASonheimPlayerState::ReportDeed`를 공통 진입점으로 사용했습니다.

~~~mermaid
flowchart TB
    G["Gameplay Producer<br/>처치 · 포획 · 제작 · 지역 진입 · 던전 완료"]
    P["ASonheimPlayerState<br/>ReportDeed → OnDeed"]
    D["UMonsterDexComponent<br/>Kill / Capture Count"]
    Q["UQuestComponent<br/>Step / Objective Progress"]
    S["서버 기록<br/>Host Save + FastArray"]
    V["Owner Client<br/>도감 · 일지 · 추적기 · Notice"]
    G --> P
    P --> D
    P --> Q
    D --> S
    Q --> S
    S --> V
~~~

`EPlayerDeed`는 `Kill`, `Capture`, `Craft`, `Reach`, `ClearDungeon`을 구분합니다. 도감은 Kill/Capture만 기록하고, 퀘스트는 목표 종류에 맞는 이벤트를 누적합니다. `Possess` 목표는 `OnInventoryChanged`로 재계산하며, `Talk` / `Deliver`는 퀘스트 대상과의 상호작용으로 처리합니다.

`ReportDeed`는 게임플레이 결과를 각 콘텐츠 시스템에 전달하는 이벤트 경계이지, UI 알림 자체를 발생시키는 함수는 아닙니다. 알림은 기록·단계 전환을 처리한 시스템이 필요한 때에 생성합니다.

### 도감: 종별 상태, 보상 중복 방지

`DT_MonsterDex`의 `FMonsterDexData`가 종별 설명·보상 단계·표시용 캐릭터 클래스를 정의합니다. 플레이어별 `FMonsterDexEntry`에는 `AreaObjectID`, `KillCount`, `CaptureCount`, `ClaimedTiers`가 저장됩니다.

- `UMonsterDexComponent::Record`는 서버에서 종별 횟수를 증가시키고, 처음 등록된 종에는 Headline 알림을 보냅니다.
- 보상 조건을 새로 충족한 경우 Feed 알림을 보냅니다. 보상 지급 자체는 별도의 `ServerClaimReward` 요청입니다.
- 서버에서 `CanClaimReward`를 재확인하고 `ClaimedTiers` 비트를 먼저 설정한 뒤 보상을 지급해 동일 단계의 반복 수령을 막습니다.
- `FMonsterDexList : FFastArraySerializer`는 `COND_OwnerOnly`로 복제되며, `PostReplicatedAdd/Change`에서 `OnDexChanged`를 알립니다.

3D 미리보기는 실제 월드 몬스터를 옮기지 않고 **로컬 전용 `AMonsterDexStage`** 에 표시합니다. 별도 SkeletalMesh와 `SceneCaptureComponent2D`로 RenderTarget에 렌더링하여 UI와 결합합니다. 해당 Actor는 복제하지 않습니다.

### 퀘스트: 단계 전환과 상호작용 검증

`DT_Quest`는 `FQuestData → FQuestStep → FQuestObjective` 순서로 목표를 정의합니다. `FQuestRecord`는 플레이어의 수락 상태, 현재 단계, 목표별 진행량, 추적 여부를 유지합니다.

~~~text
Gameplay Event / Inventory Changed / Quest Target Interact
             ↓
      QuestComponent
   Advance / Settle / Finish
             ↓
    FQuestRecord 변경
             ↓
  MarkItemDirty + Host Save
             ↓
   Owner FastArray Replication
             ↓
  QuestView → Journal / Tracker / Dialog
~~~

서버 RPC `ServerAccept` / `ServerTurnIn`은 선행 퀘스트, 상태 및 NPC 대상 이름·거리(`ReachDistance`)를 확인합니다. `Finish`는 완료 상태를 먼저 표시하고 보상을 지급합니다. 동일 요청으로 보상이 중복 지급되지 않도록 하는 순서입니다.

도감과 퀘스트 기록은 각각 `UMonsterDexSubsystem` / `UQuestSubsystem`이 호스트의 `USaveGame` 기반 저장소에 보관합니다. 현재 저장은 **호스트 기반**이며 별도의 영구 온라인 계정 서버는 아닙니다. 두 컴포넌트는 소유 클라이언트에 FastArray로 동기화됩니다.

![섬지기 대화창](../Media/Wiki/16_UMG_Migration_with_Claude/16_dialog_after.jpg)

*NPC 대화에서 퀘스트를 받고 진행할 수 있습니다. 아이템 보상은 공용 Slot Widget을 사용합니다.*

## UI Stack과 입력 상태

여러 화면이 각자 `SetInputMode`와 마우스 상태를 변경할 경우, 마지막으로 닫힌 화면이 아닌 **현재 남아 있는 최상위 화면**을 기준으로 복원해야 합니다. 이를 `UUIStackSubsystem : ULocalPlayerSubsystem`으로 통합했습니다.

~~~text
Open(Menu) → Open(MonsterDex) → Close(MonsterDex)
     ↓              ↓                ↓
 Menu Rule      Dex Rule          Menu Rule
     ↓              ↓                ↓
Cursor / InputMode / Gameplay Block / Menu IMC
~~~

- `FUIScreenRule`: 화면 ID별 ZOrder, `GameOnly / GameAndUI / UIOnly`, 커서 표시, 게임 입력 차단 정책. 설정은 `DefaultGame.ini`에서 관리합니다.
- `FUIStackEntry`: 함께 열고 닫는 다중 Widget을 하나의 Screen으로 묶습니다. `Open`은 열린 화면을 최상단으로 올릴 수 있습니다.
- `ApplyInputState`: 최상단 화면의 규칙을 사용하여 `SetInputMode`, 커서 및 `IMC_Menu` 활성화를 갱신합니다. 화면이 모두 닫히면 GameOnly로 돌아갑니다.
- `TopTab` / `StepTab`: 메뉴에 정의된 탭 중 선택 가능한 탭을 순환합니다.

**키 표시를 액션 매핑과 분리하지 않음:** `UInputHintSubsystem`은 Enhanced Input의 실제 매핑에서 키를 읽고, 마지막 입력 장치(키보드/패드)에 맞는 글리프를 선택합니다. Slate 입력 프로세서가 장치 전환을 감지하며 `ControlMappingsRebuiltDelegate`를 받아 매핑 변경 후 다시 표시합니다.

| 키보드 | 게임패드 |
|---|---|
| ![키보드 안내](../Media/Wiki/16_UMG_Migration_with_Claude/16_keyhint_keyboard.jpg) | ![게임패드 안내](../Media/Wiki/16_UMG_Migration_with_Claude/16_keyhint_gamepad.jpg) |

상단 탭의 Q/E와 LB/RB 전환은 구현했지만 메뉴 내부 버튼·슬롯 간 **패드 포커스 이동은 후속 작업**입니다. UI Stack은 일반 Screen 입력 수명 관리용이고, Dungeon HUD/Result는 기존 `UDungeonUIRouterSubsystem` 경로를 유지합니다.

## 알림과 UI 표현

### 역할별 Notice 슬롯

`UNoticeSubsystem`은 `FNoticeData`를 받되 레이아웃을 직접 소유하지 않습니다. Slot별 Widget Class·Style·Order·ZOrder를 설정하고, producer의 `Channel`을 보존해 해당 producer의 알림만 지울 수 있게 했습니다.

| Slot | 대상 | 정책 |
|---|---|---|
| `Feed` | 퀘스트·제작·보상 가능·경고·던전 진행 | `Stack` — 좌측에 누적 |
| `Headline` | 레벨업·첫 도감 등록·보스 처치 | `TakeTurns` — 순차 표시 |
| `Title` | 지역명 | `ReplaceShowing` — 최신 지역으로 교체 |

| 기존 단일 알림 | Feed v2 |
|---|---|
| ![기존 알림](../Media/Wiki/16_UMG_Migration_with_Claude/16_toast_before_feed.jpg) | ![Feed v2](../Media/Wiki/16_UMG_Migration_with_Claude/16_toast_after_feed_stack.jpg) |

| 도감 기존 알림 | 도감 Headline + 지역 Title |
|---|---|
| ![도감 기존](../Media/Wiki/16_UMG_Migration_with_Claude/16_toast_before_dex.jpg) | ![도감 Headline](../Media/Wiki/16_UMG_Migration_with_Claude/16_toast_after_dex_title.jpg) |

![레벨업 Headline](../Media/Wiki/16_UMG_Migration_with_Claude/16_toast_after_levelup.jpg)

*레벨업과 진행 상황은 같은 텍스트 카드가 아니라 별도 슬롯과 스타일로 표시합니다.*

| 지역명 기존 | Title v2 |
|---|---|
| ![지역명 이전](../Media/Wiki/16_UMG_Migration_with_Claude/16_toast_before_title.jpg) | ![지역명 개편](../Media/Wiki/16_UMG_Migration_with_Claude/16_toast_after_title_blur.jpg) |

### BindWidget 계약 유지와 공용 Slot 재사용

`C++ Widget → BindWidget → Widget Blueprint` 계약은 그대로 유지했습니다. 이벤트 그래프나 애니메이션이 연결된 `BP_Slot`과 필드 HUD는 Widget Tree를 삭제하고 다시 만드는 대신 기존 요소의 속성·레이아웃을 수정했습니다. 상대적으로 제약이 적은 화면은 트리를 재구성했습니다.

보상 아이템은 텍스트 칩 대신 기존 `USlotWidget::AddReadOnly` 경로를 사용합니다. 도감·퀘스트·던전 HUD/Result에서 ItemID를 텍스트로 따로 그리지 않고, 인벤토리와 같은 아이콘·수량·희귀도·툴팁 표현을 재사용합니다. Dungeon 추가 목표 보상은 설명 문자열이 아니라 Dungeon 정의의 보상 정보를 읽어 표시합니다.

| 인벤토리 개편 전 | 인벤토리 v2 |
|---|---|
| ![인벤토리 전](../Media/Wiki/16_UMG_Migration_with_Claude/16_inventory_before.jpg) | ![인벤토리 후](../Media/Wiki/16_UMG_Migration_with_Claude/16_inventory_after.jpg) |

| 던전 결과 전 | 결과 카드 v2 |
|---|---|
| ![결과 전](../Media/Wiki/16_UMG_Migration_with_Claude/16_dungeon_result_before.jpg) | ![결과 후](../Media/Wiki/16_UMG_Migration_with_Claude/16_dungeon_result_after.jpg) |

| 필드 HUD 전 | 필드 HUD v2 |
|---|---|
| ![필드 HUD 전](../Media/Wiki/16_UMG_Migration_with_Claude/16_hud_before.jpg) | ![필드 HUD 후](../Media/Wiki/16_UMG_Migration_with_Claude/16_hud_after.jpg) |

폰트는 Pretendard·Black Han Sans, 아이콘은 SVG 기반 27종을 도입했습니다. UI 문구 51개는 String Table 기준으로 정리했습니다. 이 변경은 Gameplay Runtime을 교체하기보다 동일한 상태를 전달받는 **Presentation 계층을 개편**한 작업입니다.

![v2 아이콘 27종](../Media/Wiki/16_UMG_Migration_with_Claude/16_icons.png)

| 섬지기 NPC 이전 | 수정 후 |
|---|---|
| ![NPC 전](../Media/Wiki/16_UMG_Migration_with_Claude/16_islander_before.jpg) | ![NPC 후](../Media/Wiki/16_UMG_Migration_with_Claude/16_islander_after.jpg) |

## AgentMcp 기반 UMG 저작

코딩 에이전트를 단순 이미지 생성에 쓰지 않고, Editor의 Widget Tree를 직접 inspect·편집하는 **저작·확인 도구**로 사용했습니다.

~~~mermaid
flowchart LR
    R["UI Reference<br/>스타일·배치 기준"]
    S["Style / Widget Kit<br/>저작 스크립트"]
    M["AgentMcp<br/>UMG inspect / edit"]
    C["Compile / Save<br/>BindWidget read-back"]
    P["PIE<br/>실제 게임 경로"]
    V["Capture / Verification"]
    R --> S --> M --> C --> P --> V
    V -. "피드백" .-> S
~~~

1. 레퍼런스와 스타일 규칙을 정하고, 색·폰트·띠·아이콘·키 안내를 공용 위젯 키트로 구성했습니다.
2. Python 저작 스크립트로 Widget Tree를 구성하고 AgentMcp를 통해 Editor의 Widget Blueprint에 반영했습니다.
3. `BindWidget` 이름 및 타입을 read-back으로 확인한 뒤 Compile/Save하고, 실제 게임 화면에서 입력과 표시를 확인했습니다.
4. 도감·퀘스트·던전 검증을 다시 실행하여 표현 변경이 Runtime 계약을 깨뜨리지 않았는지 확인했습니다.

**재현 범위:** 기존 Widget Blueprint와 코드·캡처는 PR에 포함되어 있습니다. 다만 이 작업에서 사용한 `Scripts/UI` 저작·캡처 스크립트는 현재 PR의 추적 파일 목록에 없어 저장소만으로 전체 제작 과정을 그대로 재실행할 수 있다고 주장하지 않습니다. AgentMcp 자체의 도구 구조는 [14. Unreal Editor Automation & Verification](14_Unreal_Editor_Automation_Verification.md)에서 설명합니다.

## 검증

PR 작성 시 기록된 PIE/검증 실행 결과입니다. 새로운 테스트를 이 문서 변경 과정에서 재실행한 것은 아닙니다.

| 검증 스크립트 | 통과 항목 | 주요 확인 내용 |
|---|---:|---|
| `verify_ui_stack` | 6/6 | 탭 순환, 키보드→패드 표시, 화면 종료 시 메뉴 입력 해제 |
| `verify_dex` | 3/3 | 도감 등록과 보상 알림 |
| `verify_quest` | 8/8 | 퀘스트 수락·진행·대화 및 실제 던전 1판 |
| `verify_dungeon_hud` | 98/98 | 1280×720 / 640×482, 서버·클라이언트, 추가 목표 보상 슬롯 |
| **합계** | **115/115** | 작업 당시 기록 기준 |

검증 과정에서 에디터 Python으로 Client RPC를 호출하면 실제 네트워크 호출 경로를 재현하지 못하는 문제가 있었습니다. 레벨업 표시 확인은 실제 처치와 경험치 획득으로 수행했습니다. 입력 글리프 초기 누락은 Enhanced Input 매핑이 다시 빌드되는 이벤트에 갱신을 연결하여 해결했습니다.

## 트레이드오프와 후속 과제

| 판단 | 얻은 점 | 제약 |
|---|---|---|
| PlayerState 컴포넌트 + Owner FastArray | 개인 기록의 서버 권위와 델타 복제 | 호스트 저장 방식은 온라인 영속 서버가 아님 |
| UIStackSubsystem 통합 | 중첩 Screen의 cursor·input 복원 정책 일원화 | Dungeon UI Router, Inventory 로컬 Popup은 별도 경계 |
| BindWidget 계약 유지 | 기존 C++·Blueprint 이벤트 연동을 보존 | 기존 애니메이션 위젯은 트리 재생성보다 제자리 수정 필요 |
| Notice 슬롯별 정책 | 알림의 중요도별 표현과 Queue 동작 분리 | 위젯 별 위치·모션 저작은 여전히 필요 |
| AgentMcp 활용 | Editor 편집과 Runtime 확인을 같은 작업 흐름에서 수행 | 저작 스크립트가 저장소에 없으면 완전 재현 불가 |

남은 범위는 **메뉴 내부 게임패드 포커스 탐색**, **보유 팰·지도 탭 본 화면**, **도감 Headline의 팰 초상**입니다.

## 관련 코드

- [PlayerState / PlayerDeed](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/AreaObject/Player/SonheimPlayerState.h)
- [MonsterDexComponent](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/AreaObject/Player/Utility/MonsterDexComponent.cpp) · [MonsterDexTypes](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/MonsterDex/MonsterDexTypes.h)
- [QuestComponent](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/AreaObject/Player/Utility/QuestComponent.cpp) · [QuestTypes](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/Quest/QuestTypes.h)
- [UIStackSubsystem](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/UI/System/UIStackSubsystem.cpp) · [InputHintSubsystem](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/UI/Input/InputHintSubsystem.cpp)
- [NoticeSubsystem](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/UI/Notice/NoticeSubsystem.cpp) · [DefaultGame.ini](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Config/DefaultGame.ini)
- [MonsterDexStage](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/UI/MonsterDex/MonsterDexStage.cpp)
- [[11. UI Architecture & Client Presentation|11_Client_State_Presentation_Pipeline]]
- [[14. Unreal Editor Automation & Verification|14_Unreal_Editor_Automation_Verification]]
