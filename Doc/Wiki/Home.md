# Sonheim Engineering Wiki

> **UE5.5 / C++ 서버 권위 멀티플레이 액션 어드벤처**  
> 이 Wiki는 포트폴리오에서 더 깊게 확인하고 싶은 설계, 코드 흐름, 트레이드오프와 검증 근거를 설명합니다.

---

## Start Here

처음 보는 경우 아래 순서만 읽어도 현재 Sonheim의 기술적 범위를 파악할 수 있습니다.

1. [[Project Overview|1_Project_Overview]]
2. [[Architecture Overview|2_Architecture_Overview]]
3. [[Branching Dungeon Runtime|4_Branching_Dungeon_Runtime]]
4. [[Multiplayer State & UI Pipeline|5_Multiplayer_State_UI_Pipeline]]
5. [[Content Authoring & Validation|6_Content_Authoring_Validation]]

---

## Featured Engineering

### [[Branching Dungeon Runtime|4_Branching_Dungeon_Runtime]]

DataAsset에 정의된 Event / Condition / Action / Transition을 서버 runtime이 해석해 Shortcut / ExtraWave 분기, 실패, 보상, 기록과 결과까지 진행합니다.

### [[Multiplayer State & UI Pipeline|5_Multiplayer_State_UI_Pipeline]]

Server Runtime → GameState Snapshot → Presenter → ViewData → LocalPlayer Router → UMG로 gameplay state와 presentation lifecycle을 분리했습니다.

### [[Content Authoring & Validation|6_Content_Authoring_Validation]]

GameplayTag, EditCondition, Data Validation, CallInEditor 검사와 Stage Graph 생성으로 콘텐츠 오류를 runtime 이전에 찾습니다.

### [[Multiplayer Inventory & Crafting|8_Multiplayer_Inventory_Crafting]]

FastArray, client prediction/reconciliation, subscriber-based replication과 서버 권위 shared crafting lifecycle을 다룹니다.

### [[Boss Encounter Runtime|7_Boss_Encounter_Runtime]]

DataAsset 기반 Pattern, Telegraph, Phase, Break/Down, Capture Window와 Animation Section을 하나의 Boss Runtime으로 구성했습니다.

---

## Architecture

- [[Architecture Overview|2_Architecture_Overview]]
- [[Data & Content Architecture|3_Data_Content_Architecture]]

## Gameplay Systems

- [[Pal Capture & Partner Lifecycle|9_Pal_Capture_Partner_Lifecycle]]
- [[Combat, Skill & Animation|10_Combat_Skill_Animation]]
- [[Player & Character Systems|11_Player_Character_Systems]]
- [[World Interaction Systems|12_World_Interaction_Systems]]

## Engineering

- [[Development Workflow & Verification|13_Development_Workflow_Verification]]
- [[Development History & Retrospective|14_Development_History_Retrospective]]

---

## Repository

- [Main Repository](https://github.com/chungheonLee0325/Sonheim) — 전체 Unreal Engine 프로젝트
- [Source-only Mirror](https://github.com/chungheonLee0325/Sonheim.Source) — Source / Config / Docs 중심 코드 검토용

---

## Legacy Wiki

2025년에 작성한 세부 시스템 문서는 repository의 `Doc/Wiki`에 그대로 보존되어 있습니다.

새 Wiki에서는 동일 내용을 기능별로 평평하게 나열하지 않고, **현재 설계 기준으로 통합한 14개 문서만 기본 탐색 경로에 노출**합니다.
