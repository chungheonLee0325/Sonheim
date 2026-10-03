# 1. Project Overview

Sonheim은 **Unreal Engine 5.5 / C++ 기반 서버 권위 멀티플레이 액션 어드벤처** 프로젝트입니다.  
수집·전투·포획·제작·협동이라는 기본 게임 루프 위에, 현재는 **데이터로 정의되는 분기형 던전 콘텐츠 런타임**까지 확장되어 있습니다.

이 Wiki는 기능 목록을 모두 앞에 나열하기보다, 실제 구현을 이해하는 데 필요한 **아키텍처 → 콘텐츠 런타임 → 네트워크 상태 → UI → 검증** 흐름을 중심으로 정리합니다.

---

## 현재 프로젝트의 핵심

### Data-driven Content Runtime
던전은 코드에 순서를 하드코딩하지 않고 `UDungeonDefinitionDataAsset`에 스테이지, 이벤트, 조건, 액션, 전이, 보상과 제한 시간을 정의합니다. 서버 런타임은 이 정의를 해석해 현재 run state를 진행합니다.

### Server-authoritative Multiplayer
게임플레이 판정과 공유 상태 변경은 서버가 소유합니다. 지속 상태는 Replication으로 전달하고, 사용자 요청이나 일회성 행위는 RPC로 전달한다는 원칙을 Inventory, Crafting, Capture, Dungeon 등에 일관되게 적용했습니다.

### Replicated State → Presentation
던전 서버 런타임은 UI 위젯을 직접 조작하지 않습니다. 서버의 runtime state를 `ASonheimGameState`가 복제하고, 클라이언트의 Presenter가 이를 UI용 ViewData로 변환한 뒤 LocalPlayer 단위 Router가 UMG에 반영합니다.

### Authoring & Validation
GameplayTag 계층, PrimaryAsset, soft reference, EditCondition/TitleProperty, Data Validation, CallInEditor 검사를 이용해 콘텐츠 정의를 코드 수정 없이 편집하고 잘못된 정의를 실행 전에 찾을 수 있도록 구성했습니다.

---

## 주요 시스템

| 영역 | 구현 |
|---|---|
| 콘텐츠 런타임 | 분기형 던전, 이벤트/조건/액션/전이, 시간 제한, 실패 경로, 보상, 기록/등급 |
| 멀티플레이 | 서버 권위, GameState snapshot, RPC/Replication 분리, FastArray |
| UI | Presenter/ViewData, LocalPlayer Router, HUD/Result, world marker, minimap, 공용 Notice |
| 콘텐츠 제작 | DataTable + PrimaryDataAsset + GameplayTag + validation |
| 전투 | 데이터 기반 Skill/Attack, AnimNotify timing, 근접/투사체, Boss pattern |
| 플레이어 시스템 | Inventory, Stat, Equipment, Capture, Partner |
| 월드 시스템 | Interaction, Item, Resource, Container, Crafting |
| 개발 워크플로 | Agent MCP 기반 UE Editor 자동화 및 반복 검증 |

---

## 추천 탐색 순서

처음 프로젝트를 보는 경우 아래 순서를 권장합니다.

1. [[Architecture Overview|2_Architecture_Overview]]
2. [[Branching Dungeon Runtime|4_Branching_Dungeon_Runtime]]
3. [[Multiplayer State & UI Pipeline|5_Multiplayer_State_UI_Pipeline]]
4. [[Content Authoring & Validation|6_Content_Authoring_Validation]]
5. [[Multiplayer Inventory & Crafting|8_Multiplayer_Inventory_Crafting]]

기존 게임플레이 구현이 궁금하다면 이후 시스템 reference 문서에서 Combat, Player, Capture, Interaction을 확인할 수 있습니다.

---

## Source

- [Main Repository](https://github.com/chungheonLee0325/Sonheim)
- [Source-only Mirror](https://github.com/chungheonLee0325/Sonheim.Source)
