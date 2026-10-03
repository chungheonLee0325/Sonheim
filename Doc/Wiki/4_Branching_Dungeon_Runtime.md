# 4. Branching Dungeon Runtime

> **이 문서가 답하는 질문**
>
> “분기, 목표, 제한시간, 실패, 보상 같은 Dungeon 규칙이 늘어날 때마다 C++에 Stage별 분기문을 추가하지 않고 콘텐츠를 확장하려면?”

Dungeon은 하나의 Level Script가 아니라 **Definition을 Server Runtime이 해석하는 콘텐츠 시스템**입니다.

먼저 이 구조만 보면 됩니다.

```text
Catalog Row
   ↓
PrimaryAssetId
   ↓
UDungeonDefinitionDataAsset
   ↓
FDungeonStageDefinition[]
   ↓
UDungeonStageRuntimeSubsystem
   ↓
FDungeonStageRuntimeState
   ↓
GameState Replication
```

---

# Part 1. Dungeon 하나는 무엇으로 정의되는가

## 1.1 Catalog는 “어떤 Dungeon인가”만 찾는다

실제 `FDungeonCatalogRow`:

```cpp
USTRUCT(BlueprintType)
struct FDungeonCatalogRow : public FTableRowBase
{
    GENERATED_BODY()

    FGameplayTag DungeonId;
    int32 DungeonNumber = 0;
    FPrimaryAssetId DefinitionAssetId;
    int32 RequiredLevel = 1;
};
```

- `DungeonId`: Runtime / Tag namespace
- `DungeonNumber`: SaveGame에서 사용하는 stable numeric key
- `DefinitionAssetId`: 실제 콘텐츠 Definition
- `RequiredLevel`: 입장 조건

Catalog 자체에 모든 Stage를 넣지 않습니다.

---

## 1.2 Definition은 Stage Graph를 소유한다

```cpp
UCLASS(BlueprintType)
class UDungeonDefinitionDataAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    FGameplayTag DungeonId;
    FGameplayTag StartStageId;
    TSoftObjectPtr<UDungeonPresentationDataAsset> Presentation;
    TArray<FDungeonStageDefinition> Stages;
    TArray<FDungeonGradeRule> GradeRules;
};
```

Dungeon 하나의:

- 시작 Stage
- 전체 Stage graph
- 결과 Grade rule
- Presentation dependency

를 하나의 Primary Asset 단위로 묶습니다.

---

# Part 2. Stage는 어떻게 표현되는가

## 2.1 핵심 Building Block

현재 Stage 규칙은 네 종류의 type으로 나뉩니다.

```cpp
enum class EDungeonStageEvent : uint8
{
    StageEntered,
    WaveCompleted,
    BossDefeated,
    ActorInteracted,
    StageTimeout,
    AreaEntered,
    MonsterCaptured
};

enum class EDungeonStageCondition : uint8
{
    Always,
    HasRunTag,
    SpawnGroupCompleted
};

enum class EDungeonStageAction : uint8
{
    SpawnGroup,
    SetRunTag,
    ClearRunTag,
    EmitEvent,
    GrantReward
};
```

읽는 방법은 단순합니다.

```text
Event가 들어오면
 → Action을 실행하고
 → Condition을 만족하는 Transition을 찾아
 → 다음 Stage로 이동한다
```

---

## 2.2 실제 `FDungeonStageDefinition`

```cpp
USTRUCT(BlueprintType)
struct FDungeonStageDefinition
{
    GENERATED_BODY()

    FGameplayTag StageId;
    EDungeonTerminalOutcome TerminalOutcome = EDungeonTerminalOutcome::None;

    float TimeLimitSeconds = 0.f;

    TArray<FGameplayTag> SealedBarriers;

    TArray<FDungeonStageEventRule> EventRules;
};
```

Stage가 직접 갖는 정보는 크게:

- 정체성
- Terminal 여부
- 제한시간
- 현재 막아야 할 Door/Barrier
- Event에 대한 Rule

입니다.

---

# Part 3. 실제 Runtime은 무엇을 저장하는가

Definition은 “규칙”이고 Runtime State는 “이번 Run에서 실제로 무슨 일이 일어났는가”입니다.

대표 필드를 발췌하면:

```cpp
USTRUCT(BlueprintType)
struct FDungeonStageRuntimeState
{
    GENERATED_BODY()

    FGuid RunId;
    FPrimaryAssetId DefinitionAssetId;
    FGameplayTag StageId;
    int32 Revision = 0;
    EDungeonRunStatus RunStatus = EDungeonRunStatus::Idle;

    FGameplayTag ObjectiveGroupId;
    int32 CurrentCount = 0;
    int32 RequiredCount = 0;

    FGameplayTag SelectedBranchId;
    FGameplayTagContainer RunTags;

    double StageStartedServerTime = 0;
    double StageDeadlineServerTime = 0;

    TArray<FGameplayTag> SealedBarriers;
    TArray<FDungeonRunReward> Rewards;
    TArray<FDungeonGroupTally> Groups;

    int32 DefeatedCount = 0;
    int32 CapturedCount = 0;

    TArray<TObjectPtr<APlayerState>> Participants;
    TObjectPtr<APlayerState> OwnerPlayer;

    float BossHealth = 0.f;
    FGameplayTag BossActionId;
    int32 BossPhase = 0;
    bool bBossVulnerable = false;
    float BossBreak = 0.f;
};
```

