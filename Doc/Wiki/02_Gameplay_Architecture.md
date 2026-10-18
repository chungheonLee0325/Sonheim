# 02. Gameplay Architecture

Sonheim은 **플레이어가 보유하는 데이터, 월드에서 실행되는 게임플레이, 클라이언트 UI의 역할을 분리**합니다. 상태가 유지되어야 하는 범위에 따라 `PlayerState`·`Pawn`·`Subsystem`·`ActorComponent`를 선택하고, 화면에는 각 시스템의 변경 사항 또는 화면용으로 조합한 데이터를 전달합니다.

## 핵심 구조

| 구분 | 담당 | 실제 구현 |
|---|---|---|
| 플레이어별 기록·능력치 | `PlayerState`의 컴포넌트 | Inventory, StatBonus, PalInventory, MonsterDex, Quest |
| 월드의 캐릭터 상태와 행동 | `Pawn / AAreaObject`의 컴포넌트 | 이동·애니메이션·HP·Stamina·Condition·Skill |
| 서로 다른 월드 오브젝트의 공통 동작 | `IInteractableInterface` | 아이템·보관함·제작대·던전 레버·입구 |
| 현재 던전 실행 | `UDungeonStageRuntimeSubsystem` (`UWorldSubsystem`) | 스테이지·이벤트·전환·실패·보상 |
| 여러 클라이언트가 볼 던전 상태 | `ASonheimGameState` | `FDungeonStageRuntimeState` 복제 |
| 에셋 로딩·저장된 진행 기록 | `UGameInstanceSubsystem` | DungeonAsset·DungeonProgress·MonsterDex·Quest |
| 로컬 플레이어 화면 | `ULocalPlayerSubsystem` | GameUIManager·GameUIToast·DungeonUIRouter·Notice |

**데이터의 보관 위치, 변경 권한, 네트워크 전송 범위는 서로 다른 결정**입니다. 예를 들어 Inventory 기록은 PlayerState의 컴포넌트에 있지만 실제 변경은 서버가 처리하고, 슬롯 상태는 소유 클라이언트에 복제합니다.

## 목차

