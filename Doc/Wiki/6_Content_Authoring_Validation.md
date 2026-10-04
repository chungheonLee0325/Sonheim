# 6. Content Authoring & Validation

> **핵심 구현 범위**
>
> “Data-driven 구조에서 잘못된 조합도 쉽게 만들 수 있는데, 실행하기 전에 Editor에서 어떻게 오류를 발견할 것인가?”

Data-driven은 “코드를 덜 쓴다”가 끝이 아닙니다.  
작성 가능한 데이터 조합이 늘어난 만큼 **authoring constraint와 validation**이 필요합니다.

---

## Part 1. 작성 화면에서 잘못된 입력 자체를 줄인다

예를 들어 Dungeon Action은 Type에 따라 필요한 필드가 다릅니다.

실제 선언 일부:

```cpp
USTRUCT(BlueprintType)
struct FDungeonStageAction
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    EDungeonStageAction Type = EDungeonStageAction::SpawnGroup;

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        meta=(EditCondition="Type == EDungeonStageAction::SpawnGroup",
              EditConditionHides,
              Categories="Dungeon"))
    FGameplayTag GroupId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        meta=(EditCondition="Type == EDungeonStageAction::SpawnGroup",
              EditConditionHides))
    TSoftObjectPtr<UDungeonSpawnRuleDataAsset> SpawnRule;

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        meta=(EditCondition="Type == EDungeonStageAction::GrantReward",
              EditConditionHides,
              ClampMin="1"))
    int32 RewardItemId = 0;
};
```

`SpawnGroup`을 선택했는데 Reward field까지 모두 보이게 두지 않습니다.

---

## Part 2. Editor Metadata의 역할

사용한 주요 metadata:

- `EditCondition`
- `EditConditionHides`
- `TitleProperty`
- `Categories="Dungeon"`
- `ClampMin / ClampMax`

목적은 예쁘게 보이게 하는 것이 아니라:

1. 현재 Type에 필요한 입력만 노출
2. GameplayTag namespace를 제한
3. 배열을 펼치지 않아도 어떤 Rule인지 식별
4. 명백한 범위 오류를 UI 단계에서 방지

입니다.

---

## Part 3. Validation Rule은 하나만 유지한다

`UDungeonDefinitionDataAsset`의 핵심 API:

```cpp
bool ValidateDefinition(
    TArray<FString>& Errors,
    TArray<FString>& Warnings) const;

#if WITH_EDITOR
virtual EDataValidationResult
IsDataValid(FDataValidationContext& Context) const override;

UFUNCTION(BlueprintCallable, CallInEditor)
void ValidateNow();
#endif
```

`IsDataValid`과 수동 `ValidateNow`가 서로 다른 검사 코드를 갖지 않고 같은 `ValidateDefinition`을 사용합니다.

---

## Part 4. 무엇을 검사하는가

대표적인 구조 오류:

- StartStage가 실제 존재하는가
- StageId가 중복되는가
- Transition target이 존재하는가
- SpawnGroup Action에 필요한 값이 있는가
- Reward ItemId / Count가 유효한가
- TimeLimit이 있는데 timeout 처리 경로가 없는가
- Tag가 해당 Dungeon namespace 안에 있는가
- 필요한 soft asset reference가 비어 있지 않은가

정적으로 알 수 있는 오류는 플레이 테스트까지 보내지 않습니다.

---

## Part 5. Runtime도 다시 방어한다

Editor Validation이 있다고 Runtime validation을 없애지는 않습니다.

Asset load 시에도 Definition을 검사합니다.

```text
Editor Authoring
 → ValidateDefinition

Runtime Asset Load
 → ValidateDefinition
 → dependency load
 → Run start
```

Editor tool은 편의를 위한 것이고 Runtime은 신뢰 경계입니다.

---

## Part 6. Stage Graph는 Definition에서 생성한다

`BuildStageGraph()`는 실제 Definition을 읽어 Mermaid graph를 만듭니다.

```cpp
UFUNCTION(BlueprintCallable, Category="Dungeon Tools")
FString BuildStageGraph() const;
```

그리고 Editor button으로 clipboard에 복사할 수 있습니다.

```text
Definition
   ↓
BuildStageGraph
   ↓
Mermaid
   ↓
Review / Wiki / PR
```

별도 flowchart를 사람이 수동으로 유지하면서 실제 데이터와 어긋나는 문제를 줄입니다.

---

## Part 7. Boss Pattern도 Validation 대상이다

Boss Pattern에는:

- Montage
- Section Cue
- Telegraph Mark time
- Strike time
- Phase
- Projectile
- Leap
- Charge

같은 서로 의존하는 timing data가 많습니다.

예를 들어 Strike보다 warning이 너무 짧으면 player-readable telegraph가 되지 않습니다.

`UBossPatternDataAsset`의 `Validate()`가 이런 조합을 검사합니다.

---

## Part 8. Authoring Tool의 기준

Editor tool을 추가하는 기준은 “자동화할 수 있는가?”가 아니라 아래와 같습니다.

### 사람이 반복해서 같은 실수를 하는가
→ validation

### 서로 관계없는 field가 너무 많이 보이는가
→ conditional property

### data flow를 눈으로 검토하기 어려운가
→ graph generation

### 작성 결과를 실제 runtime과 별도로 관리하고 있는가
→ source-of-truth에서 생성

---

## 연관 문서

- 실제 Definition 구조 → [[4. Branching Dungeon Runtime|4_Branching_Dungeon_Runtime]]
- DataAsset을 선택한 이유 → [[3. Data & Content Architecture|3_Data_Content_Architecture]]
- Editor agent workflow → [[13. Development Workflow & Verification|13_Development_Workflow_Verification]]

---

## 관련 코드

- [DungeonStageDefinition.h](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/GameObject/Dungeon/DungeonStageDefinition.h)
- [DungeonDefinitionDataAsset.h](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/GameObject/Dungeon/DungeonDefinitionDataAsset.h)
- [DungeonDefinitionDataAsset.cpp](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/GameObject/Dungeon/DungeonDefinitionDataAsset.cpp)
- [BossPatternDataAsset.h](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Monster/Boss/BossPatternDataAsset.h)
