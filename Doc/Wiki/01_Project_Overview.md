# 01. Project Overview

**Sonheim은 Unreal Engine 5.5 / C++ 기반의 서버 권위 멀티플레이 액션 어드벤처입니다.**

플레이어는 전투와 탐험으로 자원을 얻고 장비를 제작하며, 몬스터를 포획해 동료로 활용합니다. **전투·포획의 결과는 도감 기록과 퀘스트 목표로 이어지고**, 분기형 던전에서는 그동안 구현한 상호작용·전투·보상·동료 시스템이 하나의 콘텐츠 안에서 함께 동작합니다.

초기 2인 팀 프로젝트로 시작해 개별 게임플레이 기능과 멀티플레이 구조를 구축했고, 이후 도감·퀘스트·보스·던전·UI를 추가하며 콘텐츠를 확장했습니다.

[**전체 플레이 데모 — YouTube**](https://www.youtube.com/watch?v=TDRRWp6M_9E) · [**소스 코드**](https://github.com/chungheonLee0325/Sonheim) · [전체 기술 문서](Home.md)

![Sonheim 전체 게임플레이](../Gifs/Project_Overview.gif)

**개발 환경:** UE 5.5 · C++ · Blueprint/UMG · Listen Server · Steam/Null OSS

---

## 게임플레이 이벤트와 진행 기록 연결

~~~mermaid
flowchart LR
    ACTION["전투 · 포획 · 제작 · 탐험"] --> BAG["아이템 · 장비 · Partner"]
    BAG --> PLAY["다음 전투 · 탐험"]
    PLAY --> ACTION
    ACTION --> RECORD["PlayerState::ReportDeed<br/>서버 게임플레이 이벤트"]
    RECORD --> DEX["MonsterDex<br/>발견 · 기록 · 보상"]
    RECORD --> QUEST["Quest<br/>목표 · 단계 · 완료"]
    ACTION -. "처치 · 포획 · 상호작용" .-> DUNGEON["Forgotten Ruins<br/>Stage · Branch · Boss"]
    DUNGEON --> RESULT["보상 · 최고 기록 · 등급"]
~~~

`PlayerState::ReportDeed`를 통해 처치·포획·제작·탐험 결과를 도감·퀘스트 진행에 전달합니다. 던전은 별도 서버 Runtime이 Stage와 목표를 판정합니다.

### MonsterDex — 처치·포획 기록과 보상

![MonsterDex: 몬스터 목록, 상세 3D 프리뷰, 처치·포획 기록과 단계별 보상](../Media/Wiki/16_MonsterDex_System/16_dex_screen_areas.svg)

몬스터를 **처음 처치하거나 포획하면 도감에 등록**되고, 발견하지 못한 종은 이름과 모델을 실루엣으로 표시합니다. 등록 후에는 처치·포획 횟수와 단계별 보상이 갱신됩니다.

- `DT_MonsterDex`에 종별 정보·보상 조건을 정의하고, `UMonsterDexComponent`는 플레이어별 기록을 관리합니다.
- `AMonsterDexStage`가 상세 **회전형 3D 프리뷰**와 목록용 **몬스터 썸네일**을 생성합니다.
- 서버는 보상 수령 조건과 중복 수령을 검사하고, 기록 변경분은 **OwnerOnly FastArray**로 전달합니다.

[도감 등록 영상](https://github.com/user-attachments/assets/d1d96b5f-1076-4e29-872d-3399a540acac) · [보상 수령 영상](https://github.com/user-attachments/assets/f498002f-d15e-45ab-ac9b-1380ac0ad82b) · [16. MonsterDex 구현](16_MonsterDex_System.md)

### Quest — 게임플레이 이벤트로 단계 진행

| 퀘스트 일지 | NPC 수락·완료 보고 |
|---|---|
| ![진행 단계와 목표·보상이 보이는 퀘스트 일지](../Media/Wiki/16_UMG_Migration_with_Claude/16_journal_after.jpg) | ![퀘스트 NPC의 수락·완료 보고 대화 화면](../Media/Wiki/16_UMG_Migration_with_Claude/16_dialog_after.jpg) |

NPC에게 퀘스트를 수락한 뒤 몬스터 처치·포획, 아이템 제작, 지역 도착 등으로 **현재 단계의 목표를 진행**합니다. 목표를 모두 달성하면 다음 단계로 넘어가거나 NPC에게 완료를 보고하고 보상을 받습니다.

`DT_Quest`는 선행 조건·단계·목표·보상을 정의하고, `UQuestComponent`는 플레이어별 진행을 서버에서 관리합니다. 아이템 보유량을 검사하는 `Possess`와 실제 아이템을 차감하는 `Deliver`도 구분합니다. **건설 퀘스트의 자재 전달은 월드의 건설 완료 상태까지 변경**합니다.

**NPC 수락 → 목표 진행 → HUD 갱신 → 완료 보고·보상**

https://github.com/user-attachments/assets/356d1438-7c3c-417d-8bd4-b8dcb7a3efb6

[17. Quest 구현](17_Quest_System.md)

---

## Forgotten Ruins — 시스템을 연결한 분기 던전

Forgotten Ruins는 **분기형 멀티플레이 던전**으로, 입장·레버 상호작용, 몬스터 처치·포획, 보스 패턴, 보상과 기록을 하나의 Run에서 연결합니다.

### 지름길과 추가 웨이브

~~~text
입장 → 경비실 Wave A
          ├─ 레버 사용 → 지름길 ────┐
          └─ 레버 미사용 → 추가 Wave ┤
                                   ↓
                              수호자의 방
                                   ↓
                           결과 · 보상 · 등급
~~~

레버를 사용하면 서버가 `RunTag`를 기록합니다. **Wave A 완료 시점**에 `Event / Condition / Transition` 정의를 평가해 지름길 또는 추가 웨이브를 선택합니다. Stage별 장벽과 목표도 현재 Run 상태에 맞춰 변경됩니다.

**지름길 선택 플레이**

https://github.com/user-attachments/assets/29858da6-7d13-4297-9d71-41f53272a952

[09. Branching Dungeon Runtime](09_Branching_Dungeon_Runtime.md)

### 수호자 보스 — 데이터 기반 패턴 전투

수호자 보스는 기존 몬스터·피해·포획 기능을 활용하면서 **별도의 Boss FSM과 Pattern DataAsset**으로 전투를 구성했습니다. 패턴별 추적·경고·타격 타이밍과 영역 형상을 데이터로 지정하고, Break 누적 시 Down, HP 조건 충족 시 Resting으로 진입합니다. **Resting 중에는 기존 Pal 포획 시스템으로 포획을 시도**할 수 있습니다.

**텔레그래프와 타격 시연**

https://github.com/user-attachments/assets/cd81b345-d192-4374-9573-904b561f40bd

[10. Boss Encounter Runtime](10_Boss_Encounter_Runtime.md) · [08. Pal Capture & Partner Lifecycle](08_Pal_Capture_Partner_Lifecycle.md)

---

## UMG v2 — 화면과 알림

![도감 등록 시 Feed·Headline·Title이 함께 표시되는 UMG v2 게임 화면](../Media/Wiki/16_UMG_Migration_with_Claude/16_toast_after_dex_title.jpg)

도감 등록·퀘스트 갱신·제작 완료·지역 진입 알림을 공통 슬롯(`Feed / Headline / Title`)에 표시합니다.

- **Feed / Headline / Title:** `UNoticeSubsystem`은 알림 데이터를 전달하고, `UGameUIToastSubsystem`은 슬롯별 표시·대기 정책을 관리합니다.
- **일반 메뉴:** `GameUIFramework` 플러그인의 `UGameUIManagerSubsystem`이 인벤토리·제작·도감·퀘스트 화면을 태그로 열고, 레이어별 화면 중첩과 입력 모드·커서를 처리합니다([21. Game UI Framework](21_UI_Framework.md)).
- **입력 안내:** `UInputHintSubsystem`이 키보드·게임패드 입력에 맞는 안내를 표시합니다.
- **게임플레이 → UI:** 도감·퀘스트 기록은 각각 ViewData로, 던전은 GameState Snapshot을 Presenter가 변환해 HUD·미니맵·결과 화면에 전달합니다.

[18. UMG v2](18_UMG_v2_Migration.md)에서 화면 개편 전후를 비교할 수 있습니다.

---

## 개발 구조와 기술적 선택

| 관심 기술 | Sonheim에서 실제로 구현한 방식 |
|---|---|
| **게임플레이 책임** | Actor/Component/Interface로 기능 구성, PlayerState에 보유·진행 기록, Pawn에 현재 행동 |
| **서버 권위·동기화** | Server RPC, OwnerOnly FastArray, Skill UObject 서브오브젝트 복제, 던전 GameState Snapshot |
| **데이터 기반 콘텐츠** | 반복 데이터는 DataTable, 던전은 `UPrimaryDataAsset`, 보스 패턴은 `UDataAsset`, 식별자는 GameplayTag |
| **전투·제작** | Animation Notify·근접 Trace 보간, 장비별 Skill Grant, 공유 제작 진행·부분 완료·환불 |
| **화면·콘텐츠 검증** | Delegate·Presenter/ViewData, Dungeon Stage Graph·Boss Pattern Validation, AgentMcp 에디터 작업 |

게임플레이의 판정과 보상은 서버가 결정합니다. **클라이언트는 복제된 상태를 화면 데이터로 변환하고 필요한 알림을 표시**하며, 플레이어별 도감·퀘스트 기록과 던전의 공유 상태는 서로 다른 경로로 동기화합니다.

| 더 알아볼 내용 | 문서 |
|---|---|
| **전체 구조·데이터 설계** | [02. Gameplay Architecture](02_Gameplay_Architecture.md) · [03. Data & Content Architecture](03_Data_Content_Architecture.md) |
| **캐릭터·전투·상호작용** | [04. Player & Character Systems](04_Player_Character_Systems.md) · [05. Combat, Skill & Animation](05_Combat_Skill_Animation.md) · [06. World Interaction Systems](06_World_Interaction_Systems.md) |
| **장비·협동 제작** | [07. Inventory & Crafting](07_Inventory_Crafting.md) |
| **UI·멀티플레이** | [11. Client State Presentation](11_Client_State_Presentation_Pipeline.md) · [12. Multiplayer Synchronization](12_Multiplayer_Synchronization.md) |
| **검증·개발 이력** | [13. Content Authoring & Validation](13_Content_Authoring_Validation.md) · [14. Unreal Editor Automation & Verification](14_Unreal_Editor_Automation_Verification.md) · [15. Development History & Retrospective](15_Development_History_Retrospective.md) |

**[전체 기술 문서 목록 (01–21) →](Home.md)**

**저장소:** [Sonheim](https://github.com/chungheonLee0325/Sonheim) · [Source-only Mirror](https://github.com/chungheonLee0325/Sonheim.Source) · [AgentMcp](https://github.com/chungheonLee0325/AgentMcp)
