# 15. Development History & Retrospective

Sonheim은 **전투·포획·인벤토리 등 개별 시스템을 구축한 뒤, 이를 분기 던전과 보스 콘텐츠에서 재사용하는 과정**을 거쳤습니다. 기능을 추가하는 것뿐 아니라 **상태의 소유권, 네트워크 동기화, 콘텐츠 규칙과 화면 표현의 경계**를 명확히 하는 방향으로 구조를 수정했습니다.

이 문서는 현재 시스템을 다시 소개하기보다 **변경 전 해결해야 했던 문제, 실제로 적용한 수정, 결과를 확인할 수 있는 코드와 커밋**을 중심으로 정리합니다.

## 주요 변경 이력

| 시점 | 변경 | 확인할 수 있는 결과 |
|---|---|---|
| **2025.06** | 포획·보유 목록·Partner Skill을 컴포넌트로 분리 | PlayerState의 Pal 목록과 Pawn의 실행 기능 구분 |
| **2025.09** | Skill Component·Grant와 서버 캐스팅 상태 정리 | SkillSpecs FastArray + UBaseSkill 서브오브젝트 복제 |
| **2025.09** | 인벤토리·공유 보관함 FastArray 도입 | 슬롯 Dirty 처리, 보관함 Viewer 상태에 따른 복제 활성화 |
| **2026.09** | Forgotten Ruins 서버 Stage Runtime 구축 | Event/Action/Transition·RunId·GameState Snapshot |
| **2026.09** | 장벽 규칙을 Definition으로 이전, Dungeon ID를 GameplayTag로 정리 | Actor는 자신의 ID와 복제된 현재 상태만 확인 |
| **2026.09–10** | 보스 패턴·데이터 검증·기록/미니맵 확장 | PatternClock, Data Validation, Runtime 결과 표시 |
| **2026.10** | 던전 전용 알림을 공용 Notice로 통합 | 현재 Feed·Headline·Title과 Channel 기반 표시 |

