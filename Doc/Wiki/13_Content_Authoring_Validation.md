# 13. Content Authoring & Validation

던전의 Stage·Event·Action·Transition과 보스 패턴을 DataAsset으로 작성합니다. 필드가 각각 유효해도 **연결된 Stage로 이동할 수 없거나, 앞선 조건이 뒤의 분기를 가릴 수 있어** 데이터 간 관계를 검사하는 코드를 추가했습니다.

## 에디터에서 검출한 설정 오류

| 잘못된 Stage 참조 | Data Validation 오류 |
|---|---|
| ![Stages에 존재하지 않는 NextStageId](../Media/Wiki/13_Content_Authoring_Validation/13_dungeon_invalid_definition.png) | ![Missing NextStageId 오류](../Media/Wiki/13_Content_Authoring_Validation/13_dungeon_validation_error.png) |

`NextStageId`에 GameplayTag를 선택할 수 있어도 실제 `Stages` 목록에 그 Tag가 없으면 전환할 수 없습니다. `ValidateDefinition()`은 `FindStage(NextStageId)`로 검사해 `Missing NextStageId`를 반환합니다.

![Always 조건이 뒤의 분기를 가리는 구성과 오류](../Media/Wiki/13_Content_Authoring_Validation/13_graph_validation.png)

`WaveCompleted`의 전환은 앞에서부터 조건을 평가해 처음 참인 Stage로 이동합니다. `Always`가 `HasRunTag`보다 앞에 있으면 지름길 분기는 선택될 수 없으므로 **`Always branch shadows subsequent transitions.`** 오류를 반환합니다.

![보스 Telegraph 최소 경고 시간 오류](../Media/Wiki/13_Content_Authoring_Validation/13_boss_validation_error.png)

영역 Telegraph에서 `MarkSeconds=1.0`·`StrikeSeconds=1.2`를 지정하면 경고 노출은 0.2초입니다. 영역 공격의 최소 경고 시간(0.4초)을 만족하지 않아 에디터에서 오류로 표시됩니다.

## 콘텐츠 작성 시 필드 제약

`FDungeonStageAction`은 `Type`에 따라 편집할 프로퍼티가 달라집니다. `EditConditionHides`로 현재 Action에서 사용할 필드만 표시하고, `Categories="Dungeon"`으로 GameplayTag 선택 범위를 제한합니다.

~~~cpp
// DungeonStageDefinition.h — SpawnGroup일 때만 노출
UPROPERTY(EditAnywhere, BlueprintReadOnly,
    meta=(EditCondition="Type == EDungeonStageAction::SpawnGroup",
          EditConditionHides, Categories="Dungeon"))
FGameplayTag GroupId;

UPROPERTY(EditAnywhere, BlueprintReadOnly,
    meta=(EditCondition="Type == EDungeonStageAction::SpawnGroup",
          EditConditionHides))
TSoftObjectPtr<UDungeonSpawnRuleDataAsset> SpawnRule;
~~~

![Dungeon DataAsset의 Action별 필드와 에디터 도구](../Media/Wiki/13_Content_Authoring_Validation/13_authoring_constraints.png)

`Dungeon.ForgottenRuins` 아래에서 Stage·Group·Branch·Zone·Barrier 등 식별자를 관리합니다. `ValidateDefinition`은 Tag가 현재 `DungeonId`에 속하는지 `MatchesTag`로 검사하지만, **올바른 Tag 범위 안에 있다고 참조 대상이 존재하는 것은 아닙니다.** 전환 대상 Stage·SpawnGroup의 실제 생산 경로는 별도로 검사합니다.

## ValidateDefinition을 에디터와 실행 전 로드에 사용

핵심 구조 검사는 `UDungeonDefinitionDataAsset::ValidateDefinition(Errors, Warnings)`에 모았습니다. 에디터 버튼 `ValidateNow()`, Unreal의 `IsDataValid()`, 실행 전 `UDungeonAssetSubsystem`이 이 검사를 사용합니다.

