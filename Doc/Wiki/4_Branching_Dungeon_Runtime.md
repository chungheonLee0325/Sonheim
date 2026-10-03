# 4. Case Study — Branching Dungeon Runtime

> **문제:** 스테이지, 분기, 선택 목표, 실패 조건, 보상과 결과 정산이 늘어날 때마다 dungeon-specific C++ flow를 추가하지 않고 콘텐츠를 확장하려면?

Sonheim의 Dungeon은 하나의 scripted level이 아니라, **데이터 정의를 서버 런타임이 해석하는 콘텐츠 시스템**으로 구현했습니다.

---

## 전체 흐름

```text
Dungeon Catalog
      ↓
PrimaryAssetId
      ↓
Dungeon Definition DataAsset
      ↓
Asset Preparation
      ↓
UDungeonStageRuntimeSubsystem (Authority)
      ↓
Event → Condition → Action → Transition
      ↓
FDungeonStageRuntimeState
      ↓
ASonheimGameState Replication
```

---

## Stage Definition

`FDungeonStageDefinition`은 한 Stage의 규칙을 정의합니다.

주요 요소:

- `StageId`
- Terminal Outcome
- Time Limit
- Sealed Barriers
- Event Rules

각 Event Rule은 다음을 데이터로 가집니다.

- 어떤 Event를 받을지
- 특정 SourceId에만 반응할지
- Run 중 한 번만 실행할지
- 어떤 Action을 실행할지
- 어느 Transition으로 이동할지

---

## Event → Condition → Action → Transition

Dungeon flow는 서로 다른 기능을 같은 pipeline으로 처리합니다.

### Event

예:

- StageEntered
- AreaEntered
- Monster/Wave progress
- Capture
- Switch interaction
- Stage timeout

### Condition

Transition은 현재 RunTag, objective 상태 등 runtime state를 평가합니다.

### Action

Action은 다음과 같은 gameplay 결과를 실행합니다.

- SpawnGroup
- Set / Clear RunTag
- EmitEvent
- GrantReward

새 콘텐츠 흐름은 이 building block을 조합해서 정의합니다.

---

## Branch

Forgotten Ruins는 하나의 직선 진행이 아니라 **Shortcut / ExtraWave** 경로로 갈립니다.

플레이어의 선택은 `SelectedBranchId`와 RunTag에 남고, 이후 Stage와 Reward rule에서 조건으로 재사용됩니다.

Barrier 역시 level actor에 stage별 hardcoding을 두지 않고 현재 Stage Definition의 `SealedBarriers`로부터 상태를 결정합니다.

---

## Objective Tracking

`UDungeonObjectiveTracker`는 runtime이 생성한 monster group을 추적합니다.

- Spawned
- Defeated
- Captured

포획되어 player partner가 된 monster는 전투에서 영구 이탈하므로 objective completion에도 반영합니다.

Actor가 예상치 못하게 사라지면 invalidation을 runtime에 전달해 진행 상태가 조용히 멈추지 않도록 합니다.

---

## Failure도 동일한 Runtime으로 처리

성공 flow와 별도로 임시 exception code를 쌓지 않고 failure 역시 run state의 일부로 관리합니다.

예:

- Stage timeout
- Run owner death
- Run owner logout
- Owner portal exit
- Objective target invalidation
- Definition/runtime error

Stage에 time limit이 있으면 서버 기준 deadline을 publish하고, UI countdown도 같은 서버 시각을 사용합니다.

---

## Result & Persistence

Run 종료 시 서버는 다음 결과를 기록합니다.

- elapsed time
- rewards
- defeated / captured
- selected branch
- clear / fail count
- best time
- grade

`UDungeonProgressSubsystem`은 Dungeon의 stable numeric number를 persistence key로 사용합니다.

Runtime identifier(GameplayTag)의 이름이 바뀌더라도 기존 SaveGame key가 바뀌지 않도록 역할을 분리했습니다.

---

## 왜 Snapshot을 Publish하는가

Runtime은 HUD callback을 직접 호출하지 않습니다.

StageId만 보내는 대신 **화면을 다시 구성하는 데 필요한 현재 run state 전체**를 snapshot으로 publish합니다.

그 결과:

- UI가 재생성돼도 현재 상태를 복구할 수 있고
- gameplay와 UMG lifecycle이 분리되며
- Listen Server와 Client가 같은 데이터 모델을 소비합니다.

UI 측 구조는 [[Multiplayer State & UI Pipeline|5_Multiplayer_State_UI_Pipeline]]에서 이어집니다.

---

## 관련 코드

- [Dungeon Runtime](https://github.com/chungheonLee0325/Sonheim/tree/main/Sonheim/Source/Sonheim/GameManager/Dungeon)
- [Dungeon Definition](https://github.com/chungheonLee0325/Sonheim/tree/main/Sonheim/Source/Sonheim/GameObject/Dungeon)
- [DungeonStageRuntimeSubsystem.cpp](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/GameManager/Dungeon/DungeonStageRuntimeSubsystem.cpp)
- [DungeonObjectiveTracker.h](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/GameManager/Dungeon/DungeonObjectiveTracker.h)
- [DungeonProgressSubsystem.h](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/GameManager/Dungeon/DungeonProgressSubsystem.h)
