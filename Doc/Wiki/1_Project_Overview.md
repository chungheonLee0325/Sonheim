# 1. Project Overview

Sonheim은 **Unreal Engine 5.5 / C++ 기반 서버 권위 멀티플레이 액션 어드벤처** 프로젝트입니다.

수집·전투·포획·제작·협동이라는 기본 gameplay loop를 구현한 뒤, 그 시스템들이 하나의 콘텐츠 안에서 함께 동작하도록 **분기형 Dungeon Vertical Slice**까지 확장했습니다.

---

## Gameplay Loop

```text
Explore
  ↓
Combat / Harvest
  ↓
Item / Resource
  ↓
Inventory / Crafting
  ↓
Character / Pal Growth
  ↓
Dungeon / Boss
  ↓
Reward / Record
```

각 기능은 독립적으로 존재하는 데서 끝나지 않고 서로 같은 authoritative state와 data layer를 사용합니다.

예를 들어 Dungeon Reward는 Inventory에 들어가고, Guardian Capture는 Pal Capture lifecycle을 사용하며, Dungeon Portal/Lever는 일반 Interaction system을 사용합니다.

---

## 현재 프로젝트의 핵심 기술 영역

### Multiplayer Gameplay

- Server-authoritative state mutation
- RPC / Replication 역할 분리
- Owner-only FastArray
- Subscriber-based replication
- Client Prediction / Reconciliation
- Listen Server + Client

### Data & Content

- DataTable
- PrimaryDataAsset
- PrimaryAssetId
- GameplayTag
- Soft Reference
- StringTable
- Editor Validation

### Gameplay Systems

- Player / Attribute / Stat
- Skill / Combat / Animation
- Inventory / Equipment
- Pal Capture / Partner AI
- Interaction / Item / Resource
- Container / Crafting
- Dungeon / Boss

### Presentation

- Delegate-based HUD
- Replicated Snapshot
- Presenter / ViewData
- LocalPlayer UI Router
- Notice subsystem
- World marker / Minimap / Result

---

## Dungeon Vertical Slice

Forgotten Ruins Dungeon은 프로젝트의 현재 구조를 가장 밀도 있게 보여주는 콘텐츠입니다.

```text
Definition
   ↓
Server Runtime
   ↓
World / Monster / Boss
   ↓
Replicated State
   ↓
HUD / Minimap / Result
   ↓
Reward / SaveGame
```

포함 기능:

- Shortcut / ExtraWave 분기
- Objective / Optional Objective
- Stage Timer / Failure
- Barrier
- Guardian Boss
- Boss Capture
- Reward Chest
- World Marker
- Minimap
- Best Record / Grade
- SaveGame Progress

---

## 문서 탐색 순서

프로젝트 구조를 먼저 보고 싶다면:

1. [[Architecture Overview|2_Architecture_Overview]]
2. [[Data & Content Architecture|3_Data_Content_Architecture]]
3. [[Branching Dungeon Runtime|4_Branching_Dungeon_Runtime]]
4. [[Multiplayer State & UI Pipeline|5_Multiplayer_State_UI_Pipeline]]
5. [[Content Authoring & Validation|6_Content_Authoring_Validation]]

기존 gameplay foundation을 더 깊게 보려면:

- [[Multiplayer Inventory & Crafting|8_Multiplayer_Inventory_Crafting]]
- [[Pal Capture & Partner Lifecycle|9_Pal_Capture_Partner_Lifecycle]]
- [[Combat, Skill & Animation|10_Combat_Skill_Animation]]
- [[Player & Character Systems|11_Player_Character_Systems]]
- [[World Interaction Systems|12_World_Interaction_Systems]]

---

## Source

- [Main Repository](https://github.com/chungheonLee0325/Sonheim)
- [Source-only Mirror](https://github.com/chungheonLee0325/Sonheim.Source)
