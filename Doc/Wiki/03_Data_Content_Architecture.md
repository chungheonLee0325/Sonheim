# 03. Data & Content Architecture

Sonheim의 gameplay data는 **반복되는 row data, 독립 콘텐츠 정의, runtime identifier, asset dependency, player-facing text**의 성격에 따라 DataTable·DataAsset·GameplayTag·Soft Reference·StringTable로 나눠 관리합니다.

실제 프로젝트에서는 다음처럼 사용합니다.

| 데이터 성격 | 사용 방식 | 실제 예 |
|---|---|---|
| 같은 schema의 row를 ID로 반복 조회 | **DataTable** | Item, Skill, AreaObject, Level, Resource |
| 하나의 독립 콘텐츠 정의 | **PrimaryDataAsset / DataAsset** | Dungeon Definition, Boss Pattern |
| 계층적 Runtime identifier | **GameplayTag** | Stage, Branch, Barrier, Boss Pattern |
| 필요 시점에 Load할 dependency | **Soft Reference** | Dungeon Presentation, Spawn Rule |
| Player-facing text | **StringTable** | Dungeon, Notice, Island text |

---

## 전체 데이터 흐름

~~~mermaid
flowchart LR
    ROW["<b>Row Data</b><br/>Item · Skill · Stat"]
    CATALOG["<b>Catalog</b><br/>입장/조회용 Index"]
    DEF["<b>Content Definition</b><br/>Dungeon · Boss"]
    TAG["<b>GameplayTag</b><br/>Stage · Branch · Pattern"]
    SOFT["<b>Soft Asset Dependency</b>"]
    RUNTIME["<b>Runtime</b>"]

    ROW --> RUNTIME
    CATALOG --> DEF
    DEF --> RUNTIME
    TAG --> DEF
    TAG --> RUNTIME
    SOFT --> DEF
~~~

각 데이터 형식은 프로젝트에서 맡는 책임과 lifecycle에 맞춰 사용합니다.

---

## 1. Item / Skill처럼 반복되는 Gameplay Data — DataTable

Item이나 Skill은 같은 schema를 가진 row가 많고, Runtime에서 ID로 반복 조회합니다.

Skill Data:

~~~cpp
struct FSkillData : public FTableRowBase
{
    int SkillID = 0;
    TSubclassOf<UBaseSkill> SkillClass;

    TArray<FSkillStaminaCost> StaminaCosts;
    TArray<FSkillItemCost> ItemCosts;

    float CastRange = 0.f;
    float CoolTime = 0.f;

    UAnimMontage* Montage = nullptr;
    TArray<FAttackData> AttackData;

    int NextSkillID = 0;
};
~~~

여기서 한 row는:

- 실행할 Skill class
- Cost
- Range / Cooldown
- Animation
- Attack Data
- Combo 연결

을 선택합니다.

Skill은 같은 schema의 row가 많고 ID 기반 조회가 반복되므로 DataTable에 저장합니다.

실제 실행 과정은 [[05. Combat, Skill & Animation|05_Combat_Skill_Animation]]에서 이어집니다.

---

## 2. Dungeon Definition — 독립 콘텐츠 단위

Dungeon은 Item/Skill과 요구사항이 다릅니다.

하나의 Dungeon이 함께 소유하는 정보:

- Start Stage
- 여러 Stage와 Transition
- Reward / Grade Rule
- Presentation dependency
- Spawn Rule dependency
- Validation
- PrimaryAsset identity

이 정보는 <code>UDungeonDefinitionDataAsset</code> 하나가 콘텐츠 단위로 소유합니다.

~~~cpp
class UDungeonDefinitionDataAsset : public UPrimaryDataAsset
{
public:
    FGameplayTag DungeonId;
    FGameplayTag StartStageId;

    TSoftObjectPtr<UDungeonPresentationDataAsset> Presentation;

