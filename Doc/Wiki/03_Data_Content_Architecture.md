# 03. Data & Content Architecture

Sonheim은 **데이터의 구성과 사용 방식에 맞춰 DataTable과 DataAsset을 선택**합니다. 아이템·스킬처럼 같은 구조가 반복되는 데이터는 DataTable에, 던전·보스처럼 여러 설정이 하나의 콘텐츠를 이루는 경우에는 DataAsset에 정의합니다. GameplayTag·Soft Reference·StringTable은 각각 **식별, 에셋 참조, 문구 관리**에 사용합니다.

## 핵심 구성

| 구분 | 사용 기술 | Sonheim에서 맡는 역할 |
|---|---|---|
| **데이터 구조** | `USTRUCT` / `FTableRowBase` | `FSkillData`, `FQuestData` 등 테이블 행의 필드·타입 정의 |
| **반복되는 정의 데이터** | `DataTable` | 아이템·스킬·몬스터·레벨·퀘스트·도감 |
| **콘텐츠 단위 설정** | `DataAsset` / `PrimaryDataAsset` | 던전 스테이지·전환 규칙, 보스 패턴 |
| **식별자** | 정수 ID / `GameplayTag` / `FPrimaryAssetId` | 아이템 조회, 스테이지 식별, 에셋 조회 |
| **에셋 참조 및 로딩** | `TSoftObjectPtr` / `AssetManager` | 던전 정의, UI 설정·Spawn Rule·몬스터 클래스 |
| **화면 문구** | `FText` / `StringTable` | 던전·알림·도감·퀘스트 텍스트 |

위 기술들은 하나의 분류에서 서로 대체하는 선택지가 아닙니다. 예를 들어 **던전 입장 정보는 DataTable에 두면서, 그 행에서 참조하는 던전 정의는 PrimaryDataAsset으로 관리**할 수 있습니다.

## 목차