Definition과 Runtime을 분리했기 때문에 같은 Definition으로 여러 Run을 시작해도 각 Run의 state는 별도로 존재할 수 있습니다.

---

# Part 4. 한 Stage Event가 처리되는 방식

예를 들어 어떤 Switch를 사용했다고 가정하면:

```text
ADungeonShortcutSwitch
   ↓ Server interaction
ActorInteracted(SourceId)
   ↓
UDungeonStageRuntimeSubsystem
   ↓
현재 Stage의 EventRules 검색
   ↓
SourceId 일치 확인
   ↓
Actions 실행
   ├─ SetRunTag
   └─ EmitEvent ...
   ↓
Transitions 평가
   ↓
NextStageId / BranchId 결정
```

World Actor는 “Shortcut을 열면 Combat Stage 다음에 어떤 Stage로 가야 하는가”를 알지 않습니다.

그 규칙은 Definition이 소유합니다.

---

# Part 5. Branch를 어떻게 기억하는가

Forgotten Ruins는 Shortcut / ExtraWave 경로를 갖습니다.

Branch 선택 결과는:

- `SelectedBranchId`
- `RunTags`

에 남습니다.

이 state는 이후:

- 다음 Transition
- Barrier
- Reward
- HUD
- Result

에서 재사용됩니다.

분기를 한 번 선택한 사실을 서로 다른 시스템이 각자 bool로 중복 저장하지 않습니다.

---

# Part 6. Objective는 Spawn 결과를 추적한다

`UDungeonObjectiveTracker`는 Runtime이 생성한 group을 기준으로:

- Spawned
- Defeated
- Captured

를 집계합니다.

Capture가 중요한 이유는 Monster가 죽지 않고 Player Partner가 되어도 전투에서는 영구 이탈하기 때문입니다.

따라서 Capture 역시 objective completion에 포함됩니다.

---

# Part 7. 실패도 별도 예외가 아니라 Run State다

Failure reason:

```cpp
enum class EDungeonFailReason : uint8
{
    None,
    TimeOut,
    OwnerDown,
    OwnerLeft,
    TargetLost,
    Error
};
```

실패 원인도 Runtime State 안에 들어가므로 Result UI와 Save 기록이 같은 state를 사용합니다.

---

# Part 8. Barrier는 Stage Definition이 소유한다

초기 구현처럼 Barrier Actor가 “Combat Stage면 닫는다”를 직접 알게 하지 않습니다.

현재 Stage Definition의:

```cpp
TArray<FGameplayTag> SealedBarriers;
```

가 authoritative rule이고, Snapshot이 현재 sealed list를 전달합니다.

Barrier Actor는 Snapshot을 읽고 자신의 `BarrierId`가 포함됐는지만 판단합니다.

```text
Definition
 → Runtime State.SealedBarriers
 → GameState
 → ADungeonBarrier
```

---

# Part 9. Result / Persistence

Run 종료 시 Runtime은:

- elapsed
- rewards
- defeated / captured
- route
- clear count
- best
- grade

를 기록합니다.

SaveGame key는 `DungeonNumber`를 사용합니다.

GameplayTag는 이름이 바뀔 수 있지만 Save key는 바뀌면 안 되기 때문에 identity 역할을 분리했습니다.

---

# Part 10. 왜 Snapshot을 발행하는가

Runtime이 Widget 함수를 직접 호출하지 않습니다.

Snapshot 하나가:

- World Barrier
- Shortcut Gate
- UI Presenter
- Result
- Boss HUD

같은 여러 소비자에게 현재 authoritative state를 제공합니다.

UI에서 이 Snapshot을 어떻게 ViewData로 바꾸는지는 [[5. Multiplayer State & UI Pipeline|5_Multiplayer_State_UI_Pipeline]]에서 이어집니다.

---

# 이 문서 다음에 읽기

- Definition이 어떤 데이터 구조로 관리되는지 → [[3. Data & Content Architecture|3_Data_Content_Architecture]]
- Boss Stage 내부 구조 → [[7. Boss Encounter Runtime|7_Boss_Encounter_Runtime]]
- Runtime State가 UI로 가는 과정 → [[5. Multiplayer State & UI Pipeline|5_Multiplayer_State_UI_Pipeline]]
- Definition 오류를 Editor에서 잡는 방법 → [[6. Content Authoring & Validation|6_Content_Authoring_Validation]]

---

## 관련 코드

- [DungeonDefinitionDataAsset.h](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/GameObject/Dungeon/DungeonDefinitionDataAsset.h)
- [DungeonStageDefinition.h](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/GameObject/Dungeon/DungeonStageDefinition.h)
- [DungeonStageRuntimeTypes.h](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/GameManager/Dungeon/DungeonStageRuntimeTypes.h)
- [DungeonStageRuntimeSubsystem.cpp](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/GameManager/Dungeon/DungeonStageRuntimeSubsystem.cpp)