**주요 커밋:** [Pal 기능 분리](https://github.com/chungheonLee0325/Sonheim/commit/b8d914b5ecc99863c6c28fc0172ded1877cd11b7) · [서버 스킬/Grant 정리](https://github.com/chungheonLee0325/Sonheim/commit/83e06b821a12336c1481f77547896def45afdb1c) · [FastArray 도입](https://github.com/chungheonLee0325/Sonheim/commit/9b6882dfa2f6fa6c9e8efee2f9cdbc29808c66b1) · [던전 Runtime 추가](https://github.com/chungheonLee0325/Sonheim/commit/3d9f96af7e41fbfc484885bebc9bedfe6dbe8157) · [장벽 책임 분리](https://github.com/chungheonLee0325/Sonheim/commit/d46fcd64df1f68db7b1ae3521c9623c153b41a64) · [공용 Notice 추출](https://github.com/chungheonLee0325/Sonheim/commit/a2697024150d6981c206454867f369f09f01a9cf)

## 목차

- [Pal — 보유 데이터와 실행 기능 분리](#pal--보유-데이터와-실행-기능-분리)
- [Skill — 실행 객체와 서버 복제 상태 정리](#skill--실행-객체와-서버-복제-상태-정리)
- [Inventory — 슬롯 복제와 공유 상태 개선](#inventory--슬롯-복제와-공유-상태-개선)
- [Dungeon — 콘텐츠 규칙을 World Actor에서 분리](#dungeon--콘텐츠-규칙을-world-actor에서-분리)
- [UI — 게임플레이 상태와 화면 표현 분리](#ui--게임플레이-상태와-화면-표현-분리)
- [Validation — 데이터 관계를 실행 전에 검사](#validation--데이터-관계를-실행-전에-검사)
- [Editor 작업과 실행 검증](#editor-작업과-실행-검증)
- [개발 과정에서 정리한 설계 기준](#개발-과정에서-정리한-설계-기준)
- [관련 코드·문서](#관련-코드문서)

---

## Pal — 보유 데이터와 실행 기능 분리

### 변경 계기

포획·Pal 목록·슬롯 선택·소환·Partner Skill은 모두 Pal 기능이지만 **필요한 데이터와 수명은 같지 않습니다.** 포획이나 소환은 현재 Pawn에서 실행하고, 보유 목록은 PlayerState에서 관리해야 합니다.

[2025.06 — Pal 포획·관리·스킬 컴포넌트 분리](https://github.com/chungheonLee0325/Sonheim/commit/b8d914b5ecc99863c6c28fc0172ded1877cd11b7)

### 변경 후 구조

~~~text
ASonheimPlayerState
  └─ UPalInventoryComponent
       ├─ OwnedPals
       └─ CurrentPalIndex

ASonheimPlayer
  ├─ UPalCaptureComponent
  │    └─ PalSphere · 포획률 · Reveal · 서버 포획 판정
  └─ UPalPartnerSkillComponent
       └─ SummonedPal · 소환/회수 · Partner Skill
~~~

`ASonheimPlayer::InitializeByPlayerState`가 Pawn 컴포넌트에 PlayerState 참조를 전달해 두 수명 범위를 연결합니다.

**결과:** Pal 목록·선택 UI는 PlayerState의 보유 상태를 사용하고, 실제 투척·소환·Partner 행동은 Pawn의 컴포넌트에서 실행합니다. 추가로 포획 성공 시 **같은 `ABaseMonster` Actor에 `PartnerOwner`를 설정**해 기존 몬스터 FSM을 Partner 행동에 재사용합니다.

이후 포획 조준 UI가 갱신되지 않거나 무기 크로스헤어가 남는 문제도 컴포넌트를 직접 연결하기보다 **Pawn이 상태 변경을 중재하는 방식**으로 정리했습니다.

[2025.08 — 포획 조준 UI의 컴포넌트 간 의존성 정리](https://github.com/chungheonLee0325/Sonheim/commit/ac6a8a8a65ee5f7603b598b6c19e7f139edb8247)

상세 구현: [[04. Player & Character Systems|04_Player_Character_Systems]] · [[08. Pal Capture & Partner Lifecycle|08_Pal_Capture_Partner_Lifecycle]]

---

## Skill — 실행 객체와 서버 복제 상태 정리

### 변경 계기

스킬을 사용하려면 **실행 Phase·쿨타임·타겟 정보**가 필요하지만, 장비 UI에는 **어떤 스킬을 사용할 수 있고 시전 중인지**를 빠르게 조회할 상태가 필요합니다. 장비로 부여되는 스킬은 여러 출처가 같은 SkillId를 사용할 수도 있습니다.

2025년 9월 스킬 기능을 `USonheimSkillComponent`로 이관하며 **사용 요청, Grant 권한, 네트워크 요약 상태, 실행 객체**를 구분했습니다.

[Skill 서브오브젝트·FastArray 복제](https://github.com/chungheonLee0325/Sonheim/commit/7086ac830d19b63afb8a046e224af2afd10635f5) · [서버 캐스팅·Grant 정리](https://github.com/chungheonLee0325/Sonheim/commit/83e06b821a12336c1481f77547896def45afdb1c)

### 현재 연결

~~~mermaid
flowchart TD
    DATA["FSkillData<br/>정의·비용·Montage"] --> SKILL["UBaseSkill<br/>Phase·Cast·Fire·Cooldown"]
    INPUT["TryCastSkillById"] --> COMPONENT["USonheimSkillComponent<br/>서버 검증·Grant 권한"]
    COMPONENT --> SKILL
    COMPONENT --> SPEC["SkillSpecs FastArray<br/>시전 여부·CooldownEndTime"]
    SKILL --> SUB["ReplicateSubobjects<br/>실행 UObject의 복제 프로퍼티"]
    INVENTORY["선택 무기 변경"] --> GRANT["ReplaceGrant<br/>GrantId / RefCount"]
    GRANT --> COMPONENT
~~~

- **GrantId + RefCount:** 무기를 교체해도 다른 경로에서 부여한 동일 SkillId는 유지합니다.
- **SkillSpecs FastArray:** SkillId·Level·시전 여부·쿨타임 종료 시각을 복제합니다.
- **UBaseSkill 서브오브젝트 복제:** Phase·쿨타임·타겟 등 실행 객체 상태를 별도로 동기화합니다.
- **EnsureSkillInstance:** 기본 보유 스킬은 `BeginPlay`에서 준비하고, Grant된 스킬도 생성·캐시하며 필요한 경우 재사용합니다.

**결과:** 장비 선택·시전 UI용 상태와 실제 서버 스킬 실행을 분리하면서도, 애니메이션 Notify에서 실행되는 비용·타격은 기존 서버 권한 경로를 유지합니다.

**유지할 경계:** Grant 해제는 **사용 권한 변경**이며 캐시된 UObject의 즉시 파괴를 의미하지 않습니다. SkillSpecs와 실행 객체에도 일부 상태가 중복돼 각 경로가 맡는 범위를 명확히 해야 합니다.

상세 구현: [[05. Combat, Skill & Animation|05_Combat_Skill_Animation]] · [[12. Multiplayer Synchronization|12_Multiplayer_Synchronization]]

---

## Inventory — 슬롯 복제와 공유 상태 개선

### 변경 계기

인벤토리와 보관함에는 아이템 ID·수량뿐 아니라 **슬롯 순서**가 있습니다. 중간 삽입·삭제 후 인덱스가 달라질 수 있어, 변경된 슬롯을 복제 데이터에 일관되게 반영할 공통 처리가 필요했습니다.

[2025.09 — Inventory/Container FastArray와 Viewer 구독 도입](https://github.com/chungheonLee0325/Sonheim/commit/9b6882dfa2f6fa6c9e8efee2f9cdbc29808c66b1)

### 변경 후 처리

~~~text
서버 InventoryItems
  → FRepInventoryList (FastArray)
  → MarkItemDirty / MarkArrayDirty
  → OwnerOnly 복제
  → OnRep_RepItems
  → 슬롯 인덱스 기준 클라이언트 배열 재구성
  → OnInventoryChanged

공유 ContainerItems
  → FRepContainerList (FastArray)
  → Subscribers의 열람 상태 확인
  → RepContainerItems 복제 활성화·비활성화
~~~

`FastArraySlotUtils`로 슬롯 수정·삽입·삭제 시 인덱스 보정과 Dirty 표시를 공통화했습니다.

**결과:** 슬롯별 변경을 FastArray에 반영하는 규칙이 일관해졌고, 보관함 UI를 열 때 Viewer를 등록하고 닫을 때 해제하는 흐름을 연결했습니다.

개인 인벤토리의 `RepItems`는 OwnerOnly입니다. 공유 보관함은 **구독자가 하나라도 있을 때 전체 프로퍼티 복제를 활성화**하는 방식이며 연결별 수신자 필터링은 아닙니다. 이는 현재 코드에서 구분해 관리할 네트워크 범위입니다.

장비 전환에서는 같은 인벤토리 이벤트를 `UStatBonusComponent`의 활성 무기 보너스와 `USonheimSkillComponent::ReplaceGrant`에도 전달합니다. 아이템의 소유 상태가 전투 기능으로 연결되는 경로입니다.

상세 구현: [[07. Inventory & Crafting|07_Inventory_Crafting]]

---

## Dungeon — 콘텐츠 규칙을 World Actor에서 분리

### 던전의 진입점과 실행 규칙 분리

2026년 9월 Forgotten Ruins를 추가하면서 기존 **상호작용·몬스터 처치/포획·인벤토리**를 새로운 Stage Runtime에 연결했습니다.

[분기 던전 Runtime 첫 통합](https://github.com/chungheonLee0325/Sonheim/commit/3d9f96af7e41fbfc484885bebc9bedfe6dbe8157)

~~~text
ADungeonTestArea / ADungeonShortcutSwitch
  → IInteractableInterface
  → UDungeonStageRuntimeSubsystem
  → Event + SourceId
  → Definition의 Action·Condition·Transition
  → 현재 Stage 변경·SpawnGroup·GrantReward
~~~

레버는 **지름길 분기를 직접 결정하지 않고 `ActorInteracted` 이벤트만 전달**합니다. Runtime은 `SetRunTag`로 사용 여부를 보관하고, Wave 완료 이벤트가 발생했을 때 `HasRunTag`와 `Always` Transition을 순서대로 평가합니다.

이 구조를 통해 **같은 레버 입력 코드**를 유지한 채 분기 조건·보상·웨이브 순서를 Definition에서 구성할 수 있게 했습니다.

### 장벽 Actor가 Stage 조건을 몰라도 되도록 변경

초기 장벽은 Runtime의 Stage·Run 상태를 참조해 충돌과 이동 경로를 제어했습니다. Stage가 늘어나면 각 장벽이 던전의 분기 규칙을 알아야 할 수 있어, **닫아야 할 장벽 목록을 Definition으로 이동**했습니다.

[2026.09 — 배리어 규칙을 던전 정의로 이동](https://github.com/chungheonLee0325/Sonheim/commit/d46fcd64df1f68db7b1ae3521c9623c153b41a64)

~~~text
UDungeonDefinitionDataAsset
  └─ Stage.SealedBarriers
       ↓
UDungeonStageRuntimeSubsystem
  └─ State.SealedBarriers
       ↓ GameState Snapshot
ADungeonBarrier
  └─ 자신의 BarrierId가 목록에 있는지 확인
     → 충돌·표시·NavModifier 적용
~~~

**결과:** Stage마다 어떤 통로를 막을지 결정하는 책임은 콘텐츠 정의에 두고, 실제 월드 표현은 `ADungeonBarrier`에 유지했습니다.

같은 시기에 Stage·Group·Branch·PointSet·Zone·Barrier ID를 **`Dungeon.ForgottenRuins` 아래 GameplayTag 계층**으로 정리하고, 저장용 기록 키는 별도 `DungeonNumber`로 분리했습니다.

[2026.09 — 던전 ID를 GameplayTag 트리로 전환](https://github.com/chungheonLee0325/Sonheim/commit/65e469da7a96569cc91d3da12095f5f8fe27ea2e)

**유지할 경계:** Definition과 월드에 배치한 Actor의 Tag·BarrierId가 일치해야 합니다. 이를 보완하기 위해 작성 단계의 Definition 검사와 실행 시작 시 월드 참조 검사를 연결했습니다.

실제 분기 플레이와 Stage Graph: [[09. Branching Dungeon Runtime|09_Branching_Dungeon_Runtime]]

---

## UI — 게임플레이 상태와 화면 표현 분리

### 서버 던전 진행을 Current State로 전달

던전 HUD는 Stage·분기·목표·파티·보스·타이머처럼 여러 데이터의 현재 값을 조합합니다. 서버의 `Event Queue`와 Definition 규칙을 UMG에 구현하는 대신 **현재 표시할 상태를 복제**합니다.

~~~mermaid
flowchart TD
    SERVER["서버 DungeonStageRuntimeSubsystem"] --> STATE["FDungeonStageRuntimeState"]
    STATE --> GS["ASonheimGameState<br/>RunId · Revision Snapshot"]
    GS --> PRESENT["UDungeonStagePresenter"]
    PRESENTATION["UDungeonPresentationDataAsset<br/>문구·아이콘·지도"] --> PRESENT
    PRESENT --> VIEW["FDungeonStageViewData"]
    VIEW --> UI["HUD · Minimap · Result"]
~~~

현재 `UDungeonStagePresenter`는 `RunId`와 `Revision`을 비교해 오래된 Snapshot을 제외하고, Widget이 다시 만들어져도 **현재 복제 상태로 표시 데이터를 재구성**합니다.

파티 HP처럼 기존 컴포넌트가 소유하는 독립 상태는 던전 Snapshot에 중복 저장하지 않고 해당 `UHealthComponent`를 구독합니다.

**결과:** 콘텐츠의 단계 전환과 Widget 생성·제거의 수명 경계를 분리했습니다. 표시 문구·아이콘·미니맵 설정도 `UDungeonPresentationDataAsset`에 둬 같은 진행 상태에 서로 다른 표시를 적용할 수 있습니다.

상세 구현: [[11. UI Architecture & Client Presentation|11_Client_State_Presentation_Pipeline]]

### 던전 전용 알림을 공용 Notice로 이동

던전 진행 알림과 방 이름을 처음에는 던전 UI 경로에서 관리했습니다. 이후 레벨업·포획·제작·지역 이름도 같은 표시·교체·취소 동작이 필요해 **`UNoticeSubsystem`으로 이동**했습니다.

[2026.10 — 토스트와 방 이름을 공용 알림으로 이전](https://github.com/chungheonLee0325/Sonheim/commit/a2697024150d6981c206454867f369f09f01a9cf)

현재는 `Feed`·`Headline`·`Title` 슬롯과 Channel 정책으로 구분합니다.

~~~text
게임플레이 알림 생성
  → UNoticeSubsystem
  → Slot (Feed / Headline / Title)
  → Queue · Replace · Channel 정책
  → UNoticeWidget
~~~

**결과:** 던전 이벤트, 도감 등록, 퀘스트·레벨업, 지역 표시가 공통 알림 관리 경로를 사용할 수 있게 됐습니다. 일회성 알림은 화면 효과이고 실제 진행 상태의 원본은 각 게임플레이 시스템에 남깁니다.

개편 화면과 전후 비교: [[18. UMG v2 Migration & Editor Verification|18_UMG_v2_Migration]]

---

## Validation — 데이터 관계를 실행 전에 검사

### Stage 하나가 아니라 Graph 전체 확인

`FDungeonStageDefinition`의 개별 Tag나 필드가 유효하더라도 **서로 연결된 전환이 잘못될 수 있습니다.**

~~~text
Stage A → Stage B → Stage A
  → Cycle 검출

WaveCompleted(Group.WaveA)
  → 선행 SpawnGroup 생산 경로 없음
  → Producer/Consumer 오류

HasRunTag 분기 앞에 Always
  → 뒤 Transition이 실행되지 않음
  → 분기 순서 오류
~~~

`UDungeonDefinitionDataAsset::ValidateDefinition`은 Stage 참조·중복·도달 가능성·순환·분기 순서·그룹 생산 경로를 검사합니다. `IsDataValid`는 이 결과와 SpawnRule 의존성을 에디터에 전달하고, `UDungeonAssetSubsystem`은 실행 전 실제 에셋 로드 성공도 확인합니다.

`BuildStageGraph()`는 같은 Definition에서 Mermaid 그래프를 만들어 **현재 데이터의 분기 경로를 문서로 추출**합니다.

### Boss Pattern의 Animation 계약 검사

`UBossPatternDataAsset::Validate`는 Montage Section 존재, `MarkSeconds ≤ StrikeSeconds ≤ Pattern.Seconds`, 텔레그래프의 최소 경고 시간, Leap·Charge·Exhaust 조건을 검사합니다.

단순 필드 유효성보다 **서로 연관된 값이 함께 실행 가능한지**를 확인하는 방향으로 검사 범위를 넓힌 사례입니다.

에디터에서 실제 오류를 발생시킨 캡처와 검사 코드는 [[13. Content Authoring & Validation|13_Content_Authoring_Validation]]에 있습니다.

---

## Editor 작업과 실행 검증

Sonheim의 UMG·DataAsset·Animation 작업에서는 [AgentMcp](https://github.com/chungheonLee0325/AgentMcp)를 이용해 **에디터 조사와 변경 후 실행 확인**을 연결했습니다.

~~~text
Inspect
  → Edit
  → Read-back
  → Blueprint Compile / Save
  → PIE
  → Runtime Read-back · Log · Viewport
~~~

현재 Tool의 결과만 믿는 대신 수정한 Widget Tree·Class Default를 재조회하고, Blueprint 컴파일 및 PIE에서 실제 표시·동작을 확인하는 흐름입니다.

플러그인 자체의 **별도 테스트베드 UI 제작·Tool 검증**과 Sonheim에서의 **실제 화면 개편 작업**은 구분해 기록했습니다.

적용 사례: [[14. Unreal Editor Automation & Verification|14_Unreal_Editor_Automation_Verification]] · [[18. UMG v2 Migration & Editor Verification|18_UMG_v2_Migration]]

---

## 개발 과정에서 정리한 설계 기준

| 이후 기능에도 적용한 기준 | 현재 코드에서 확인되는 사례 |
|---|---|
| **상태를 사용하는 곳과 소유하는 곳 구분** | PlayerState의 Pal/Inventory 기록 ↔ Pawn의 포획·소환·무기 실행 |
| **하나의 소스에서 여러 기능에 전달** | 인벤토리 무기 변경 → 스탯 보너스·Skill Grant·Mesh |
| **콘텐츠 규칙과 월드 Actor의 실행 분리** | 레버 → Event, Barrier → SealedBarriers, Boss → Pattern Data |
| **현재 상태와 일회성 연출을 구분** | GameState Snapshot ↔ Notice / Montage RPC |
| **작성 데이터의 관계를 검사** | Stage Graph·Group 생산 경로·Montage Section·Strike 시간 |
| **변경 결과를 다시 확인** | Editor Read-back → Compile → PIE → Log/Viewport |

가장 큰 변화는 **새 콘텐츠가 기존 시스템을 직접 수정하는 대신, 각 시스템이 공개하는 이벤트·상태·API를 조합하도록 설계 기준이 구체화된 것**입니다.

Forgotten Ruins는 그 결과를 확인하는 통합 사례입니다. 입구·레버는 상호작용 인터페이스, 목표는 몬스터 처치·포획 이벤트, 보상은 인벤토리 API, 보스 타격은 기존 Damage Context, 화면은 서버 Snapshot과 UI 표시 데이터를 재사용합니다.

## 관련 코드·문서

- [SonheimSkillComponent.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/AreaObject/Skill/SonheimSkillComponent.cpp) — Skill Grant·복제 상태·캐싱
- [InventoryComponent.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/AreaObject/Player/Utility/InventoryComponent.cpp) — 슬롯 FastArray·장비 연동
- [DungeonStageRuntimeSubsystem.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/GameManager/Dungeon/DungeonStageRuntimeSubsystem.cpp) — Event Queue·전환
- [DungeonBarrier.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/GameObject/Dungeon/DungeonBarrier.cpp) — Stage 상태 기반 장벽
- [DungeonDefinitionDataAsset.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/GameObject/Dungeon/DungeonDefinitionDataAsset.cpp) — 구조·그래프 검증
- [NoticeSubsystem.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/UI/Notice/NoticeSubsystem.cpp) — 공용 알림 정책
- [[02. Gameplay Architecture|02_Gameplay_Architecture]] — 현재 객체와 데이터 책임
- [[05. Combat, Skill & Animation|05_Combat_Skill_Animation]] — 서버 스킬 실행
- [[07. Inventory & Crafting|07_Inventory_Crafting]] — 슬롯 상태와 공동 제작
- [[08. Pal Capture & Partner Lifecycle|08_Pal_Capture_Partner_Lifecycle]] — Pal 수명과 보유 데이터
- [[09. Branching Dungeon Runtime|09_Branching_Dungeon_Runtime]] — 분기 동작 영상·Definition
- [[13. Content Authoring & Validation|13_Content_Authoring_Validation]] — 실제 에디터 오류 검증
- [[14. Unreal Editor Automation & Verification|14_Unreal_Editor_Automation_Verification]] — Editor Tool 워크플로
- [[18. UMG v2 Migration & Editor Verification|18_UMG_v2_Migration]] — 화면 전후 비교와 실행 확인
