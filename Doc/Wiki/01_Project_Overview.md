# 01. Project Overview

**Sonheim은 Unreal Engine 5.5 / C++ 기반의 서버 권위 멀티플레이 액션 어드벤처 프로젝트**입니다. 전투·아이템·제작·장비·몬스터 포획과 Partner를 구현했으며, 이를 하나의 게임플레이 흐름으로 연결했습니다. 이후 분기형 던전 **Forgotten Ruins**를 제작하면서 서버 진행 상태, 보스 전투, HUD·미니맵, 데이터 검증까지 확장했습니다.

초기 2인 팀 개발 이후 개별 시스템과 통합 콘텐츠를 확장하며, **기능별 데이터 소유권과 멀티플레이 동기화, C++·에셋·UI의 책임 경계**를 정리했습니다.

## 게임플레이 데모

[전체 프로젝트 플레이 영상 — YouTube](https://www.youtube.com/watch?v=TDRRWp6M_9E)

![Sonheim 프로젝트 플레이 개요](../Gifs/Project_Overview.gif)

| 구분 | 구현 |
|---|---|
| **엔진·언어** | Unreal Engine 5.5 · C++ · Blueprint / UMG |
| **멀티플레이** | Listen Server · Server RPC · Property Replication · FastArray · Steam/Null OSS |
| **기본 게임플레이** | 전투·스킬·장비·인벤토리·보관함·협동 제작·월드 상호작용 |
| **몬스터 시스템** | Pal 포획·보유·선택·소환·Partner AI / Skill |
| **진행 콘텐츠** | MonsterDex·Quest·분기 던전·보스·보상·기록/등급 |
| **콘텐츠 저작** | DataTable · DataAsset · GameplayTag · StringTable · Data Validation |
| **화면·도구** | UMG · UI Stack / Notice · Presenter / ViewData · AgentMcp Editor Tool |

## 목차

- [게임플레이 시스템의 연결](#게임플레이-시스템의-연결)
- [핵심 구현과 기술 선택](#핵심-구현과-기술-선택)
- [멀티플레이에서 상태를 나눈 방식](#멀티플레이에서-상태를-나눈-방식)
- [통합 콘텐츠 — Forgotten Ruins](#통합-콘텐츠--forgotten-ruins)
- [콘텐츠 작성과 화면 구현](#콘텐츠-작성과-화면-구현)
- [기술 문서 바로가기](#기술-문서-바로가기)

---

## 게임플레이 시스템의 연결

플레이어의 행동이 다른 시스템의 입력으로 이어지도록 구성했습니다.

~~~mermaid
flowchart LR
    WORLD["탐험 · 상호작용"] --> ITEM["아이템 획득"]
    WORLD --> FIGHT["전투 · 스킬"]
    FIGHT --> ITEM
    ITEM --> INV["인벤토리"]
    INV --> CRAFT["협동 제작"]
    CRAFT --> INV
    INV --> EQUIP["장비 · 스탯 · 스킬 Grant"]
    EQUIP --> FIGHT
    FIGHT --> CAPTURE["몬스터 포획"]
    CAPTURE --> PAL["보유 Pal · Partner"]
    PAL --> FIGHT
    FIGHT -. "처치·포획 이벤트" .-> DUNGEON["분기 던전 · 보스"]
    WORLD -. "입구·레버" .-> DUNGEON
    DUNGEON --> REWARD["보상 · 기록"]
    REWARD --> INV
~~~

| 시스템 | 연결되는 실제 동작 |
|---|---|
| **Combat → Item / Capture** | 서버가 공격 결과와 포획 성공을 확정하고 아이템 보상 또는 Pal 소유권에 반영 |
| **Inventory → Equipment** | 활성 무기 변경을 Stat Modifier·Skill Grant·무기 Mesh와 HUD에 전달 |
| **Inventory ↔ Crafting** | 재료를 서버에서 소비하고 여러 플레이어가 하나의 제작 작업을 누적한 뒤 완성품을 수령 |
| **Capture → Partner** | 포획한 몬스터 Actor의 소유권을 등록하고 같은 Actor를 소환해 Partner AI로 사용 |
| **Interaction → World Object** | `IInteractableInterface` 호출을 통해 아이템·보관함·제작대·던전 레버가 서로 다른 동작을 구현 |
| **기존 시스템 → Dungeon** | 처치·포획·상호작용·보상 이벤트를 Stage 목표와 분기 규칙에 연결 |

전투·제작·포획의 개별 코드와 던전 분기 규칙을 한곳에 합치지 않았습니다. **각 시스템의 상태·이벤트·공개 API를 콘텐츠에서 조합**하도록 구성했습니다.

---

## 핵심 구현과 기술 선택

| 기술 | 실제 적용 | 주요 문서 |
|---|---|---|
| **ActorComponent + Interface** | Health·Skill·Inventory·Pal 및 월드 상호작용의 역할 분리 | [[02. Gameplay Architecture|02_Gameplay_Architecture]] · [[06. World Interaction Systems|06_World_Interaction_Systems]] |
| **서버 스킬 실행** | `TryCastSkillById`, Phase별 비용·Notify·근접 Sweep 보간 | [[05. Combat, Skill & Animation|05_Combat_Skill_Animation]] |
| **FastArray / Replicated UObject** | 인벤토리 슬롯·스킬 요약 상태·스킬 실행 UObject·플레이어 기록 | [[12. Multiplayer Synchronization|12_Multiplayer_Synchronization]] |
| **협동 제작** | 단일 `ActiveWork`에 작업량 누적, 부분 완료·수령·미완료 재료 환불 | [[07. Inventory & Crafting|07_Inventory_Crafting]] |
| **Pal Lifecycle** | 포획 Reveal, OwnerOnly 목록, 같은 몬스터 Actor의 비활성화·소환·AI 재사용 | [[08. Pal Capture & Partner Lifecycle|08_Pal_Capture_Partner_Lifecycle]] |
| **데이터 기반 콘텐츠** | Dungeon Event/Action/Transition, Boss Pattern/Strike, 작성 단계 검증 | [[03. Data & Content Architecture|03_Data_Content_Architecture]] · [[13. Content Authoring & Validation|13_Content_Authoring_Validation]] |
| **Client Presentation** | 변경 Delegate와 던전 Snapshot → Presenter → ViewData → UMG | [[11. UI Architecture & Client Presentation|11_Client_State_Presentation_Pipeline]] |
| **Editor Automation** | AgentMcp로 Widget Blueprint·데이터 조사/편집, Compile·PIE·화면 확인 | [[14. Unreal Editor Automation & Verification|14_Unreal_Editor_Automation_Verification]] |

### 정의 데이터의 종류

- **DataTable:** Item·Skill·AreaObject·Level 등 같은 구조의 반복 데이터
- **`UDungeonDefinitionDataAsset : UPrimaryDataAsset`:** Stage·Event·Action·Transition·Grade를 포함한 던전 단위 정의
- **`UBossPatternDataAsset : UDataAsset`:** 패턴 선택 조건·Montage Cue·Telegraph·Strike 정의
- **GameplayTag:** 던전 Stage/Group/Branch/Barrier 및 보스 Pattern 등의 콘텐츠 식별자
- **Presentation DataAsset / StringTable:** 화면 이름·문구·아이콘과 표시 설정

정의 데이터는 콘텐츠 규칙을 담고, Runtime은 실행 중 변경되는 상태를 관리합니다. 두 데이터의 구분과 Asset 로드 흐름은 [[03. Data & Content Architecture|03_Data_Content_Architecture]]에 설명했습니다.

---

## 멀티플레이에서 상태를 나눈 방식

~~~mermaid
flowchart TB
    REQUEST["클라이언트 입력 · 서버 요청"] --> SERVER["서버 권위 Gameplay<br/>전투 · 제작 · 포획 · 던전"]
    SERVER --> COMPONENT["Actor / PlayerState 상태<br/>Health · Inventory · Skill · Quest"]
    SERVER --> RUN["Dungeon Runtime<br/>Stage · Objective · Branch"]
    COMPONENT --> REPLICATION["Property Replication<br/>FastArray · UObject Subobject"]
    RUN --> SNAPSHOT["GameState<br/>FDungeonStageRuntimeState"]
    REPLICATION --> UI["클라이언트 HUD · 화면"]
    SNAPSHOT --> PRESENTER["Presenter · ViewData"]
    PRESENTER --> UI
    SERVER -. "일회성 연출" .-> EFFECT["Multicast · Client RPC<br/>몽타주 · 알림"]
~~~

**서버가 최종 게임플레이 결과를 결정**합니다. 상태 종류에 따라 복제 범위와 화면 갱신 경로는 다르게 적용했습니다.

| 상태 | 소유·동기화 방식 |
|---|---|
| 아이템 슬롯·보유 Pal·도감·퀘스트 | PlayerState 컴포넌트의 플레이어별 상태, OwnerOnly 복제 |
| Skill Spec·실행 객체 | FastArray 요약 상태와 복제 `UBaseSkill`의 Phase·타겟·쿨타임 분리 |
| 현재 장비·캐릭터 외형 | 무기 선택과 Actor 상태를 복제하고 `OnRep`에서 적용 |
| 던전 진행 | 서버 `UDungeonStageRuntimeSubsystem` → GameState Run Snapshot |
| HUD·일회성 알림 | Delegate / Presenter / Notice 또는 연출 RPC로 표시 |

인벤토리 슬롯의 **클라이언트 예측 경로는 선택적으로 구현**돼 있으며 C++ 기본 설정은 비활성화입니다. Pal 슬롯 선택은 로컬 UI에 예상 선택을 먼저 표시하고 서버 결과로 확정합니다. 세부 요청·복제 범위는 [[12. Multiplayer Synchronization|12_Multiplayer_Synchronization]]에서 구분합니다.

---

## 통합 콘텐츠 — Forgotten Ruins

Forgotten Ruins는 **기존 시스템을 하나의 서버 주도 Run에서 결합한 분기형 던전**입니다.

### 두 갈래 진행



~~~text
던전 입장
   ↓
Guard Room / Wave A
   ├─ 레버 사용 → Shortcut → Boss
   └─ 레버 미사용 → Extra Wave → Boss
                                     ↓
                              Result · Reward
                              Best Time · Grade
~~~

**지름길 플레이**

https://github.com/user-attachments/assets/29858da6-7d13-4297-9d71-41f53272a952

레버는 `IInteractableInterface`로 입력을 받고 `ActorInteracted` 이벤트를 전달합니다. 서버 Runtime은 `RunTag`를 기록한 뒤 **Wave A 완료 시점에 Transition 조건을 평가**해 지름길 또는 추가 웨이브 경로를 선택합니다.

### 연결된 시스템

| 던전 기능 | 재사용하거나 확장한 구현 |
|---|---|
| 입구·레버 | 공통 Interaction Component와 Interface |
| 웨이브 목표 | Monster 처치·포획 이벤트와 ObjectiveTracker |
| 분기·장벽 | DataAsset의 Transition·`SealedBarriers`, GameState Snapshot |
| 수호자 보스 | 기존 Monster·Damage 위의 전용 FSM·PatternClock·Telegraph |
| HUD·미니맵·결과 | 복제된 현재 Run 상태 + 화면용 DataAsset → Presenter |
| 보상·기록 | Inventory `AddItem`, `UDungeonProgressSubsystem` SaveGame |

**던전 분기 플레이·Stage Graph**는 [[09. Branching Dungeon Runtime|09_Branching_Dungeon_Runtime]], **보스 시연과 패턴 실행**은 [[10. Boss Encounter Runtime|10_Boss_Encounter_Runtime]]에 있습니다.

---

## 콘텐츠 작성과 화면 구현

### 게임플레이 기록에서 화면까지

몬스터 도감과 퀘스트는 전투·포획·제작 등의 서버 이벤트를 기록하고, 플레이어별 진행을 OwnerOnly 상태로 전달합니다.

- [[16. MonsterDex System|16_MonsterDex_System]] — 처치·포획 기록, 도감 등록·보상, 3D 프리뷰
- [[17. Quest System|17_Quest_System]] — 목표 종류별 집계, NPC 수락·보고, 보상·월드 오브젝트 연동
- [[18. UMG v2 Migration & Editor Verification|18_UMG_v2_Migration]] — 메뉴 입력·커서, Feed/Headline/Title 알림, Widget Blueprint 개편

### 콘텐츠 정의와 검증

던전 정의는 `UDungeonDefinitionDataAsset::ValidateDefinition`으로 Stage 참조·분기 순서·도달 가능성·Group 생산 경로를 확인합니다. 보스 패턴은 경고 시간·영역·Montage Section 조합을 `UBossPatternDataAsset::Validate`로 검사합니다.

`BuildStageGraph`는 현재 던전 Definition에서 Mermaid Stage Graph를 생성합니다. 에디터 검증 사례는 [[13. Content Authoring & Validation|13_Content_Authoring_Validation]]에, AgentMcp 작업 사례는 [[14. Unreal Editor Automation & Verification|14_Unreal_Editor_Automation_Verification]]에 정리했습니다.

---

## 기술 문서 바로가기

| 먼저 보고 싶은 내용 | 문서 |
|---|---|
| 시스템 전체 구조 | [[02. Gameplay Architecture|02_Gameplay_Architecture]] |
| 전투·Skill / Inventory·Crafting / Pal | [[05. Combat, Skill & Animation|05_Combat_Skill_Animation]] · [[07. Inventory & Crafting|07_Inventory_Crafting]] · [[08. Pal Capture & Partner Lifecycle|08_Pal_Capture_Partner_Lifecycle]] |
| 분기 던전·보스 | [[09. Branching Dungeon Runtime|09_Branching_Dungeon_Runtime]] · [[10. Boss Encounter Runtime|10_Boss_Encounter_Runtime]] |
| UI·데이터·멀티플레이 | [[03. Data & Content Architecture|03_Data_Content_Architecture]] · [[11. UI Architecture & Client Presentation|11_Client_State_Presentation_Pipeline]] · [[12. Multiplayer Synchronization|12_Multiplayer_Synchronization]] |
| 개발 과정의 실제 구조 변경 | [[15. Development History & Retrospective|15_Development_History_Retrospective]] |

전체 목록은 [[Wiki Home|Home]]에서 확인할 수 있습니다.

**Source:** [Sonheim 저장소](https://github.com/chungheonLee0325/Sonheim) · [Source-only Mirror](https://github.com/chungheonLee0325/Sonheim.Source) · [AgentMcp](https://github.com/chungheonLee0325/AgentMcp)