    TArray<FDungeonStageDefinition> Stages;
    TArray<FDungeonGradeRule> GradeRules;
};
~~~

\`\`\`text
Dungeon Definition
 ├─ Identity
 ├─ Stage Graph
 ├─ Reward / Grade
 └─ Asset Dependencies
\`\`\`

Definition 하나가 **작성·검증·Load의 단위**가 됩니다.

---

## 3. Catalog는 “콘텐츠 찾기”만 담당

Dungeon 입구에서 전체 Stage Graph를 알 필요는 없습니다.

\`FDungeonCatalogRow\`는 작은 index 역할만 가집니다.

~~~cpp
struct FDungeonCatalogRow : public FTableRowBase
{
    FGameplayTag DungeonId;
    int32 DungeonNumber = 0;
    FPrimaryAssetId DefinitionAssetId;
    int32 RequiredLevel = 1;
};
~~~

~~~text
Entrance / Catalog
      ↓
DefinitionAssetId
      ↓
Dungeon Definition Load
      ↓
Runtime Start
~~~

입장 조건과 Definition 조회 정보만 Catalog에 두고 실제 콘텐츠 내용은 DataAsset으로 넘깁니다.

---

## 4. Runtime Identity — GameplayTag

Dungeon에는 서로 관계 있는 ID가 많이 필요합니다.

예:

~~~text
Dungeon.ForgottenRuins.Stage.*
Dungeon.ForgottenRuins.Group.*
Dungeon.ForgottenRuins.Branch.*
Dungeon.ForgottenRuins.Barrier.*

Boss.Grizzbolt.Pattern.*
~~~

GameplayTag는 Stage/Group/Branch/Barrier를 같은 Dungeon namespace 아래 계층적으로 구성하고, Editor filtering과 validation에도 같은 identifier를 사용하기 위해 적용했습니다.

### 콘텐츠별 namespace

Stage / Group / Barrier가 어느 Dungeon에 속하는지 이름 자체에 계층이 생깁니다.

### Editor authoring

Property metadata로 특정 category의 Tag만 선택하게 제한할 수 있습니다.

### Runtime 비교

Branch 선택, RunTag, Barrier state처럼 서로 다른 시스템이 같은 identifier를 공유할 수 있습니다.

### Validation

잘못된 Dungeon namespace의 Tag를 Definition에 넣으면 Editor Validation에서 탐지합니다.

같은 GameplayTag가 Runtime ID와 Editor authoring/validation 기준을 함께 제공합니다.

---

## 5. Runtime ID와 Save ID는 의도적으로 분리

현재 Dungeon Record는 \`DungeonNumber\`를 Save key로 사용합니다.

~~~text
DungeonId : GameplayTag
→ Runtime / Authoring identity

DungeonNumber : int32
→ Persistent Save identity
~~~

GameplayTag는 콘텐츠 정리 과정에서 rename될 수 있습니다.

반면 SaveGame key가 같이 바뀌면 기존 Player record와의 호환 문제가 생깁니다.

따라서 **읽기 좋은 콘텐츠 ID와 장기 저장용 stable ID를 같은 값에 묶지 않았습니다.**

---

## 6. Soft Reference — Dependency와 Load 시점을 분리

Dungeon Definition은 Presentation을 soft reference로 갖습니다.

~~~cpp
TSoftObjectPtr<UDungeonPresentationDataAsset> Presentation;
~~~

Stage의 Spawn Action도 Spawn Rule을 soft reference로 참조합니다.

~~~text
Dungeon Definition
   ├─ Presentation (Soft)
   └─ Spawn Rule (Soft)
          ↓
Dungeon 진입 시 Asset Subsystem 준비
~~~

Soft reference로 dependency를 유지하고, Dungeon 진입 등 실제 사용 시점에 필요한 asset을 준비합니다.

Editor Validation에서는 필요하면 soft dependency를 load해 구조까지 검사하지만 Runtime structural validation과는 분리합니다.

---

## 7. Gameplay Definition과 Presentation Data를 분리

Dungeon의 진행 규칙은 Gameplay Definition, 화면 문구·아이콘·Map 정보는 Presentation Data가 소유합니다.

### Gameplay Definition

- Stage
- Event / Condition / Action
- Transition
- Time Limit
- Reward
- Grade Rule

### Presentation Data

- Dungeon / Stage Title
- Objective Text
- Icon
- Minimap Texture / Room Rect
- Notice
- Boss Label
- Result Text

~~~text
Runtime
StageId / GroupId / BossActionId
        ↓
Presenter
        +
Presentation Data
        ↓
HUD / Minimap / Result
~~~

예를 들어 Stage Transition 규칙을 바꾸지 않고도 Objective 문구나 icon을 수정할 수 있습니다.

반대로 UI copy를 바꾸기 위해 gameplay Definition을 건드릴 필요도 없습니다.

---

## 8. Player-facing Text — StringTable

현재 주요 text는:

- \`ST_Dungeon\`
- \`ST_Notice\`
- \`ST_Island\`

StringTable로 관리합니다.

코드나 DataAsset에 최종 문자열을 반복 저장하기보다 key를 통해 text를 참조해 **같은 문구의 위치와 localization source를 분리**합니다.

등급 기호처럼 번역 대상이 아닌 고정 text는 \`INVTEXT\` 등과 구분합니다.

---

## 실제 선택 기준

~~~text
같은 구조의 Row가 많이 필요한가?
→ DataTable

자체 identity와 여러 dependency를 가진 콘텐츠인가?
→ PrimaryDataAsset / DataAsset

여러 시스템이 공유하는 계층형 Runtime ID인가?
→ GameplayTag

Asset dependency를 Load 시점과 분리해야 하는가?
→ Soft Reference

Player-facing text인가?
→ StringTable
~~~

데이터의 조회 방식·identity·dependency·lifecycle에 따라 저장 형식을 구분합니다.

---

## 설계 선택과 비용

| 선택 | 얻은 것 | 비용 / 제약 |
|---|---|---|
| **DataTable for row data** | Item/Skill 같은 대량 schema를 한 곳에서 조회 | nested 콘텐츠 graph에는 부적합 |
| **DataAsset for content definition** | Dungeon/Boss를 독립 authoring·validation 단위로 관리 | asset 수와 dependency 관리 증가 |
| **GameplayTag identity** | 계층형 ID와 Editor/Validation 연계 | namespace 규칙을 지속적으로 관리해야 함 |
| **Soft Reference** | Load timing 분리 | 비동기/실패 load path를 처리해야 함 |
| **Gameplay / Presentation 분리** | 규칙과 UI 표현을 독립 수정 | ID mapping과 Presenter layer 추가 |

---

## 연관 문서

- [[05. Combat, Skill & Animation|05_Combat_Skill_Animation]] — Skill Data가 실제 실행 Logic으로 연결되는 과정
- [[09. Branching Dungeon Runtime|09_Branching_Dungeon_Runtime]] — Dungeon Definition을 Server Runtime이 해석하는 과정
- [[11. UI Architecture & Client Presentation|11_Client_State_Presentation_Pipeline]] — Presentation Data와 Runtime State 결합
- [[13. Content Authoring & Validation|13_Content_Authoring_Validation]] — GameplayTag / Definition / dependency 검사

---

## 관련 코드

- [SonheimGameType.h](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/ResourceManager/SonheimGameType.h)
- [DungeonDefinitionDataAsset.h](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/GameObject/Dungeon/DungeonDefinitionDataAsset.h)
- [DungeonStageDefinition.h](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/GameObject/Dungeon/DungeonStageDefinition.h)
- [DungeonAssetSubsystem](https://github.com/chungheonLee0325/Sonheim/tree/main/Sonheim/Source/Sonheim/GameManager/Dungeon)
