# 3. Data & Content Architecture

Sonheim은 초기의 DataTable 중심 구조에서 현재는 **DataTable + Primary DataAsset + GameplayTag + Soft Reference**를 용도별로 선택하는 구조로 확장됐습니다.

핵심은 모든 데이터를 한 방식으로 통일하는 것이 아니라, **데이터의 수명과 사용 방식에 맞는 표현을 선택하는 것**입니다.

---

## DataTable — 대량의 Row 데이터

동일 schema의 row가 많고 ID 조회가 빈번한 데이터에 사용합니다.

대표 예:

- Item
- Skill
- AreaObject
- Level
- Resource
- Container

이 데이터는 여러 시스템에서 반복 조회되며, 한 row가 독립적인 콘텐츠 asset lifecycle을 가질 필요는 없습니다.

---

## Primary DataAsset — 콘텐츠 정의와 Dependency

Dungeon처럼 하나의 콘텐츠가 여러 stage와 asset dependency를 갖는 경우 `UPrimaryDataAsset`을 사용합니다.

```text
FDungeonCatalogRow
  ├─ DungeonNumber
  ├─ RequiredLevel
  └─ DefinitionAssetId
             ↓
        Asset Manager
             ↓
UDungeonDefinitionDataAsset
```

`FDungeonCatalogRow`는 실제 Definition asset을 강참조하지 않고 `FPrimaryAssetId`를 보관합니다.  
`UDungeonAssetSubsystem`이 필요한 시점에 Definition과 gameplay dependency를 준비합니다.

### 왜 DataTable 하나로 끝내지 않았는가

Dungeon은 하나의 row보다 다음 요구가 큽니다.

- 여러 Stage와 Rule의 중첩 구조
- Presentation / SpawnRule 등 asset dependency
- 필요 시점 load
- editor validation
- 자체 identity

이런 콘텐츠는 독립 asset으로 관리하는 편이 더 자연스럽습니다.

---

## GameplayTag — 콘텐츠 내부 Identifier

Dungeon Stage / Group / Branch / Barrier / Event Source와 Boss Pattern은 GameplayTag 계층을 ID로 사용합니다.

예:

```text
Dungeon.ForgottenRuins
├─ Stage.*
├─ Group.*
├─ Branch.*
├─ Barrier.*
└─ Source.*

Boss.Grizzbolt
└─ Pattern.*
```

문자열이나 전역 enum에 비해 콘텐츠별 namespace를 만들 수 있고, editor property에 category 제한도 적용할 수 있습니다.

---

## Soft Reference — Dependency와 Load Timing 분리

Dungeon Definition은 Presentation, SpawnRule 등 gameplay asset을 soft reference로 연결합니다.

목표는 프로젝트 시작 시 모든 asset을 강제로 로드하는 것이 아니라, **콘텐츠 진입 시 필요한 dependency를 명시적으로 준비하는 것**입니다.

이 구조는 기존 GameInstance의 DataTable cache와 충돌하지 않습니다.

- 대량 정적 row → DataTable
- 콘텐츠 단위 dependency graph → PrimaryAsset / Soft Reference

---

## Runtime Data와 Presentation Data 분리

Dungeon은 게임 규칙과 화면 표현을 같은 asset에 모두 넣지 않습니다.

- `UDungeonDefinitionDataAsset`: stage / rule / action / transition / grade 등 gameplay definition
- `UDungeonPresentationDataAsset`: 텍스트 / icon / minimap / room presentation

그 결과 동일한 runtime state를 UI가 presentation 전용 데이터와 조합해 표현할 수 있습니다.

---

## StringTable

플레이어가 읽는 Dungeon / Notice / Island 문구는 StringTable key로 이동했습니다.

- `ST_Dungeon`
- `ST_Notice`
- `ST_Island`

코드와 DataAsset은 최종 문자열을 직접 소유하기보다 StringTable entry를 참조합니다.  
기호, 등급 문자, 숫자 format 등 번역 대상이 아닌 값은 `INVTEXT`로 구분합니다.

---

## 선택 기준

| 요구 | 방식 |
|---|---|
| 동일 schema의 대량 데이터 | DataTable |
| 자체 identity/dependency를 가진 콘텐츠 | PrimaryDataAsset |
| 계층적 runtime ID | GameplayTag |
| 필요 시점 로딩 | Soft Object/Class Reference |
| 플레이어 노출 문구 | StringTable |
| 시스템 전역 policy | Config |

---

## 관련 코드

- [DungeonDefinitionDataAsset](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/GameObject/Dungeon/DungeonDefinitionDataAsset.h)
- [DungeonAssetSubsystem](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/GameManager/Dungeon/DungeonAssetSubsystem.h)
- [DungeonStageDefinition](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/GameObject/Dungeon/DungeonStageDefinition.h)
- [StringTableIds](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/Utilities/StringTableIds.h)

관련 문서: [[Content Authoring & Validation|6_Content_Authoring_Validation]]
