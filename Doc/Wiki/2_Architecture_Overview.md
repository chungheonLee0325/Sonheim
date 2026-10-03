# 2. Architecture Overview

Sonheim의 아키텍처는 특정 패턴 이름보다 **상태 소유권, 객체 수명, 네트워크 경계, presentation 책임**을 기준으로 시스템을 나눕니다.

---

## 서버가 공유 상태를 소유한다

멀티플레이 결과에 영향을 주는 상태 변경은 서버가 확정합니다.

- Inventory / Container
- Crafting Station
- Pal Capture
- Dungeon Run
- Boss Combat

RPC는 요청과 순간적인 행위에, Replication은 현재 상태 복원에 사용합니다.

```text
Client Input
   ↓ RPC
Server Authority
   ↓ state mutation
Replicated State
   ↓
Client Presentation
```

Dungeon은 이 원칙을 가장 명확하게 보여줍니다. `UDungeonStageRuntimeSubsystem`이 authority에서 run을 진행하고, `ASonheimGameState`가 `FDungeonStageRuntimeState`를 복제합니다.

---

## Unreal Gameplay Framework 수명에 맞춘 배치

| 범위 | 책임 |
|---|---|
| GameInstance / GameInstanceSubsystem | 세션, 장기 진행 기록, asset loading |
| World / WorldSubsystem | 현재 월드의 dungeon runtime |
| GameState | 공유 replicated state |
| PlayerController | 연결 단위 요청과 client bridge |
| LocalPlayerSubsystem | 로컬 UI routing / notice |
| Pawn / ActorComponent | 캐릭터 기능과 상태 |

런타임 로직을 특정 Widget/Pawn의 수명에 종속시키지 않고, 상태의 실제 수명과 UE Framework의 객체 수명을 맞추는 것을 우선했습니다.

---

## Composition

공용 기능은 거대한 상속 계층보다 component로 분리합니다.

- `UHealthComponent`
- `ULevelComponent`
- `UConditionComponent`
- `UStaminaComponent`
- `USonheimSkillComponent`
- `UInventoryComponent`
- `UPalCaptureComponent`
- `UInteractionComponent`

공통 캐릭터 기반인 `AAreaObject`는 모든 기능을 직접 구현하기보다, 각 component가 자신의 상태와 책임을 소유하도록 구성합니다.

---

## 시스템 간 연결

시스템은 직접적인 Widget/Class coupling보다 아래 경계를 사용합니다.

- Delegate
- GameplayTag
- Interface
- Replicated snapshot
- Presenter / ViewData
- Subsystem API

```text
Definition
   ↓
Server Runtime
   ↓
Replicated Snapshot
   ↓
Presenter
   ↓
ViewData
   ↓
UMG
```

Dungeon Runtime은 HUD 클래스를 알지 못하고, HUD 역시 Transition 조건을 알 필요가 없습니다.

---

## 데이터 종류에 따라 저장 방식을 나눈다

초기 Sonheim은 DataTable 중심이었지만 현재는 용도에 따라 구분합니다.

- **DataTable**: ID 기반 대량 row 데이터
- **PrimaryDataAsset**: 수명과 dependency를 가진 콘텐츠 정의
- **GameplayTag**: 계층적 runtime identifier
- **Soft Reference**: 필요한 시점에 로드할 asset dependency
- **Config**: 프로젝트 전역 정책과 registry
- **StringTable**: 플레이어 노출 문구

자세한 내용은 [[Data & Content Architecture|3_Data_Content_Architecture]]에서 다룹니다.

---

## 기존 시스템이 새 콘텐츠에서 재사용되는 방식

아키텍처의 가치는 패턴 이름보다 **새 요구사항에서 기존 경계가 유지되는가**로 판단했습니다.

- 2025년 Interaction contract → 2026년 Dungeon Portal / Shortcut Switch에 재사용
- 기존 Capture pipeline → Guardian Boss의 취약 상태 포획에 재사용
- Inventory reward API → Dungeon completion reward에 재사용
- Monster death/capture delegate → Dungeon objective tracker에 연결
- 공용 NoticeSubsystem → Dungeon, Level-up, Capture, Crafting, Region title에 재사용

---

## 관련 문서

- [[Data & Content Architecture|3_Data_Content_Architecture]]
- [[Branching Dungeon Runtime|4_Branching_Dungeon_Runtime]]
- [[Multiplayer State & UI Pipeline|5_Multiplayer_State_UI_Pipeline]]
- [[Multiplayer Inventory & Crafting|8_Multiplayer_Inventory_Crafting]]
