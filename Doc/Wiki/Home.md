# Sonheim Engineering Wiki

**Unreal Engine 5.5 · C++ · Listen Server 멀티플레이 액션 어드벤처**

전투·스킬·상호작용·인벤토리·협동 제작·몬스터 포획/Partner를 구현하고, 이를 **분기형 던전·보스·도감·퀘스트·UI**에 연결한 프로젝트입니다. 이 Wiki는 각 기능의 실제 C++ 구조, 서버 처리, 데이터 정의와 게임 화면을 설명합니다.

**처음 보는 경우:** [[01. Project Overview|01_Project_Overview]]에서 전체 게임플레이와 구현 범위를 확인한 뒤, 관심 기술 문서로 이동하면 됩니다.

[전체 프로젝트 플레이 영상](https://www.youtube.com/watch?v=TDRRWp6M_9E) · [GitHub Repository](https://github.com/chungheonLee0325/Sonheim) · [Source-only Mirror](https://github.com/chungheonLee0325/Sonheim.Source)

---

## 대표 구현부터 보기

| 관심 영역 | 실제 구현 | 바로가기 |
|---|---|---|
| **전투·스킬** | 데이터 기반 스킬, 서버 Cast/Fire, 근접 충돌 보간·Damage Context | [[05. Combat, Skill & Animation|05_Combat_Skill_Animation]] |
| **인벤토리·협동 제작** | OwnerOnly FastArray, 장비·Skill Grant 연동, 여러 플레이어의 제작 진행 공유 | [[07. Inventory & Crafting|07_Inventory_Crafting]] |
| **포획·Partner** | 포획 서버 판정, 보유 Pal 복제, 같은 몬스터 Actor의 Partner AI 재사용 | [[08. Pal Capture & Partner Lifecycle|08_Pal_Capture_Partner_Lifecycle]] |
| **분기 던전·보스** | Event/Condition/Action/Transition, Boss FSM·PatternClock·Telegraph | [[09. Branching Dungeon Runtime|09_Branching_Dungeon_Runtime]] · [[10. Boss Encounter Runtime|10_Boss_Encounter_Runtime]] |
| **UI·클라이언트 표현** | 화면 Stack, 공용 알림, Snapshot → Presenter → ViewData, UMG v2 | [[11. UI Architecture & Client Presentation|11_Client_State_Presentation_Pipeline]] · [[18. UMG v2 Migration & Editor Verification|18_UMG_v2_Migration]] |
| **콘텐츠 검증·도구** | Dungeon Graph·Boss Pattern Validation, AgentMcp 기반 에디터 수정·PIE 확인 | [[13. Content Authoring & Validation|13_Content_Authoring_Validation]] · [[14. Unreal Editor Automation & Verification|14_Unreal_Editor_Automation_Verification]] |

## 구현 구조 한눈에 보기

~~~mermaid
flowchart LR
    DATA["DataTable · DataAsset<br/>GameplayTag · StringTable"] --> GAME["Gameplay<br/>Actor · Component · Interface"]
    DATA --> CONTENT["통합 콘텐츠<br/>Dungeon Runtime · Boss FSM"]
    GAME --> STATE["서버 권위 상태<br/>RPC · FastArray · Replication"]
    CONTENT --> STATE
    STATE --> UI["클라이언트 화면<br/>Delegate · Presenter · UMG"]
~~~

- **Gameplay:** PlayerState에 아이템·Pal·도감·퀘스트 기록, Pawn에 현재 캐릭터 동작을 두고 Component·Interface로 기능을 연결합니다.
- **Content:** 던전은 서버 Runtime이 Stage를 진행하고, 보스는 Pattern Data와 FSM으로 행동을 실행합니다.
- **Networking:** 서버가 전투·제작·포획 결과를 결정하고, 개인 상태와 공유 상태의 복제 범위를 구분합니다.
- **Presentation:** 독립적인 HUD는 변경 이벤트를, 던전처럼 복합적인 화면은 Snapshot·ViewData를 사용합니다.

구조와 책임의 상세 근거는 [[02. Gameplay Architecture|02_Gameplay_Architecture]]에 있습니다.

---

## 전체 기술 문서 · 01–18

### 프로젝트·아키텍처

| 번호 | 문서 | 확인할 내용 |
|---|---|---|
| **01** | [[Project Overview|01_Project_Overview]] | 프로젝트 데모·게임플레이 루프·핵심 구현 |
| **02** | [[Gameplay Architecture|02_Gameplay_Architecture]] | PlayerState / Pawn / Subsystem 책임, 데이터와 UI 경계 |
| **03** | [[Data & Content Architecture|03_Data_Content_Architecture]] | DataTable / DataAsset / GameplayTag / StringTable 선택 |

### 게임플레이 시스템

| 번호 | 문서 | 확인할 내용 |
|---|---|---|
| **04** | [[Player & Character Systems|04_Player_Character_Systems]] | 장비 → 스탯·스킬·무기 외형, Action State·Condition |
| **05** | [[Combat, Skill & Animation|05_Combat_Skill_Animation]] | Skill Grant·Phase, Montage Notify·근접 판정·피해 |
| **06** | [[World Interaction Systems|06_World_Interaction_Systems]] | `IInteractableInterface` 다형성, 공통 탐지·Hold |
| **07** | [[Inventory & Crafting|07_Inventory_Crafting]] | FastArray, 보관함·제작대, 협동 작업·환불 |
| **08** | [[Pal Capture & Partner Lifecycle|08_Pal_Capture_Partner_Lifecycle]] | PalSphere·포획 Reveal·OwnerOnly 목록·Partner |
| **16** | [[MonsterDex System|16_MonsterDex_System]] | 도감 기록·보상·3D 프리뷰·몬스터 썸네일 |
| **17** | [[Quest System|17_Quest_System]] | 퀘스트 목표 집계·NPC 상호작용·월드 연동 |

### 통합 콘텐츠·UI·멀티플레이

| 번호 | 문서 | 확인할 내용 |
|---|---|---|
| **09** | [[Branching Dungeon Runtime|09_Branching_Dungeon_Runtime]] | 실제 분기 플레이, Event Queue·Stage Graph·장벽 |
| **10** | [[Boss Encounter Runtime|10_Boss_Encounter_Runtime]] | PatternClock·Telegraph·Phase·Break·포획 |
| **11** | [[UI Architecture & Client Presentation|11_Client_State_Presentation_Pipeline]] | 화면 Stack·Notice·Dungeon Snapshot→ViewData |
| **12** | [[Multiplayer Synchronization|12_Multiplayer_Synchronization]] | 서버 요청, 복제 범위, FastArray·UObject, 연출 RPC |
| **18** | [[UMG v2 Migration & Editor Verification|18_UMG_v2_Migration]] | UI 전후 비교, 입력·알림 공통화, 에디터 검증 |

### 콘텐츠 제작·검증·개발 이력

| 번호 | 문서 | 확인할 내용 |
|---|---|---|
| **13** | [[Content Authoring & Validation|13_Content_Authoring_Validation]] | 잘못된 Stage·분기·Boss Timing의 에디터 오류 검출 |
| **14** | [[Unreal Editor Automation & Verification|14_Unreal_Editor_Automation_Verification]] | AgentMcp Inspect→Edit→Compile→PIE→Review |
| **15** | [[Development History & Retrospective|15_Development_History_Retrospective]] | Pal·Skill·Inventory·Dungeon·UI의 실제 리팩터링 커밋 |

---

## 코드와 구현 근거

각 기술 문서는 기능별로 **C++ 클래스·주요 처리 흐름·데이터 구조**를 설명하고, 관련 소스 코드로 연결합니다. 플레이 장면이 구현 이해에 도움이 되는 문서에는 실제 게임 영상과 에디터 캡처를 배치했습니다.

**변경 이력**은 [[15. Development History & Retrospective|15_Development_History_Retrospective]]의 Git 커밋에서, **콘텐츠 검증 방식**은 [[13. Content Authoring & Validation|13_Content_Authoring_Validation]]에서 확인할 수 있습니다.

## 관련 저장소

- [Sonheim](https://github.com/chungheonLee0325/Sonheim) — Unreal Engine 프로젝트·에셋·Wiki 원본
- [Sonheim.Source](https://github.com/chungheonLee0325/Sonheim.Source) — Source / Config / Docs 중심 코드 검토용
- [AgentMcp](https://github.com/chungheonLee0325/AgentMcp) — UE 5.5 Editor MCP 플러그인·도구 및 테스트베드