- [정의 데이터와 실행 상태](#정의-데이터와-실행-상태)
- [DataTable — 반복되는 데이터 정의](#datatable--반복되는-데이터-정의)
- [DataAsset — 콘텐츠 단위 정의](#dataasset--콘텐츠-단위-정의)
- [Catalog — 입장 정보와 콘텐츠 조회](#catalog--입장-정보와-콘텐츠-조회)
- [GameplayTag와 저장용 ID](#gameplaytag와-저장용-id)
- [Soft Reference와 에셋 로딩](#soft-reference와-에셋-로딩)
- [게임플레이 데이터와 UI 표시 데이터](#게임플레이-데이터와-ui-표시-데이터)
- [StringTable — 화면 문구 관리](#stringtable--화면-문구-관리)
- [선택 기준과 비용](#선택-기준과-비용)
- [관련 코드 및 문서](#관련-코드-및-문서)

---

## 정의 데이터와 실행 상태

**정의 데이터(Definition Data)**는 스킬의 소모 비용이나 던전의 스테이지 전환 규칙처럼 콘텐츠를 실행할 때 참고하는 설정입니다. **런타임 상태(Runtime State)**는 스킬의 현재 재사용 대기 상태, 던전의 현재 단계처럼 실행 중에 변경되는 값입니다.

| 시스템 | 정의 데이터 | 실행 중 상태 |
|---|---|---|
| 스킬 | `FSkillData` — 비용·애니메이션·공격 설정 | `FSonheimSkillSpecItem` — 현재 스킬 상태 |
| 던전 | `UDungeonDefinitionDataAsset` — 스테이지·이벤트·전환 | `FDungeonStageRuntimeState` — 현재 단계·진행 결과 |
| 도감 | `FMonsterDexData` — 등록 대상·보상 조건 | `FMonsterDexEntry` — 처치·포획·수령 기록 |
| 퀘스트 | `FQuestData` — 단계·목표·보상 | `FQuestRecord` — 현재 단계·목표 진행도 |

정의와 상태를 분리하면 **같은 콘텐츠 설정을 여러 플레이어·실행에서 재사용**하면서, 플레이어별 기록이나 현재 실행 상태만 변경할 수 있습니다. 구조체의 필드·타입을 규정하는 것은 *데이터 스키마*이고, DataTable의 각 행은 그 구조에 맞춘 실제 정의 데이터입니다.

---

## DataTable — 반복되는 데이터 정의

`FSkillData`는 `FTableRowBase`를 상속해 스킬의 DataTable 행 구조를 정의합니다. 스킬마다 같은 필드 구성을 사용하면서 `SkillID`, 비용, 몽타주, 공격 설정 등의 값은 다르게 지정합니다.

~~~cpp
// SonheimGameType.h — 주요 필드 발췌
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

- `SkillClass`: 해당 스킬의 실행 클래스를 지정
- `StaminaCosts / ItemCosts`: 스킬 실행 단계별 소모 비용
- `Montage / AttackData`: 애니메이션과 타격 설정
- `NextSkillID`: 연계할 다음 스킬의 ID

여기서 **`FSkillData`는 행 구조를 정의하고, DataTable에는 스킬별 실제 설정값이 들어갑니다.** DataTable이 가진 행 이름(`RowName`)과 `SkillID`처럼 구조체 내부에 정의한 콘텐츠 ID도 개념적으로 구분합니다.

![스킬 DataTable 편집 화면 — 스킬 정의 데이터](https://github.com/user-attachments/assets/c41639c0-db7e-4a79-9b3c-1735bd9df9e3)

*DataTable에서 스킬별 정의 데이터를 행 단위로 관리하는 에디터 화면입니다.*

같은 방식으로 `FItemData`, `FAreaObjectData`, `FLevelData`를 관리하며, 최근 콘텐츠의 `FQuestData`와 `FMonsterDexData`도 DataTable 행 구조를 사용합니다.

스킬의 실제 실행 상태와 판정은 [05. Combat, Skill & Animation](05_Combat_Skill_Animation.md)에서 다룹니다.

---

## DataAsset — 콘텐츠 단위 정의

던전은 단일 설정 행보다 **스테이지 목록과 스테이지 간 전환, 결과 등급, 다른 에셋 참조가 한 묶음**으로 관리되는 콘텐츠입니다.

~~~cpp
// DungeonDefinitionDataAsset.h — 주요 필드 발췌
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

`Stages` 안에는 스테이지별 이벤트·조건·액션·전환 규칙이 포함됩니다. 정의 에셋은 `ValidateDefinition`으로 이러한 관계를 검사하며, `BuildStageGraph()`로 실제 설정에서 스테이지 그래프를 생성합니다.

던전 정의는 `UPrimaryDataAsset`으로 만들어 `FPrimaryAssetId`로 조회하고, 보스의 패턴·타이밍·타격 정보는 `UBossPatternDataAsset : UDataAsset`에 정의했습니다.

![던전 정의 DataAsset 편집 화면 — 콘텐츠 단위 설정](https://github.com/user-attachments/assets/e34540c3-2627-4d42-8912-4f0776413304)

*던전 정의를 하나의 에셋으로 관리하는 에디터 화면입니다. 스킬의 DataTable 행과 달리 던전 단위 설정을 함께 관리합니다.*

데이터 검증과 그래프 생성은 [13. Content Authoring & Validation](13_Content_Authoring_Validation.md)에서 별도로 설명합니다.

---

## Catalog — 입장 정보와 콘텐츠 조회

던전 입구에서는 전체 스테이지 정의를 직접 보유하지 않고 `FDungeonCatalogRow`에서 **입장 조건과 정의 에셋의 ID**를 읽습니다.

~~~cpp
// DungeonDefinitionDataAsset.h
struct FDungeonCatalogRow : public FTableRowBase
{
    FGameplayTag DungeonId;
    int32 DungeonNumber = 0;
    FPrimaryAssetId DefinitionAssetId;
    int32 RequiredLevel = 1;
};
~~~

~~~mermaid
flowchart TB
    ENTRY["던전 입구<br/>ADungeonTestArea"] --> CATALOG["Catalog DataTable<br/>FDungeonCatalogRow"]
    CATALOG --> CHECK["입장 가능 여부 검사<br/>RequiredLevel 등"]
    CHECK --> ID["DefinitionAssetId"]
    ID --> LOAD["UDungeonAssetSubsystem<br/>LoadDefinition"]
    LOAD --> DEF["UDungeonDefinitionDataAsset<br/>Stages · EventRules · GradeRules"]
    DEF --> RUN["UDungeonStageRuntimeSubsystem<br/>현재 던전 실행"]
~~~

서버의 `UDungeonStageRuntimeSubsystem`은 입구의 Catalog 행을 조회하고 레벨 등 입장 조건을 검사합니다. 입장에 사용할 `DefinitionAssetId`를 확보한 뒤 `UDungeonAssetSubsystem::LoadDefinition`으로 실제 던전 정의를 로드합니다.

Catalog는 입장 조건과 정의 에셋 ID를 보관하고, DataAsset은 던전의 실행 규칙을 보관합니다.

---

## GameplayTag와 저장용 ID

GameplayTag는 던전 안에서 스테이지·그룹·분기를 구분하는 식별자입니다.

~~~text
Dungeon.ForgottenRuins
 ├─ Stage.GuardRoom
 ├─ Group.Guards
 ├─ Branch.Shortcut
 └─ Barrier.GuardNorth

Boss.Grizzbolt.Pattern.Discharge
~~~

같은 던전 아래의 식별자를 계층적으로 관리할 수 있고, 에디터에서는 `Categories="Dungeon"`과 같은 메타데이터로 태그 선택 범위를 좁힙니다. `ValidateDefinition`에서는 사용한 태그가 해당 던전의 루트에 속하는지 검사하며, `NextStageId`는 실제 `Stages` 목록에도 존재해야 합니다.

다만 **모든 ID를 GameplayTag 하나로 통일하지는 않았습니다.**

| 식별자 | 역할 | 적용 예 |
|---|---|---|
| `SkillID` / `ItemID` | 반복 데이터 조회 및 게임플레이 참조 | 스킬·아이템 |
| `DungeonId` (`GameplayTag`) | 던전 및 하위 콘텐츠 식별 | 스테이지·그룹·분기 |
| `DefinitionAssetId` (`FPrimaryAssetId`) | AssetManager를 통한 정의 에셋 조회 | 던전 로딩 |
| `DungeonNumber` (`int32`) | 저장된 던전 기록을 조회하는 키 | 클리어·최고 기록 |

`UDungeonDefinitionDataAsset::GetPrimaryAssetId`는 `DungeonId`를 이용해 `DungeonDefinition` 타입의 Primary Asset ID를 구성합니다. 하지만 저장된 플레이어 기록은 별도의 `DungeonNumber`로 구분합니다.

태그 이름은 콘텐츠 정리 과정에서 변경될 수 있으므로 **실행 시 쓰는 식별자와 저장 기록의 식별자를 분리**했습니다. 저장용 숫자 ID도 기존 데이터와의 호환성을 유지하도록 관리해야 합니다.

---

## Soft Reference와 에셋 로딩

Soft Reference는 에셋을 가리키는 경로 기반 참조로, 참조만 보유한다고 대상 에셋을 즉시 메모리에 로드하지 않습니다. 실제 사용 전에는 로딩을 수행해야 합니다.

예를 들어 던전 정의는 UI 설정을 다음과 같이 참조합니다.

~~~cpp
TSoftObjectPtr<UDungeonPresentationDataAsset> Presentation;
~~~

스테이지의 `SpawnGroup` 액션은 `TSoftObjectPtr<UDungeonSpawnRuleDataAsset>`으로 스폰 규칙을 참조합니다. 스폰 규칙 안의 몬스터 클래스도 `TSoftClassPtr<ABaseMonster>`로 지정합니다.

`UDungeonAssetSubsystem`이 이 참조들을 비동기로 준비하는 순서는 다음과 같습니다.

~~~mermaid
flowchart TB
    A["FPrimaryAssetId"] --> B["던전 Definition 로드"]
    B --> C["Presentation / SpawnRule 로드"]
    C --> D["Definition·참조 검증"]
    D --> E["스폰할 MonsterClass 로드"]
    E --> F["요청 완료 · 던전 실행 준비"]
~~~

구현에서는 `UAssetManager::GetStreamableManager().RequestAsyncLoad`를 사용하고, 요청별 `FStreamableHandle`을 보관합니다. 에셋 ID를 찾지 못하거나 정의·참조 검사에 실패하면 로딩 결과로 오류를 전달합니다. 로딩 요청이 항상 성공한다고 가정하지 않습니다.

Soft Reference의 실제 로딩은 `RequestAsyncLoad`가 수행합니다. `FSkillData::Montage`처럼 일반 UObject 참조를 사용한 필드도 있습니다.

---

## 게임플레이 데이터와 UI 표시 데이터

던전에서는 **진행 규칙(`UDungeonDefinitionDataAsset`)**과 **화면 설정(`UDungeonPresentationDataAsset`)**을 별도 에셋으로 관리합니다.

| 게임플레이 정의 | UI 표시용 정의 |
|---|---|
| 현재 단계로 진입할 조건 | 스테이지 이름·목표 설명 |
| 이벤트·조건·액션·전환 | 목표 아이콘·마커 |
| 제한 시간·보상·결과 등급 | 미니맵 텍스처·방 영역 |
| `UDungeonDefinitionDataAsset` | `UDungeonPresentationDataAsset` |

서버의 진행 결과는 `FDungeonStageRuntimeState`에 담겨 복제됩니다. 클라이언트의 `UDungeonStagePresenter`는 **복제된 현재 상태와 화면 설정을 조합**해 `FDungeonStageViewData`를 구성합니다.

~~~mermaid
flowchart TB
    DEF["UDungeonDefinitionDataAsset<br/>실행 규칙"] --> SERVER["서버 던전 진행"]
    SERVER --> SNAP["FDungeonStageRuntimeState<br/>현재 진행 상태 복제"]
    SNAP --> PRESENTER["UDungeonStagePresenter"]
    PRESENT["UDungeonPresentationDataAsset<br/>문구·아이콘·맵"] --> PRESENTER
    PRESENTER --> VIEW["FDungeonStageViewData"]
    VIEW --> UI["던전 HUD · 미니맵 · 결과 화면"]
~~~

이 분리 덕분에 스테이지 전환 규칙을 바꾸지 않고 목표 문구나 아이콘을 수정할 수 있습니다. 위젯도 GameplayTag와 진행 규칙 전체를 직접 해석하지 않고 화면에 필요한 값을 받아 표시합니다.

같은 원칙을 사용하는 다른 사례도 있습니다. **도감**은 `FMonsterDexData`와 `FMonsterDexEntry`를 조합해 `FMonsterDexPageView`를 만들고, **퀘스트**는 `FQuestData`와 `FQuestRecord`를 조합해 `FQuestView`를 만듭니다.

자세한 화면 처리와 데이터 갱신은 [11. Client State Presentation](11_Client_State_Presentation_Pipeline.md), [16. MonsterDex System](16_MonsterDex_System.md), [17. Quest System](17_Quest_System.md)에서 설명합니다.

---

## StringTable — 화면 문구 관리

화면에 반복해서 사용하는 문구는 `FText`와 StringTable 키로 연결합니다. 현재 C++에서 테이블 ID를 참조하는 대표 항목은 다음과 같습니다.

- `ST_Dungeon` — 던전 HUD·결과·상호작용 문구
- `ST_Notice` — 공통 알림 문구
- `ST_Dex` — 도감 화면·알림 문구
- `ST_Quest` — 퀘스트 화면·알림 문구

~~~cpp
// StringTableIds.h / 실제 사용 형태
#define SONHEIM_ST_DUNGEON "/Game/StringTables/ST_Dungeon.ST_Dungeon"

// 사용 예
LOCTABLE(SONHEIM_ST_DUNGEON, "Boss.CaptureHint")
~~~

코드에는 테이블 경로와 키를 두고, 실제 문구는 StringTable 에셋에서 관리합니다. 반면 번역할 필요가 없는 고정 기호·형식은 `INVTEXT`로 정의하는 경우가 있습니다.

StringTable은 문구의 키와 번역 가능한 텍스트를 관리하고, DataTable·DataAsset에는 콘텐츠 정의를 저장합니다.

---

## 선택 기준과 비용

| 대상 | 사용 기술 | 선택 이유 | 관리할 부분 |
|---|---|---|---|
| 아이템·스킬·퀘스트 등 반복 정의 | `DataTable` | 같은 필드 구조의 데이터를 행 단위로 조회 | 큰 중첩 그래프는 행으로 표현하기 불편 |
| 던전·보스 콘텐츠 설정 | `DataAsset` | 단계·규칙·타격 구성을 하나의 에셋에 저장 | 에셋 간 참조와 검증 |
| ID로 조회할 던전 정의 | `UPrimaryDataAsset` | `FPrimaryAssetId`로 로딩 대상 식별 | Catalog ID·실제 에셋 대응 |
| Stage·Group·Branch 식별자 | `GameplayTag` | 계층형 이름으로 콘텐츠 참조 | 태그 변경·참조 일치 |
| 런타임에 로딩하는 에셋 | Soft Reference + `AssetManager` | 참조와 메모리 로드 시점 분리 | 로딩 완료·실패·이전 요청 해제 |
| UI에서 공유하는 문구 | `FText` / StringTable | 코드와 문구 분리 | 테이블 경로·키 |
| 던전 진행·화면 표시 | Definition + Presentation DataAsset | 진행 규칙과 문구·아이콘을 별도 수정 | 같은 Stage ID로 데이터 연결 |

데이터 형식의 선택과 런타임 상태의 소유권은 구분했습니다. 예를 들어 던전 Definition은 에셋이지만 현재 진행 상태는 서버 Runtime과 GameState에서 관리합니다.

## 관련 코드 및 문서

- [SonheimGameType.h](../../Sonheim/Source/Sonheim/ResourceManager/SonheimGameType.h) — `FSkillData`·`FItemData` 등의 행 구조
- [DungeonDefinitionDataAsset.h](../../Sonheim/Source/Sonheim/GameObject/Dungeon/DungeonDefinitionDataAsset.h) — Catalog·던전 정의
- [DungeonStageDefinition.h](../../Sonheim/Source/Sonheim/GameObject/Dungeon/DungeonStageDefinition.h) — 이벤트·조건·액션·전환 데이터
- [BossPatternDataAsset.h](../../Sonheim/Source/Sonheim/AreaObject/Monster/Boss/BossPatternDataAsset.h) — 보스 패턴 정의
- [DungeonAssetSubsystem.cpp](../../Sonheim/Source/Sonheim/GameManager/Dungeon/DungeonAssetSubsystem.cpp) — 비동기 에셋 로딩
- [DungeonPresentationDataAsset.h](../../Sonheim/Source/Sonheim/GameObject/Dungeon/DungeonPresentationDataAsset.h) — 화면 표시용 정의
- [StringTableIds.h](../../Sonheim/Source/Sonheim/Utilities/StringTableIds.h) — 텍스트 테이블 ID
- [02. Gameplay Architecture](02_Gameplay_Architecture.md) — 데이터와 실행 상태의 역할
- [09. Branching Dungeon Runtime](09_Branching_Dungeon_Runtime.md) — 데이터 기반 던전 진행
- [13. Content Authoring & Validation](13_Content_Authoring_Validation.md) — 정의 데이터 검증