- [전체 처리 구조](#전체-처리-구조)
- [객체 수명에 따른 역할 분리](#객체-수명에-따른-역할-분리)
- [컴포넌트와 인터페이스](#컴포넌트와-인터페이스)
- [정의 데이터·실행 상태·UI 표시 데이터](#정의-데이터실행-상태ui-표시-데이터)
- [상태 변경과 화면 갱신](#상태-변경과-화면-갱신)
- [던전에서 기존 시스템 재사용](#던전에서-기존-시스템-재사용)
- [선택 기준과 비용](#선택-기준과-비용)
- [관련 코드 및 문서](#관련-코드-및-문서)

---

## 전체 처리 구조

~~~mermaid
flowchart TB
    DEF["정의 데이터<br/>DataTable · DataAsset"]

    subgraph GAMEPLAY["서버 게임플레이"]
        PLAYER["PlayerState / ActorComponent<br/>Inventory · Stat · Pal · Quest · Dex"]
        PAWN["Pawn / AAreaObject<br/>Health · Skill · Condition · Movement"]
        DUNGEON["UDungeonStageRuntimeSubsystem<br/>스테이지·이벤트·전환"]
    end

    REP["캐릭터·플레이어 상태 복제<br/>Property / FastArray"]
    GAMESTATE["ASonheimGameState<br/>FDungeonStageRuntimeState"]
    PRESENT["UDungeonStagePresenter<br/>스냅샷 + UI 표시용 정의"]
    EVENT["상태 변경 Delegate / RepNotify"]
    VIEW["FDungeonStageViewData"]
    UI["UMG<br/>HUD · Inventory · Dungeon · Result"]
    NOTICE["UNoticeSubsystem<br/>Feed · Headline · Title"]

    DEF --> PLAYER
    DEF --> PAWN
    DEF --> DUNGEON
    PLAYER --> REP
    PAWN --> REP
    DUNGEON --> GAMESTATE
    REP --> EVENT --> UI
    GAMESTATE --> PRESENT --> VIEW --> UI
    DEF -. "던전 화면 설정 참조" .-> PRESENT
    GAMEPLAY -. "일회성 알림 전달" .-> NOTICE
    NOTICE --> UI
~~~

게임플레이 실행과 현재 상태 관리는 Actor·Component·Subsystem 안에서 이루어집니다. 복제가 필요한 상태만 클라이언트로 전달하며, **UI가 모든 시스템의 실행 규칙을 직접 해석하지 않도록** 표시 경로를 나눕니다.

위 그림은 주요 책임의 연결을 설명한 개념도입니다. 각 상태의 복제 조건과 화면별 갱신 방식은 아래에서 구분합니다.

---

## 객체 수명에 따른 역할 분리

### PlayerState와 Pawn

`ASonheimPlayerState`는 플레이어의 보유 아이템, 스탯 보정, 보유 Pal, 도감·퀘스트 기록을 컴포넌트로 관리합니다.

~~~cpp
// SonheimPlayerState.cpp — 생성자 발췌
m_InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("Inventory"));
m_StatBonusComponent = CreateDefaultSubobject<UStatBonusComponent>(TEXT("StatBonus"));
m_PalInventoryComponent = CreateDefaultSubobject<UPalInventoryComponent>(TEXT("PalInventory"));
m_MonsterDexComponent = CreateDefaultSubobject<UMonsterDexComponent>(TEXT("MonsterDex"));
m_QuestComponent = CreateDefaultSubobject<UQuestComponent>(TEXT("Quest"));
~~~

반면 `ASonheimPlayer`는 현재 조작하는 캐릭터의 이동·메시·애니메이션·전투 상태를 다룹니다. 공통 캐릭터 기반인 `AAreaObject`에는 `UHealthComponent`, `UStaminaComponent`, `UConditionComponent`, `USonheimSkillComponent` 등이 연결됩니다.

장비 변경을 예로 들면, **장비 및 스탯 보정 정보는 PlayerState에 두고, 계산된 공격력·최대 HP·이동 속도 등은 실제 Pawn에 적용**합니다.

~~~text
PlayerState / Inventory
   ↓ 장비 교체
StatBonusComponent · Skill Grant
   ↓
ASonheimPlayer
   ├─ 공격력·방어력 적용
   ├─ Health / CharacterMovement 반영
   └─ 무기 Mesh·HUD·사용 스킬 갱신
~~~

Inventory를 Pawn의 생성·파괴에 함께 묶지 않기 위한 구조입니다. 다만 **PlayerState에 둔다고 디스크에 자동 저장되거나 모든 레벨 이동에서 상태가 보존되는 것은 아닙니다.** 저장 여부는 별도 SaveGame 처리, Pawn과의 연결은 초기화·재연결 코드로 관리합니다.

### WorldSubsystem·GameState·GameInstanceSubsystem

던전은 서로 다른 세 책임을 나눕니다.

- **`UDungeonStageRuntimeSubsystem : UWorldSubsystem`** — 현재 월드에서 서버가 스테이지 진입, 이벤트 처리, 분기와 종료를 관리
- **`ASonheimGameState`** — `FDungeonStageRuntimeState`를 복제해 참가 클라이언트가 같은 진행 상태를 확인하도록 전달
- **`UDungeonAssetSubsystem / UDungeonProgressSubsystem : UGameInstanceSubsystem`** — 던전 에셋 로딩과 완료 기록의 SaveGame 관리

`UDungeonStageRuntimeSubsystem::Publish`로 반영된 상태는 `ASonheimGameState::PublishDungeonStageState`를 통해 복제됩니다. 서버의 실행 객체를 클라이언트에 그대로 복제하는 것이 아니라, **화면과 다른 월드 오브젝트에 필요한 현재 진행 정보를 스냅샷으로 전달**합니다.

도감·퀘스트도 `UMonsterDexSubsystem`과 `UQuestSubsystem`이 정의 데이터를 조회하고 호스트 SaveGame을 관리하지만, **실제 플레이어별 진행 기록은 PlayerState의 Component가 관리**합니다.

### PlayerController와 LocalPlayerSubsystem

`PlayerController`는 로컬 입력과 UI 초기화, 서버·클라이언트 요청을 연결합니다. 화면 종류마다 입력 모드·커서 상태를 따로 관리하지 않도록 로컬 UI 관리 기능을 Subsystem으로 분리했습니다.

| 로컬 시스템 | 관리 내용 |
|---|---|
| `UGameUIManagerSubsystem` | 메뉴·인벤토리·도감·퀘스트 화면의 열림/닫힘, 레이어별 맨 위 화면의 입력 모드·커서 |
| `UDungeonUIRouterSubsystem` | 던전 HUD·결과 화면 생성·교체, ViewData 보관, 기존 HUD 숨김·복원 |
| `UNoticeSubsystem` | `Feed`·`Headline`·`Title` 슬롯별 알림 표시 순서·채널 |

세 클래스는 `ULocalPlayerSubsystem`으로, **각 로컬 플레이어의 화면**을 담당합니다. 전투 상태 자체나 던전 진행 규칙은 이 안에 두지 않습니다.

---

## 컴포넌트와 인터페이스

### ActorComponent — 상태와 기능을 조합

`AAreaObject`는 플레이어와 몬스터가 공유하는 캐릭터 기반 클래스입니다. Health·Stamina·Condition·Skill·Level과 이동·회전 관련 기능은 컴포넌트로 분리합니다.

~~~text
AAreaObject : ACharacter
 ├─ UHealthComponent
 ├─ UStaminaComponent
 ├─ UConditionComponent
 ├─ ULevelComponent
 ├─ USonheimSkillComponent
 ├─ UMoveUtilComponent
 └─ URotateUtilComponent
~~~

외부 코드는 `AAreaObject::DecreaseHP`, `AddCondition`, `CastSkill` 같은 공통 API를 호출할 수 있습니다. 내부 상태 변경과 스킬 실행은 해당 컴포넌트와 스킬 로직에 위임합니다.

컴포넌트를 분리한 기준은 **독립적으로 관리해야 할 상태가 있고, 플레이어·몬스터 등 여러 캐릭터가 같은 기능을 사용하는가**입니다. 모든 로직을 컴포넌트로 나누지는 않았으며, 연동이 필요한 부분은 상위 Actor나 PlayerState가 연결합니다.

### Interface — 서로 다른 오브젝트의 공통 상호작용

아이템, 보관함, 제작대, 던전 레버·입구는 클래스가 다르지만 `IInteractableInterface`의 공통 입력 흐름을 사용합니다.

~~~mermaid
flowchart TB
    INPUT["UInteractionComponent<br/>대상 탐색·입력·Hold"] --> IFACE["IInteractableInterface"]
    IFACE --> ITEM["아이템 획득"]
    IFACE --> CONTAINER["보관함 열기"]
    IFACE --> CRAFT["제작대 사용"]
    IFACE --> LEVER["던전 레버 작동"]
    IFACE --> ENTRANCE["던전 입구 사용"]
~~~

`UInteractionComponent`는 상호작용 가능한 대상을 찾고 입력·Hold 진행을 처리합니다. 각 대상은 표시할 안내, Hold 시간·취소 규칙, 실제 실행 동작을 인터페이스로 제공합니다.

새 오브젝트를 추가해도 플레이어 입력 코드에 오브젝트 종류별 분기를 늘리는 대신, 대상이 인터페이스를 구현하도록 했습니다. 자세한 판정·서버 실행은 [[06. World Interaction Systems|06_World_Interaction_Systems]]에서 설명합니다.

---

## 정의 데이터·실행 상태·UI 표시 데이터

이 세 가지는 역할이 다릅니다.

| 구분 | 의미 | 실제 예 |
|---|---|---|
| **정의 데이터** | 스킬·던전 등 콘텐츠의 설정과 규칙 | `FSkillData`, `UDungeonDefinitionDataAsset` |
| **실행 상태** | 현재 게임에서 변하는 값 | `FSonheimSkillSpecItem`, `FDungeonStageRuntimeState` |
| **UI 표시 데이터** | 화면에 필요한 문구·수치·표시 상태로 변환한 값 | `FQuestView`, `FMonsterDexPageView`, `FDungeonStageViewData` |

### 스킬의 정의·상태·실행

~~~text
FSkillData (DataTable)
 └─ 소모 비용·공격 범위·몽타주
          ↓
USonheimSkillComponent
 ├─ FSonheimSkillSpecItem : SkillId·bIsCasting·CooldownEndTime
 └─ UBaseSkill : 실제 스킬 실행 로직
~~~

스킬의 설정과 현재 사용·재사용 대기 상태, 실행 객체를 한 구조체에 섞지 않았습니다. `FSonheimSkillSpecItem`은 FastArray로 상태를 동기화하고, `UBaseSkill`은 스킬 실행을 담당합니다.

### 던전의 정의·진행·화면 표시

~~~mermaid
flowchart TB
    DEF["UDungeonDefinitionDataAsset<br/>스테이지·이벤트·전환 정의"] --> RUN["UDungeonStageRuntimeSubsystem<br/>서버 실행"]
    RUN --> SNAP["ASonheimGameState<br/>FDungeonStageRuntimeState"]
    SNAP --> PRES["UDungeonStagePresenter"]
    DATA["UDungeonPresentationDataAsset<br/>목표 문구·아이콘·미니맵 설정"] --> PRES
    PRES --> VIEW["FDungeonStageViewData"]
    VIEW --> ROUTER["UDungeonUIRouterSubsystem"]
    ROUTER --> WIDGET["UMG · HUD / Minimap / Result"]
~~~

던전 UI는 스냅샷의 `StageId`, 목표 진행도, 분기·보스 상태 등을 읽어 화면을 구성합니다. `UDungeonStagePresenter`는 `UDungeonPresentationDataAsset`의 문구·아이콘·지도 정보와 결합해 `FDungeonStageViewData`로 변환하고, Router가 화면 위젯을 관리합니다.

위젯이 **스테이지 전환 조건이나 GameplayTag 해석 규칙을 직접 구현하지 않도록** 분리한 구조입니다.

도감과 퀘스트도 같은 원칙으로 정의 데이터와 플레이어 기록을 조합합니다.

- **도감** — `FMonsterDexData + FMonsterDexEntry → FMonsterDexPageView`
- **퀘스트** — `FQuestData + FQuestRecord → FQuestView`

표시 데이터 생성은 화면 특성에 맞춰 `MonsterDexView::Build`, `QuestView::Build`와 던전 Presenter 등으로 구현했습니다. **모든 UI에 동일한 Presenter 클래스나 별도의 MVVM 체계를 강제하지 않았습니다.**

---

## 상태 변경과 화면 갱신

UI 갱신은 변경되는 데이터의 범위에 따라 세 가지 경로를 사용합니다.

### 1. 독립 컴포넌트 상태

~~~text
HealthComponent / InventoryComponent
    ↓ Replication·RepNotify 또는 상태 변경
OnHealthChanged / OnInventoryChanged
    ↓
HUD / Inventory Widget 갱신
~~~

HP는 `UHealthComponent::OnHealthChanged`, 인벤토리는 `UInventoryComponent::OnInventoryChanged` 등을 사용합니다. 해당 컴포넌트가 상태를 관리하고, 이를 사용하는 UI가 변경 알림을 구독합니다.

### 2. 복합 콘텐츠 상태

~~~text
Dungeon Runtime
    ↓
GameState Snapshot
    ↓
UDungeonStagePresenter
    ↓
FDungeonStageViewData
    ↓
Dungeon UI Router → HUD / Result
~~~

던전은 단계·파티·보스·목표 상태를 함께 표시하므로 복제된 스냅샷과 별도 상태를 조합합니다. 예를 들어 **던전 진행은 GameState 스냅샷**, **파티 HP는 각 HealthComponent**에서 가져오므로 동일한 HP를 던전 스냅샷에 중복 저장하지 않습니다.

현재 시간을 기준으로 변하는 던전 타이머도 서버 시간 정보를 이용해 계산하므로, HUD를 새로 만들어도 해당 스테이지의 진행 시간을 이어서 표시할 수 있습니다.

### 3. 일회성 알림

~~~text
레벨 상승 / 첫 포획 / 퀘스트 갱신 / 제작 완료
    ↓
알림 데이터 전달
    ↓
UNoticeSubsystem
    ├─ Feed : 진행·갱신 알림
    ├─ Headline : 주요 달성 알림
    └─ Title : 지역 이름
~~~

이 알림은 UI가 잠시 보여주는 이벤트입니다. **레벨·퀘스트 진행도 같은 실제 현재 상태를 대신 저장하거나 복제하는 수단은 아닙니다.** 플레이어가 나중에 UI를 열었을 때 필요한 정보는 원래 컴포넌트나 스냅샷에서 다시 읽습니다.

---

## 던전에서 기존 시스템 재사용

분기형 던전은 개별적으로 구축한 시스템이 실제 콘텐츠에서 함께 동작하는 사례입니다.

| 기존 기능 | 던전에서의 사용 |
|---|---|
| `IInteractableInterface` | 던전 입구·지름길 레버 등 월드 상호작용 |
| `ASonheimGameState`의 던전 스냅샷 | 장벽·보상 상자 등이 현재 단계·완료 상태에 맞춰 월드 표현 갱신 |
| 전투·포획 결과 | 스폰 그룹과 던전 목표 진행 |
| `UInventoryComponent` | 던전 보상 아이템 지급 |
| 기존 몬스터·포획 구조 | 수호자 보스와 포획 가능 상태 |
| `FAttackData / FCustomDamageEvent` | 보스 패턴의 실제 타격·데미지 처리 |
| `UNoticeSubsystem` | 단계 갱신·완료·지역 이름 알림 |

예를 들어 지름길 레버는 상호작용 사실과 식별자를 던전 실행 시스템에 전달하고, **실제 분기 선택은 던전 정의의 이벤트·전환 규칙**이 결정합니다. 레버 Actor에 다음 스테이지를 하드코딩하지 않습니다.

이처럼 새로운 콘텐츠에서는 기존 컴포넌트와 인터페이스를 재사용하고, 콘텐츠 고유의 진행 규칙은 `UDungeonStageRuntimeSubsystem`과 정의 데이터에 둡니다.

---

## 선택 기준과 비용

| 설계 선택 | 적용한 이유 | 필요한 관리 |
|---|---|---|
| **PlayerState / Pawn 분리** | 보유 데이터와 현재 캐릭터의 수명 분리 | Pawn 생성·교체 시 참조 재연결 |
| **ActorComponent** | HP·스킬 등 독립 기능의 공유와 조합 | 컴포넌트 초기화·의존 관계 |
| **Interface** | 다른 오브젝트에도 같은 입력·Hold 흐름 적용 | 인터페이스 규칙과 대상별 구현 |
| **WorldSubsystem + GameState** | 서버의 던전 실행과 클라이언트 상태 복제 분리 | 스냅샷 구성·복제 시점 |
| **정의 데이터 / 실행 상태 분리** | 같은 콘텐츠 정의로 여러 실행·플레이어 기록 관리 | ID 대응과 상태 구조 관리 |
| **Presenter / ViewData** | 복합 상태를 UI에서 직접 해석하지 않음 | 화면용 데이터 변환 코드 |
| **LocalPlayer UI Subsystem** | 화면별 입력·수명 정책을 한곳에서 관리 | 화면과 게임플레이 간 입력 정책 |

**모든 상태를 별도 ViewData로 만들거나, 모든 시스템을 Subsystem으로 옮기는 것이 목적은 아닙니다.** 상태 수명과 실제 사용 범위에 필요한 만큼만 역할을 분리했습니다.

## 관련 코드 및 문서

- [AreaObject.h](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/AreaObject/Base/AreaObject.h) — 공통 캐릭터 API·컴포넌트
- [SonheimPlayerState.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/AreaObject/Player/SonheimPlayerState.cpp) — 플레이어별 컴포넌트와 스탯 처리
- [InteractionComponent.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/AreaObject/Player/Utility/InteractionComponent.cpp) — 인터페이스 기반 상호작용
- [DungeonStageRuntimeSubsystem.h](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/GameManager/Dungeon/DungeonStageRuntimeSubsystem.h) — 던전 서버 실행
- [SonheimGameState.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/GameManager/SonheimGameState.cpp) — 던전 상태 복제
- [DungeonStagePresenter.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/UI/Dungeon/DungeonStagePresenter.cpp) — 던전 화면용 데이터 생성
- [DungeonUIRouterSubsystem.h](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/UI/Dungeon/DungeonUIRouterSubsystem.h) — 던전 UI 수명 관리
- [GameUIManagerSubsystem.h](../../Sonheim/Plugins/GameUIFramework/Source/GameUIFramework/Public/GameUIManagerSubsystem.h) · [NoticeSubsystem.h](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/UI/Notice/NoticeSubsystem.h) — 로컬 UI 관리
- [[03. Data & Content Architecture|03_Data_Content_Architecture]] — DataTable·DataAsset·식별자·로딩
- [[04. Player & Character Systems|04_Player_Character_Systems]] — PlayerState·Pawn·Stat·Skill
- [[06. World Interaction Systems|06_World_Interaction_Systems]] — 공통 상호작용
- [[09. Branching Dungeon Runtime|09_Branching_Dungeon_Runtime]] — 던전 이벤트와 분기
- [[11. UI Architecture & Client Presentation|11_Client_State_Presentation_Pipeline]] — 실제 UI 연결·갱신
- [[12. Multiplayer Synchronization|12_Multiplayer_Synchronization]] — RPC·FastArray·동기화 범위
