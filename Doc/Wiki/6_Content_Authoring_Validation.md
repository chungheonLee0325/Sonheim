# 6. Case Study — Content Authoring & Validation

> **문제:** 데이터 기반 구조가 커질수록 잘못된 데이터 조합도 늘어납니다. 콘텐츠 오류를 실제 플레이에서 발견하기 전에 어떻게 Editor 단계에서 찾을 것인가?

---

## Dungeon Definition Authoring

Dungeon Definition은 Stage / Event / Action / Condition / Transition을 DataAsset에서 편집합니다.

Editor metadata를 사용해 **선택한 타입과 관계없는 property는 숨기고**, 작성자가 필요한 값에 집중하도록 구성했습니다.

- `EditCondition`
- `EditConditionHides`
- `TitleProperty`
- GameplayTag category 제한
- ClampMin / ClampMax

예를 들어 `SpawnGroup` Action일 때만 GroupId / PointSet / SpawnRule 입력을 노출합니다.

---

## Validation Rule을 한 곳에 둔다

`UDungeonDefinitionDataAsset::ValidateDefinition`을 구조 검사 규칙의 단일 구현으로 두고:

1. Unreal Data Validation
2. CallInEditor `ValidateNow`

가 같은 검사 결과를 사용합니다.

자동 검사와 수동 Editor 검사의 규칙이 서로 달라지는 것을 막기 위한 구조입니다.

---

## 검사 대상

대표적으로 다음 오류를 runtime 이전에 검사합니다.

- StartStage 존재 여부
- StageId 중복
- Transition target 유효성
- Action별 필수 데이터
- Reward item/count
- TimeLimit과 timeout flow
- GameplayTag namespace
- Soft asset dependency

Runtime은 별도의 방어 코드를 유지하지만, **정적으로 찾을 수 있는 오류는 authoring 단계에서 최대한 먼저 실패**하도록 합니다.

---

## Stage Graph

`BuildStageGraph`는 실제 Definition에서 Mermaid graph를 생성합니다.

목적은 별도의 flowchart 문서를 사람이 수동으로 유지하는 것이 아니라:

```text
Actual Definition
      ↓
Graph Generation
      ↓
Review / Documentation
```

으로 **실행 데이터와 문서 사이의 불일치 가능성을 줄이는 것**입니다.

---

## Boss Pattern Validation

`UBossPatternDataAsset`도 자체 `Validate()`를 제공합니다.

Pattern authoring은 다음 값들이 서로 맞아야 합니다.

- Montage / Section
- Telegraph timing
- Strike timing
- Phase
- Area shape
- Projectile
- Leap / Charge timing

복잡한 timing data일수록 asset save/검사 시점의 validation 가치가 커집니다.

---

## 관련 코드

- [DungeonDefinitionDataAsset](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/GameObject/Dungeon/DungeonDefinitionDataAsset.h)
- [DungeonStageDefinition](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/GameObject/Dungeon/DungeonStageDefinition.h)
- [BossPatternDataAsset](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Monster/Boss/BossPatternDataAsset.h)

관련 문서:
- [[Data & Content Architecture|3_Data_Content_Architecture]]
- [[Development Workflow & Verification|13_Development_Workflow_Verification]]