~~~mermaid
flowchart LR
    A["Dungeon Definition"] --> B["ValidateDefinition<br/>Stage·Event·Graph 검사"]
    B --> C["ValidateNow<br/>Output Log · 에디터 알림"]
    B --> D["IsDataValid<br/>추가 SpawnRule 참조 검사"]
    B --> E["DungeonAssetSubsystem<br/>실행 전 로드 검사"]
    A --> G["BuildStageGraph<br/>Mermaid 내보내기"]
~~~

`IsDataValid()`는 구조 검사 결과와 함께 SpawnGroup의 `SpawnRule` 참조를 로드해 `Count(1~16)`와 `MonsterClass` 설정도 확인합니다. `DungeonAssetSubsystem`의 게임플레이 로드는 `Presentation`·`SpawnRule`·`MonsterClass`가 실제 로드됐는지 추가 확인합니다.

**에디터 검사가 통과한 것과 실제 콘텐츠 에셋이 로드됐다는 것은 다릅니다.** 구조의 기본 규칙은 공유하고, 실행 진입점은 필요한 객체의 로드 결과까지 확인합니다.

## Stage Graph와 Transition 검증

`ValidateDefinition()`은 Stage·Transition ID와 연결 관계를 검사합니다.

| 검사 | 발견하는 설정 |
|---|---|
| Stage·Transition 참조 | 비어 있거나 중복된 ID, 존재하지 않는 `NextStageId` |
| EventRule·Action | 같은 Event/Source 중복, 실행할 수 없는 Action 값 |
| Terminal Stage | 종료 Stage에 실행되지 않을 EventRule이 남은 경우 |
| 전환 조건 순서 | `Always` 뒤의 조건부 Transition |
| DFS 순환 검사 | 자기 참조 또는 탐색 중인 Stage로 되돌아가는 전환 |
| 도달 가능성 | `StartStageId`에서 도달하지 않는 Stage(Warning) |

DFS는 방문 중인 Stage(`Visiting`)와 탐색을 마친 Stage를 구분합니다. 시작 Stage에서 도달하지 못한 Stage도 따로 순회하므로 분리된 그래프의 순환을 놓치지 않습니다. 순환은 오류, 시작점에서 도달 불가능한 Stage는 경고입니다.

### SpawnGroup을 사용하는 이벤트가 실행 가능한가?

스테이지의 Action이 그룹을 생성하는 `SpawnGroup(GroupId)`를 **Producer**, `WaveCompleted`·`BossDefeated`·`SpawnGroupCompleted`처럼 그 그룹의 진행 상태를 확인하는 조건을 **Consumer**로 검사합니다.

~~~text
Stage.Enter: SpawnGroup(Group.WaveA)  → Producer
             ↓
Stage.Combat: WaveCompleted(WaveA)   → Consumer

해당 Group의 Producer가 없음        → Error
Producer Stage에서 Consumer Stage로 경로가 없음 → Error
~~~

`ProducersByGroup`와 Stage 전환 Edge를 사용해 Producer에서 Consumer까지 도달 경로가 **존재할 수 있는지** 확인합니다. `BossDefeated`는 Boss Group 여부도 구분합니다. 모든 이벤트 순서나 조건의 실제 조합을 실행 시뮬레이션으로 증명하는 검사는 아닙니다.

### 이벤트의 Source와 Stage 타이머

`StageEntered / StageTimeout`은 Stage 자체가 발생시키므로 `SourceId`를 지정하면 오류입니다. Actor·Zone·Monster 관련 이벤트는 각각 SourceId를 요구합니다. `EmitEvent`는 `StageEntered`만 허용하고, 같은 이벤트·소스로 즉시 자신을 다시 호출하는 정의도 차단합니다.

`TimeLimitSeconds > 0`인 Stage에는 `StageTimeout` EventRule이 필요하고, 시간 제한이 없는데 Timeout Rule이 있으면 오류입니다. Stage 진입 시 서버 Timer의 만료가 실제 처리 가능한 이벤트로 연결되는지 확인하는 검사입니다.

