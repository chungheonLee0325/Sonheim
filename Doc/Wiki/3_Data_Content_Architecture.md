# 3. Data & Content Architecture

> **핵심 구현 범위**
>
> “왜 어떤 데이터는 DataTable이고, 어떤 콘텐츠는 PrimaryDataAsset이며, Runtime ID는 GameplayTag인가?”

Sonheim은 모든 데이터를 한 형태로 통일하지 않습니다.  
**조회 방식, 수명, dependency, editor workflow**가 다르면 표현 방식도 달라집니다.


> **코드 예시:** 실제 구현에서 구조 이해에 필요한 선언과 함수만 발췌했으며, `UPROPERTY` metadata와 보조 필드는 일부 생략했습니다.
---

## Part 1. 먼저 구분해야 할 네 종류

| 문제 | 사용 방식 |
|---|---|
| 같은 schema의 row가 많고 ID로 자주 조회 | DataTable |
| 자체 identity와 asset dependency가 있는 콘텐츠 | PrimaryDataAsset |
| 계층적인 runtime identifier | GameplayTag |
| 당장 load할 필요가 없는 asset dependency | Soft Reference |

이 구분을 이해하면 Item/Skill과 Dungeon이 왜 다른 data model을 갖는지 설명됩니다.

---

## Part 2. DataTable — 대량 Row 데이터

대표적으로:

- Item
- Skill
- AreaObject
- Level
- Resource
- Container

가 DataTable row입니다.

예를 들어 Skill row는 실제로 다음 정보를 갖습니다.

```cpp
USTRUCT(BlueprintType)
struct FSkillData : public FTableRowBase
{
    GENERATED_USTRUCT_BODY()

    int SkillID = 0;
    TSubclassOf<UBaseSkill> SkillClass = nullptr;

    TArray<FSkillStaminaCost> StaminaCosts;
    TArray<FSkillItemCost> ItemCosts;

    float CastRange = 0.0f;
    float CoolTime = 0.0f;

    UAnimMontage* Montage = nullptr;
    TArray<FAttackData> AttackData;

    int NextSkillID = 0;
};
```

Skill마다 독립 asset lifecycle이 필요한 것이 아니라 같은 schema를 가진 row를 ID로 반복 조회하므로 DataTable이 자연스럽습니다.

Skill 자체의 실행 구조는 [[10. Combat, Skill & Animation|10_Combat_Skill_Animation]]에서 설명합니다.

---

## Part 3. Dungeon은 왜 PrimaryDataAsset인가

Dungeon은 “row 하나”보다 하나의 **콘텐츠 패키지**에 가깝습니다.

필요한 것:

- 자체 identity
- 여러 Stage의 nested graph
- Presentation asset
- Spawn Rule
- Validation
- 필요 시점 asset loading

그래서 Catalog와 실제 Definition을 분리합니다.

### 3.1 Catalog Row

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

Catalog는 “어떤 Dungeon을 열 것인가”를 찾는 작은 index입니다.

실제 Stage graph는 넣지 않습니다.

---

### 3.2 Definition Asset

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

이 asset이 Dungeon 콘텐츠의 authoring unit입니다.

```text
Catalog Row
   ↓ PrimaryAssetId
Dungeon Definition
   ├─ Stage Graph
   ├─ Grade Rules
   └─ Soft Presentation Reference
```

---

## Part 4. GameplayTag — Runtime Identifier

Dungeon의:

- Stage
- Group
- Branch
- Barrier
- Event Source

와 Boss Pattern은 GameplayTag 계층을 사용합니다.

예:

```text
Dungeon.ForgottenRuins.Stage.*
Dungeon.ForgottenRuins.Group.*
Dungeon.ForgottenRuins.Branch.*
Dungeon.ForgottenRuins.Barrier.*

Boss.Grizzbolt.Pattern.*
```

### 왜 enum이 아닌가

전역 enum에 콘텐츠별 값을 계속 추가하면:

- 서로 다른 Dungeon ID가 한 namespace에 섞이고
- 콘텐츠 추가마다 C++ enum 수정이 필요하며
- editor에서 유효 category를 제한하기 어렵습니다.

GameplayTag는 콘텐츠별 namespace와 editor category filter를 함께 사용할 수 있습니다.

---

## Part 5. Stable Save ID와 Runtime ID는 다르다

Dungeon Record의 key는 `DungeonNumber`를 사용합니다.

왜 GameplayTag를 그대로 저장 key로 쓰지 않는가?

GameplayTag 이름은 콘텐츠 정리 중 rename할 수 있습니다.  
반면 SaveGame key가 바뀌면 기존 기록을 잃습니다.

따라서:

```text
DungeonId (GameplayTag)
→ Runtime / Authoring Identity

DungeonNumber (int32)
→ Persistent Save Identity
```

로 역할을 나눕니다.

---

## Part 6. Soft Reference — Dependency를 Load Timing과 분리

Definition은 Presentation을 soft reference로 가집니다.

```cpp
TSoftObjectPtr<UDungeonPresentationDataAsset> Presentation;
```

Stage Action의 SpawnRule도 soft reference입니다.

콘텐츠를 참조한다고 프로젝트 시작 시 전부 강제로 memory에 올리지 않고, Dungeon 진입 시 Asset Subsystem이 필요한 dependency를 준비합니다.

---

## Part 7. Runtime Data와 Presentation Data를 분리한다

Dungeon gameplay definition과 화면 표현은 별도 asset입니다.

### Gameplay
`UDungeonDefinitionDataAsset`

- Stage
- Event
- Action
- Transition
- Time Limit
- Reward
- Grade

### Presentation
`UDungeonPresentationDataAsset`

- Title
- Objective Text
- Icon
- Minimap
- Room Rect
- Notice
- Result Text

Runtime은 “Combat Stage”라는 ID를 다루고, Presenter가 이를 “경비실을 정리하세요” 같은 화면 정보로 변환합니다.

---

## Part 8. StringTable — Player-facing Text

현재 주요 player-facing text는:

- `ST_Dungeon`
- `ST_Notice`
- `ST_Island`

StringTable로 이동했습니다.

코드와 DataAsset이 최종 문자열을 직접 소유하는 대신 localization key를 참조합니다.

등급 기호나 단순 format처럼 번역 대상이 아닌 text는 `INVTEXT`와 구분합니다.

---

## 선택 기준 요약

```text
대량 Row인가?
 └─ Yes → DataTable

하나의 독립 콘텐츠 단위인가?
 └─ Yes → PrimaryDataAsset

계층형 Runtime ID인가?
 └─ Yes → GameplayTag

지금 즉시 Load할 필요가 없는 Asset인가?
 └─ Yes → Soft Reference

Player가 읽는 문구인가?
 └─ Yes → StringTable
```

---

## 연관 문서

- 이 데이터가 실제 Dungeon Runtime에서 어떻게 해석되는지 → [[4. Branching Dungeon Runtime|4_Branching_Dungeon_Runtime]]
- Definition 작성 실수를 어떻게 잡는지 → [[6. Content Authoring & Validation|6_Content_Authoring_Validation]]
- Skill Data가 실제 실행과 어떻게 연결되는지 → [[10. Combat, Skill & Animation|10_Combat_Skill_Animation]]

---

## 관련 코드

- [SonheimGameType.h](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/ResourceManager/SonheimGameType.h)
- [DungeonDefinitionDataAsset.h](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/GameObject/Dungeon/DungeonDefinitionDataAsset.h)
- [DungeonStageDefinition.h](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/GameObject/Dungeon/DungeonStageDefinition.h)
