# Sonheim Engineering Wiki

> **Unreal Engine 5.5 / C++ 기반 서버 권위 멀티플레이 액션 어드벤처**
>
> 수집·전투·포획·제작·협동 시스템과, 그 위에 구축한 **데이터 기반 분기형 던전 Vertical Slice**의 설계와 구현을 정리합니다.

---

## 빠르게 보기

프로젝트의 현재 구조를 파악하려면 아래 순서로 보는 것을 권장합니다.

1. [[Project Overview|1_Project_Overview]]
2. [[Architecture Overview|2_Architecture_Overview]]
3. [[Branching Dungeon Runtime|4_Branching_Dungeon_Runtime]]
4. [[Multiplayer State & UI Pipeline|5_Multiplayer_State_UI_Pipeline]]
5. [[Content Authoring & Validation|6_Content_Authoring_Validation]]

---

## Featured Engineering

### [[Branching Dungeon Runtime|4_Branching_Dungeon_Runtime]]

Dungeon을 Stage 순서가 하드코딩된 Level Script가 아니라, **Event / Condition / Action / Transition을 데이터로 정의하고 서버가 해석하는 콘텐츠 런타임**으로 구성했습니다. Shortcut / ExtraWave 분기, 제한시간, 실패, 보상, 기록과 결과 정산까지 같은 모델에서 처리합니다.

### [[Multiplayer State & UI Pipeline|5_Multiplayer_State_UI_Pipeline]]

서버 gameplay state와 UMG lifecycle을 분리하기 위해 **Server Runtime → Replicated Snapshot → Presenter → ViewData → LocalPlayer UI Router** 흐름을 구성했습니다. 일반 HUD의 delegate 기반 갱신과 Dungeon의 snapshot 기반 presentation을 각각 상태 성격에 맞게 사용합니다.

### [[Content Authoring & Validation|6_Content_Authoring_Validation]]

GameplayTag, PrimaryAsset, EditCondition, Data Validation, CallInEditor 검사와 Definition 기반 Stage Graph를 이용해 **콘텐츠 오류를 실행 전에 발견하고 흐름을 검토할 수 있는 authoring 환경**을 구성했습니다.

### [[Multiplayer Inventory & Crafting|8_Multiplayer_Inventory_Crafting]]

Owner-only FastArray, client prediction/reconciliation, subscriber-based Container replication, shared Crafting state와 동시성 제어를 통해 **개인 소유 상태와 공유 월드 상태를 서로 다른 replication policy로 처리**합니다.

### [[Boss Encounter Runtime|7_Boss_Encounter_Runtime]]

Grizzbolt Guardian의 Pattern, Telegraph, Phase, Down/Exhaust, Capture Window와 Montage Section을 데이터와 상태 머신으로 구성해 Dungeon의 Boss Stage에 연결했습니다.

---

## Architecture

- [[1. Project Overview|1_Project_Overview]]
- [[2. Architecture Overview|2_Architecture_Overview]]
- [[3. Data & Content Architecture|3_Data_Content_Architecture]]

## Gameplay & Systems

- [[8. Multiplayer Inventory & Crafting|8_Multiplayer_Inventory_Crafting]]
- [[9. Pal Capture & Partner Lifecycle|9_Pal_Capture_Partner_Lifecycle]]
- [[10. Combat, Skill & Animation|10_Combat_Skill_Animation]]
- [[11. Player & Character Systems|11_Player_Character_Systems]]
- [[12. World Interaction Systems|12_World_Interaction_Systems]]

## Engineering

- [[13. Development Workflow & Verification|13_Development_Workflow_Verification]]
- [[14. Development History & Retrospective|14_Development_History_Retrospective]]

---

## Source

- [Sonheim](https://github.com/chungheonLee0325/Sonheim) — 전체 Unreal Engine 프로젝트
- [Sonheim.Source](https://github.com/chungheonLee0325/Sonheim.Source) — Source / Config / Docs 중심 코드 검토용
- [AgentMcp](https://github.com/chungheonLee0325/AgentMcp) — Unreal Editor automation / agent integration