보상 등급인 `GradeRules`는 순서대로 평가합니다. 마지막 Rule에는 `MaxClearSeconds=0`, `RequiredOptionalObjectives=0`을 요구해 앞의 등급에 해당하지 않는 클리어도 최종 등급으로 처리할 수 있게 합니다.

## Definition에서 Stage Graph 추출

~~~cpp
// DungeonDefinitionDataAsset.h
UFUNCTION(BlueprintCallable, Category="Dungeon Tools")
FString BuildStageGraph() const;
~~~

`BuildStageGraph()`는 실제 Definition의 Stage·EventRule·Action·Transition을 읽어 **Mermaid 텍스트**를 만듭니다. Stage 전환과 분기뿐 아니라 장벽·제한 시간도 주석에 반영하고, `ValidateDefinition`의 오류·경고 개수를 함께 기록합니다.

[Forgotten Ruins — 생성된 Stage Graph 원본](../Media/Wiki/09_Branching_Dungeon_Runtime/09_dungeon_stage_graph.mmd) · [09. Branching Dungeon Runtime](09_Branching_Dungeon_Runtime.md)

## BossPatternDataAsset의 타이밍·Animation 검사

`UBossPatternDataAsset::Validate()`는 패턴·Strike의 값과 Montage Section이 서로 맞는지 검사합니다. `IsDataValid()`는 결과를 에디터 Data Validation 오류로 전달합니다.

~~~cpp
// BossPatternDataAsset.cpp — 영역 Telegraph 경고 시간 검사
if (Strike.StrikeSeconds - Strike.MarkSeconds <
    UBossPatternDataAsset::MinWarningSeconds)
{
    Problems.Add(FString::Printf(
        TEXT("%s: the mark shows for less than %.1f s, too short to step out"),
        *At, UBossPatternDataAsset::MinWarningSeconds));
}
~~~

| 대상 | 검사할 관계 |
|---|---|
| 시간 | `0 ≤ MarkSeconds ≤ StrikeSeconds ≤ Pattern.Seconds` |
| 영역 Telegraph | 순수 투사체를 제외하고 최소 경고 0.4초 |
| 범위 | Radius·Ring 안쪽 반경·Cone 각도·Line 너비 |
| Montage | 각 Cue의 Section이 실제 Montage에 존재하는지 |
| 특수 행동 | Leap·Hop·Charge·Exhaust·GetUp 값과 필요한 Section |
| 패턴 | PatternId 중복·Montage·거리 범위·Strike 정의 |

![실제 Montage에 없는 Section을 참조한 Validation 오류](../Media/Wiki/13_Content_Authoring_Validation/13_boss_animation_validation.png)

`Montage->GetSectionIndex(Section)`으로 존재 여부를 확인합니다. `Shape=None`인 순수 투사체는 영역 Telegraph의 최소 경고 조건을 적용받지 않지만, 기본 시간 순서는 검사합니다.

이 문서의 검사는 **작성된 콘텐츠가 실행 조건을 갖췄는지**를 확인하는 부분입니다. 실제 패턴 선택·이동·타격은 [10. Boss Encounter Runtime](10_Boss_Encounter_Runtime.md)의 Runtime이 처리합니다.

## 관련 코드

- [DungeonStageDefinition.h](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/GameObject/Dungeon/DungeonStageDefinition.h) — Action·Event·Condition과 필드 조건
- [DungeonDefinitionDataAsset.cpp](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/GameObject/Dungeon/DungeonDefinitionDataAsset.cpp) — 구조·Graph·Editor 검사
- [DungeonAssetSubsystem.cpp](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/GameManager/Dungeon/DungeonAssetSubsystem.cpp) — 실행 전 참조 로드
- [BossPatternDataAsset.cpp](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Monster/Boss/BossPatternDataAsset.cpp) — Telegraph·Montage 검증
- [03. Data & Content](03_Data_Content_Architecture.md) · [14. Editor Automation](14_Unreal_Editor_Automation_Verification.md)
