# Sonheim Engineering Wiki

> **Unreal Engine 5.5 / C++ 기반 서버 권위 멀티플레이 액션 어드벤처**
>
> 이 Wiki는 기능 목록보다 **어떤 문제를 어떤 상태 모델과 데이터 흐름으로 해결했는지**를 중심으로 설명합니다.

---

# 처음 보는 사람이라면

프로젝트 전체를 처음 보는 리뷰어라면 아래 4개만 먼저 읽어도 됩니다.

1. [[1. Project Overview|1_Project_Overview]]  
   프로젝트가 어떤 게임이고 무엇을 구현했는지

2. [[2. Architecture Overview|2_Architecture_Overview]]  
   Server Authority, Replication, UE Gameplay Framework에서 책임을 어떻게 나눴는지

3. [[4. Branching Dungeon Runtime|4_Branching_Dungeon_Runtime]]  
   기존 시스템들을 실제 콘텐츠 하나로 묶은 대표 Vertical Slice

4. [[5. Multiplayer State & UI Pipeline|5_Multiplayer_State_UI_Pipeline]]  
   서버 상태가 Client UI까지 어떻게 전달되는지

여기까지 읽은 뒤 관심 영역으로 내려가는 것을 권장합니다.

---

# 시스템 지도

```text
                   ┌──────────────────────────┐
                   │ 2. Architecture Overview │
                   │ 3. Data & Content        │
                   └────────────┬─────────────┘
                                │
         ┌──────────────────────┼──────────────────────┐
         │                      │                      │
┌────────▼────────┐   ┌─────────▼──────────┐  ┌───────▼──────────┐
│11. Player /     │   │10. Combat / Skill │  │12. World         │
│Character       │   │/ Animation        │  │Interaction       │
└────────┬────────┘   └─────────┬──────────┘  └───────┬──────────┘
         │                      │                      │
         ├──────────────┬───────┴──────────────┬───────┤
         │              │                      │       │
┌────────▼────────┐ ┌───▼────────────────┐ ┌───▼───────▼──────┐
│8. Inventory /  │ │9. Pal Capture /    │ │4. Branching     │
│Crafting        │ │Partner Lifecycle   │ │Dungeon Runtime  │
└────────────────┘ └──────────┬──────────┘ └───────┬──────────┘
                              │                    │
                              └──────────┬─────────┘
                                         │
                    ┌────────────────────┼────────────────────┐
                    │                    │                    │
             ┌──────▼──────┐     ┌───────▼───────┐    ┌──────▼────────┐
             │7. Boss      │     │5. State & UI │    │6. Authoring & │
             │Encounter    │     │Pipeline      │    │Validation     │
             └─────────────┘     └───────────────┘    └───────────────┘
```

Dungeon은 별도 데모가 아니라, 아래 기반 시스템들이 실제로 함께 동작하는 지점입니다.

- Player / Character
- Combat / Skill / Animation
- Interaction
- Inventory / Crafting
- Pal Capture / Partner AI

---

# 관심 영역별 읽기 경로

## 멀티플레이 / 네트워킹을 보고 싶다면

```text
2. Architecture
 → 8. Inventory & Crafting
 → 5. Multiplayer State & UI
 → 9. Pal Capture
```

주요 주제:

- Server Authority
- RPC vs Replication
- FastArray
- Owner-only replication
- Subscriber-based replication
- Client Prediction / Reconciliation
- Replicated Snapshot

---

## 전투 / 게임플레이 구조를 보고 싶다면

```text
11. Player & Character
 → 10. Combat, Skill & Animation
 → 9. Pal Capture
 → 7. Boss Encounter
```

주요 주제:

- Data / State / Logic 분리
- Skill lifecycle
- Animation-driven timing
- Melee sweep interpolation
- Damage context
- Capture lifecycle
- Boss Pattern Runtime

---

## 콘텐츠 시스템 / 에디터 툴링을 보고 싶다면

```text
3. Data & Content
 → 4. Branching Dungeon Runtime
 → 6. Content Authoring & Validation
 → 13. Development Workflow & Verification
```

주요 주제:

- DataTable / PrimaryDataAsset 역할 분리
- GameplayTag 기반 ID
- Event / Condition / Action / Transition
- Data Validation
- Stage Graph
- Editor automation

---

# 대표 문서

### [[4. Branching Dungeon Runtime|4_Branching_Dungeon_Runtime]]

분기, 목표, 시간 제한, 실패, 보상과 결과 정산을 C++ stage switch문으로 만들지 않고  
**Definition을 Server Runtime이 해석하는 구조**로 구현했습니다.

### [[10. Combat, Skill & Animation|10_Combat_Skill_Animation]]

`FSkillData`, FastArray Skill Spec, `UBaseSkill`, AnimNotify, Melee Trace와 Damage Pipeline이  
한 번의 공격에서 어떻게 연결되는지 실제 타입과 코드 흐름으로 설명합니다.

### [[8. Multiplayer Inventory & Crafting|8_Multiplayer_Inventory_Crafting]]

개인 Inventory와 공유 Container/Crafting Station이 같은 Item 데이터를 사용하면서도  
왜 서로 다른 replication policy를 갖는지 설명합니다.

### [[5. Multiplayer State & UI Pipeline|5_Multiplayer_State_UI_Pipeline]]

Dungeon Server Runtime의 상태를 Widget에 직접 밀어 넣지 않고  
**Replicated Snapshot → Presenter → ViewData → LocalPlayer UI**로 변환하는 과정을 설명합니다.

---

# 전체 문서

## Architecture Foundation
- [[1. Project Overview|1_Project_Overview]]
- [[2. Architecture Overview|2_Architecture_Overview]]
- [[3. Data & Content Architecture|3_Data_Content_Architecture]]

## Core Gameplay Foundations
- [[11. Player & Character Systems|11_Player_Character_Systems]]
- [[10. Combat, Skill & Animation|10_Combat_Skill_Animation]]
- [[12. World Interaction Systems|12_World_Interaction_Systems]]
- [[8. Multiplayer Inventory & Crafting|8_Multiplayer_Inventory_Crafting]]
- [[9. Pal Capture & Partner Lifecycle|9_Pal_Capture_Partner_Lifecycle]]

## Dungeon Vertical Slice
- [[4. Branching Dungeon Runtime|4_Branching_Dungeon_Runtime]]
- [[7. Boss Encounter Runtime|7_Boss_Encounter_Runtime]]
- [[5. Multiplayer State & UI Pipeline|5_Multiplayer_State_UI_Pipeline]]
- [[6. Content Authoring & Validation|6_Content_Authoring_Validation]]

## Engineering Evidence
- [[13. Development Workflow & Verification|13_Development_Workflow_Verification]]
- [[14. Development History & Retrospective|14_Development_History_Retrospective]]

---

# Source

- [Sonheim](https://github.com/chungheonLee0325/Sonheim) — 전체 Unreal Engine 프로젝트
- [Sonheim.Source](https://github.com/chungheonLee0325/Sonheim.Source) — Source / Config / Docs 중심 코드 검토용
- [AgentMcp](https://github.com/chungheonLee0325/AgentMcp) — Unreal Editor automation / agent integration
