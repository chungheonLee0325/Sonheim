# 15. Development History — 주요 구조 변경

Sonheim의 전투·포획·인벤토리 기능을 던전 콘텐츠까지 연결하면서 수정한 구조를 정리했습니다. 각 변경의 구현 근거는 당시 커밋과 현재 코드에서 확인할 수 있습니다.

## 변경 이력

| 시기 | 변경 내용 | 코드·커밋 |
|---|---|---|
| 2025.06 | Pal 보유 기록과 포획·소환 실행 분리 | [Pal 컴포넌트 분리](https://github.com/chungheonLee0325/Sonheim/commit/b8d914b5ecc99863c6c28fc0172ded1877cd11b7) |
| 2025.09 | 스킬 사용 권한(Grant)·실행 객체·복제 상태 구분 | [서버 스킬/Grant 정리](https://github.com/chungheonLee0325/Sonheim/commit/83e06b821a12336c1481f77547896def45afdb1) |
| 2025.09 | 인벤토리·보관함 슬롯의 FastArray 도입 | [슬롯 복제 변경](https://github.com/chungheonLee0325/Sonheim/commit/9b6882dfa2f6fa6c9e8efee2f9cdbc29808c66b1) |
| 2026.09 | 이벤트 기반 던전 Runtime과 Stage Definition | [던전 Runtime 통합](https://github.com/chungheonLee0325/Sonheim/commit/3d9f96af7e41fbfc484885bebc9bedfe6dbe8157) |
| 2026.09 | 장벽 통제 조건을 Actor에서 Stage Definition으로 이동 | [SealedBarriers 변경](https://github.com/chungheonLee0325/Sonheim/commit/d46fcd64df1f68db7b1ae3521c9623c153b41a64) |
| 2026.10 | 던전 알림을 다른 콘텐츠와 같은 Notice로 통합 | [공용 Notice](https://github.com/chungheonLee0325/Sonheim/commit/a2697024150d6981c206454867f369f09f01a9cf) |

## Pal — 보유 기록과 실행 대상의 수명 분리

포획한 Pal 목록은 현재 조작 중인 Pawn보다 오래 유지해야 합니다. `ASonheimPlayerState`에 `UPalInventoryComponent`를 두고 `OwnedPals`·`CurrentPalIndex`를 관리하며, Pawn의 `UPalCaptureComponent`와 `UPalPartnerSkillComponent`는 포획 시도·소환·회수·스킬 사용을 실행합니다.

~~~text
PlayerState
  UPalInventoryComponent → 보유 목록 · 선택 슬롯
             ↓
Pawn의 Capture/PartnerSkill Component
             ↓
포획 판정 · 소환 · Partner 행동
~~~

`ASonheimPlayer::InitializeByPlayerState`에서 두 수명의 객체를 연결합니다. 포획에 성공한 몬스터는 새 Actor로 교체하지 않고 기존 `ABaseMonster`에 `PartnerOwner`를 설정해 행동을 전환합니다. 포획 조준 UI도 각 컴포넌트를 서로 직접 연결하기보다 Pawn을 통해 상태가 전달되도록 변경했습니다.

[포획 조준 UI 의존성 변경 커밋](https://github.com/chungheonLee0325/Sonheim/commit/ac6a8a8a65ee5f7603b598b6c19e7f139edb8247) · [08. Pal Capture & Partner Lifecycle](08_Pal_Capture_Partner_Lifecycle.md)

## Skill — 부여 권한과 실행 상태 분리

장비 교체 시 여러 출처가 같은 `SkillId`를 부여할 수 있습니다. `USonheimSkillComponent::ReplaceGrant`는 `GrantId`·`RefCount`로 권한을 갱신하며, 부여 해제가 실행 UObject의 즉시 삭제를 뜻하지 않도록 관리합니다.

`EnsureSkillInstance`는 실제 실행에 필요한 `UBaseSkill`을 생성·재사용합니다. 서버에서 사용 가능 여부와 시전 동작을 결정하고, UI가 참고할 `SkillSpecs`는 FastArray로 전달합니다. 실행 객체가 가진 Phase·타겟 등의 프로퍼티는 별도 서브오브젝트 복제 경로를 사용합니다.

~~~mermaid
flowchart LR
    I["무기·장비 변경"] --> G["ReplaceGrant<br/>GrantId / RefCount"]
    G --> C["SonheimSkillComponent<br/>사용 가능 여부 · TryCast"]
    C --> E["UBaseSkill<br/>실제 스킬 실행"]
    C --> V["SkillSpecs<br/>시전·쿨타임 요약"]
    E --> O["복제 서브오브젝트<br/>Phase·Target"]
~~~

스킬 정의·장비 선택 UI·애니메이션 Notify·타격 실행이 한 객체에 모이지 않도록 역할을 나눈 변경입니다. 실제 시전 조건과 효과 판정은 서버 경로를 유지합니다.

[스킬 서브오브젝트 변경](https://github.com/chungheonLee0325/Sonheim/commit/7086ac830d19b63afb8a046e224af2afd10635f5) · [05. Combat, Skill & Animation](05_Combat_Skill_Animation.md)

## Inventory — 슬롯 인덱스와 복제 변경 표시

아이템 슬롯은 순서가 의미 있는 데이터입니다. 중간 삽입·삭제 시 후속 슬롯의 인덱스까지 바뀌기 때문에 단일 항목의 수량 변경과 배열 구조 변경을 구분했습니다.

`FastArraySlotUtils`로 슬롯 추가·이동·삭제 시 인덱스 보정과 Dirty 표시를 공통화합니다. 서버의 `InventoryItems`는 복제용 `FRepInventoryList`를 통해 OwnerOnly로 전달하고, 클라이언트에서는 슬롯 인덱스 순서로 로컬 배열을 다시 구성한 뒤 `OnInventoryChanged`를 호출합니다.

공유 `Container`도 FastArray를 사용하지만, `Subscribers`가 하나 이상일 때 `RepContainerItems` 복제를 활성화합니다. 이는 **구독자별 독립 필터링이 아닌 프로퍼티 단위 활성화**입니다.

인벤토리에서 무기를 선택하면 장비 표시만 변경하는 것이 아니라 활성 무기의 스탯 보너스와 `ReplaceGrant`도 함께 갱신합니다. 따라서 슬롯 변경은 게임플레이 상태를 수정하는 API를 거치고, Widget은 변경된 결과를 다시 표시합니다.

[07. Inventory & Crafting](07_Inventory_Crafting.md) · [12. Multiplayer Synchronization](12_Multiplayer_Synchronization.md)

## Dungeon — 진행 규칙과 월드 Actor 분리

Forgotten Ruins를 구현할 때 월드의 레버·입구·장벽이 Stage 분기 규칙을 직접 판단하지 않도록 변경했습니다.

레버는 기존 `IInteractableInterface`로 `ActorInteracted + SourceId`를 전달합니다. `UDungeonStageRuntimeSubsystem`은 해당 `EventRule`의 `Action`을 실행하고, Wave 완료 시점에 `RunTag` 조건을 평가해 지름길/추가 웨이브를 결정합니다.

장벽도 자신의 Stage를 직접 판단하지 않습니다. Stage의 `SealedBarriers`를 `GameState` Snapshot에 포함하고, `ADungeonBarrier`는 자신의 `BarrierId`가 현재 닫힌 목록에 있는지만 확인해 충돌·표시를 전환합니다.

~~~text
DungeonDefinition (Event / Action / Transition · SealedBarriers)
                 ↓
     DungeonStageRuntimeSubsystem
                 ↓ 현재 Stage / 닫힌 Barrier ID
           GameState Snapshot
                 ↓
     ADungeonBarrier → 실제 통로 표시·충돌
~~~

분기와 차단 대상은 콘텐츠 데이터에서 정하고, 월드 Actor는 상호작용 입력이나 상태 표현을 맡도록 조정한 사례입니다. Stage·Group·Branch·Barrier 식별자는 Dungeon 하위 GameplayTag로 정리했고, 별도의 `DungeonNumber`는 저장 기록 키로 사용합니다.

[09. Branching Dungeon Runtime](09_Branching_Dungeon_Runtime.md) · [13. Content Authoring & Validation](13_Content_Authoring_Validation.md)

## UI — 복합 게임 상태를 화면용 데이터로 변환

던전 UI는 한 화면에서 스테이지·목표·파티 HP·타이머·보스 행동·결과를 함께 표시합니다. `UDungeonStagePresenter`가 `GameState` Snapshot, `UDungeonPresentationDataAsset`, 개별 `UHealthComponent`를 읽어 `FDungeonStageViewData`로 변환합니다. `UDungeonUIRouterSubsystem`은 HUD와 결과 위젯을 교체하거나 다시 열 때 현재 ViewData를 적용합니다.

던전 전용 알림은 이후 `UNoticeSubsystem`으로 이동했습니다. 퀘스트·도감·레벨업도 같은 `FNoticeData`와 슬롯 정책을 사용하며, 던전 종료 시 `Dungeon` 채널만 정리합니다. 알림은 **한 번 표시하고 사라지는 피드백**이며 실제 진행 상태를 보관하지 않습니다.

[11. Client State Presentation](11_Client_State_Presentation_Pipeline.md) · [18. UMG v2](18_UMG_v2_Migration.md)

## 콘텐츠 정의 검사 추가

Stage마다 설정값이 올바르더라도 전환을 연결하면 실행할 수 없는 경로가 생길 수 있습니다. `UDungeonDefinitionDataAsset::ValidateDefinition`은 누락 Stage·순환·도달 불가능·`Always` 조건에 가려진 분기·SpawnGroup 생산 경로를 검사합니다.

보스 `UBossPatternDataAsset::Validate`는 경고 시작과 타격 사이 시간, Montage Section 및 Leap·Charge 설정 간 관계를 검사합니다. 이 결과를 Unreal Data Validation으로 표시하고, Stage Graph는 Definition에서 Mermaid로 추출합니다.

[13. Content Authoring & Validation](13_Content_Authoring_Validation.md)

## 관련 코드

- [PalInventoryComponent.cpp](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Player/Utility/PalInventoryComponent.cpp) · [PalCaptureComponent.cpp](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Player/Utility/PalCaptureComponent.cpp)
- [SonheimSkillComponent.cpp](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Skill/SonheimSkillComponent.cpp)
- [InventoryComponent.cpp](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Player/Utility/InventoryComponent.cpp)
- [DungeonStageRuntimeSubsystem.cpp](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/GameManager/Dungeon/DungeonStageRuntimeSubsystem.cpp) · [DungeonBarrier.cpp](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/GameObject/Dungeon/DungeonBarrier.cpp)
- [DungeonStagePresenter.cpp](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/UI/Dungeon/DungeonStagePresenter.cpp) · [NoticeSubsystem.cpp](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/UI/Notice/NoticeSubsystem.cpp)
- [DungeonDefinitionDataAsset.cpp](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/GameObject/Dungeon/DungeonDefinitionDataAsset.cpp)
