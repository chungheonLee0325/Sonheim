# 04. Player & Character Systems

Sonheim의 플레이어 시스템은 **PlayerState에 보유 데이터와 최종 스탯을 두고, Pawn에 실제 이동·전투·애니메이션 상태를 적용**합니다. 장비를 바꾸면 Inventory의 선택 상태가 **스탯 보너스, 사용 가능한 스킬, 무기 Mesh·HUD**로 전달되고, 플레이어의 입력 가능 범위는 `EPlayerState`와 `FActionRestrictions`로 제어합니다.

## 게임 화면 — 장비 전환

https://github.com/user-attachments/assets/2079af91-4ad9-4f93-99e1-3e21efbca57f

곡괭이와 Shotgun을 전환하면서 **장착 무기 Mesh·HUD 표시·공격 방식이 함께 바뀌는 동작**을 확인할 수 있습니다.

## 핵심 구현

| 기능 | 상태를 관리하는 곳 | 실제 적용·결과 |
|---|---|---|
| **플레이어 보유 기록** | `ASonheimPlayerState`의 Inventory·PalInventory·MonsterDex·Quest 컴포넌트 | 장비·보유 Pal·도감·퀘스트 기록 |
| **기본/최종 스탯** | `ASonheimPlayerState` + `UStatBonusComponent` | `ASonheimPlayer::StatChanged`로 Health·공격/방어·Movement에 반영 |
| **장비와 무기 선택** | `UInventoryComponent` | `UStatBonusComponent`·`USonheimSkillComponent`·무기 Mesh/HUD 갱신 |
| **행동 제한** | `ASonheimPlayer::CurrentPlayerState` + `FActionRestrictions` | 이동·회전·행동 입력 허용 여부 |
| **상태 조건** | `UConditionComponent` | `Dead / Invincible / Hidden` 비트 조합·시간 만료 |
| **월드 캐릭터** | `ASonheimPlayer : AAreaObject` | 카메라·무기·스프린트·락온·글라이더·Pal 관련 행동 |

PlayerState/Pawn을 나눈 이유와 전체 시스템 소유권은 [[02. Gameplay Architecture|02_Gameplay_Architecture]]에서 설명합니다. 이 문서는 **플레이어 기능 간 실제 데이터·이벤트 연결**을 중심으로 다룹니다.

## 목차

- [플레이어를 구성하는 객체](#플레이어를-구성하는-객체)
- [장비 변경이 스탯·스킬·외형에 반영되는 흐름](#장비-변경이-스탯스킬외형에-반영되는-흐름)
- [기본 스탯과 Source별 Modifier](#기본-스탯과-source별-modifier)
- [Action State로 입력 범위 제어](#action-state로-입력-범위-제어)
- [Condition Bitmask와 시간 관리](#condition-bitmask와-시간-관리)
- [이동·무기 상태와 복제](#이동무기-상태와-복제)
- [PlayerState 연결 시점](#playerstate-연결-시점)
- [설계 선택과 비용](#설계-선택과-비용)
- [관련 코드 및 문서](#관련-코드-및-문서)

---

## 플레이어를 구성하는 객체

### PlayerState — 플레이어별 기록·스탯

현재 `ASonheimPlayerState` 생성자는 다음 컴포넌트를 생성합니다.

~~~cpp
// SonheimPlayerState.cpp — 실제 생성 필드
m_InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("Inventory"));
m_StatBonusComponent = CreateDefaultSubobject<UStatBonusComponent>(TEXT("StatBonus"));
m_PalInventoryComponent = CreateDefaultSubobject<UPalInventoryComponent>(TEXT("PalInventory"));
m_MonsterDexComponent = CreateDefaultSubobject<UMonsterDexComponent>(TEXT("MonsterDex"));
m_QuestComponent = CreateDefaultSubobject<UQuestComponent>(TEXT("Quest"));
~~~

| 컴포넌트 | 데이터·역할 |
|---|---|
| `UInventoryComponent` | 인벤토리 슬롯, 장비 목록, 현재 무기 슬롯 |
| `UStatBonusComponent` | 장비 출처별 스탯 Modifier·무기 슬롯 보너스 |
| `UPalInventoryComponent` | 보유 Pal 목록·선택 슬롯 |
| `UMonsterDexComponent` | 발견·처치·포획·보상 수령 기록 |
| `UQuestComponent` | 퀘스트 단계·목표 진행·완료 기록 |

각 컴포넌트의 **복제 대상과 저장 방식은 같지 않습니다.** 예를 들어 도감·퀘스트는 소유 플레이어에게 변경 기록을 복제하고 서버 저장 기능도 사용합니다. PlayerState에 배치했다는 사실 자체가 SaveGame 저장을 보장하지는 않습니다.

### Pawn — 캐릭터의 현재 행동과 외형

`ASonheimPlayer`는 `AAreaObject`를 상속하며 기본 캐릭터 상태와 플레이어 전용 기능을 결합합니다.

~~~text
ASonheimPlayer : AAreaObject
 ├─ AAreaObject
 │   ├─ UHealthComponent / UStaminaComponent
 │   ├─ UConditionComponent / ULevelComponent
 │   ├─ USonheimSkillComponent
 │   └─ UMoveUtilComponent / URotateUtilComponent
 └─ ASonheimPlayer
     ├─ CharacterMovement / Animation / Camera
     ├─ WeaponMesh / PalSphereMesh / GliderMesh
     ├─ UInteractionComponent
     ├─ UPalCaptureComponent
     └─ UPalPartnerSkillComponent
~~~

HP 조작은 `AAreaObject::DecreaseHP`처럼 캐릭터 공통 API에서 시작하고, 실제 값은 `UHealthComponent`가 관리합니다. Pal 보유 목록은 PlayerState에, PalSphere 투척·Partner 스킬처럼 Pawn과 함께 움직이는 실행 기능은 플레이어 컴포넌트에 둡니다.

---

## 장비 변경이 스탯·스킬·외형에 반영되는 흐름

**하나의 무기 슬롯 전환이 세 경로로 이어집니다.**

~~~mermaid
flowchart TB
    INPUT["무기 전환 입력"] --> INV["UInventoryComponent<br/>SwitchWeaponSlot"]
    INV --> ACTIVE["CurrentWeaponSlot 변경"]
    ACTIVE --> STATS["UStatBonusComponent<br/>SetCurrentWeaponSlot"]
    STATS --> MOD["활성 슬롯 Modifier 적용"]
    MOD --> PLAYERSTATE["ASonheimPlayerState<br/>ModifiedStat · OnPlayerStatsChanged"]
    PLAYERSTATE --> PAWN["ASonheimPlayer::StatChanged"]
    ACTIVE --> NOTIFY["NotifyWeaponSlot<br/>OnWeaponChanged"]
    NOTIFY --> WEAPON["ASonheimPlayer<br/>SelectedWeapon · Mesh / Anim"]
    NOTIFY --> SKILL["USonheimSkillComponent<br/>ReplaceGrant"]
    WEAPON --> HUD["무기 타입·크로스헤어"]
~~~

### 1. 현재 슬롯에 해당하는 보너스만 적용

`UInventoryComponent::SwitchWeaponSlot`은 서버에서 현재 무기 슬롯을 바꾸고 `UStatBonusComponent::SetCurrentWeaponSlot`을 호출합니다.

`SetCurrentWeaponSlot`은 **이전 무기의 Modifier를 제거하고 새로운 활성 무기의 Modifier를 추가**합니다. 장착돼 있지만 선택하지 않은 다른 무기 슬롯의 보너스는 `WeaponSlotBonuses`에 보관했다가 선택될 때 적용합니다.

### 2. 선택한 무기의 스킬 사용 권한 교체

`UInventoryComponent::NotifyWeaponSlot`은 현재 `ItemID`의 `FItemData::EquipmentData.SkillID`를 조회하고 `USonheimSkillComponent::ReplaceGrant`를 호출합니다.

~~~cpp
// InventoryComponent.cpp — 선택 무기 스킬 변경부 발췌
TArray<int32> Skills;
if (SkillId > 0)
{
    Skills.Add(SkillId);
}
Comp->ReplaceGrant(ActiveWeaponGrantId, Skills);
~~~

`ActiveWeaponGrantId`로 무기 출처의 스킬 집합을 교체하므로, **기본 보유 스킬이나 다른 Grant에서 제공한 스킬과 구분**할 수 있습니다. Grant 참조 카운트·스킬 객체의 수명은 [[05. Combat, Skill & Animation|05_Combat_Skill_Animation]]에서 설명합니다.

### 3. 다른 플레이어도 무기 변경 결과를 확인

`OnWeaponChanged`는 `ASonheimPlayer::UpdateSelectedWeapon`으로 전달됩니다. 서버에서 `SelectedWeaponSlot`·`CurrentWeaponItemID`를 변경하고, `OnRep_SelectedWeapon`이 `FItemData`의 무기 Mesh와 Animation 설정을 적용합니다.

~~~text
OnWeaponChanged(Slot, ItemID)
    ↓ UpdateSelectedWeapon (Server)
SelectedWeaponSlot / CurrentWeaponItemID
    ↓ OnRep_SelectedWeapon
UpdateWeaponMesh 또는 ClearWeaponMesh
    ↓
WeaponMesh + AnimInstance 정보
    ↓
로컬 CurrentWeaponType / 크로스헤어 반영
~~~

**Inventory의 아이템 기록**, **현재 무기 외형**, **스킬 사용 권한**을 하나의 Widget에서 임의로 변경하지 않고 각 소유 시스템에 전달하는 구조입니다.

---

## 기본 스탯과 Source별 Modifier

### PlayerState에서 최종 스탯 계산

`ASonheimPlayerState`는 서버에서 `BaseStat`을 초기화합니다. `UStatBonusComponent`는 각 스탯별 Modifier를 관리하며, `PlayerState::UpdateStat / UpdateStats`가 최종 값을 계산합니다.

~~~text
BaseStat
   + UStatBonusComponent::StatBonuses
   ↓ GetModifiedStatValue
ModifiedStat
   ├─ OnPlayerStatsChanged → Pawn::StatChanged
   └─ ReplicatedStats → OnRep_ReplicatedStats → 클라이언트 UI
~~~

`UStatBonusComponent::GetModifiedStatValue`는 Additive를 기본값에 더한 다음, Multiplicative 보너스를 곱하고, Override가 있으면 그 값을 최종 값으로 사용합니다.

~~~cpp
// StatBonusComponent.h — Modifier 필드 발췌
struct FStatModifier
{
    EAreaObjectStatType StatType;
    float Value;
    EStatModifierType ModifierType;
    int SourceID;
};
~~~

장비가 등록한 Modifier는 `SourceID`(아이템 ID)로 추적하므로 `RemoveAllBonusesFromSource(ItemID)`로 **특정 아이템에서 적용한 보너스만 해제**할 수 있습니다.

### 최종 값을 Pawn의 컴포넌트에 반영

`ASonheimPlayer::BindDelegates`는 서버에서 PlayerState의 `OnPlayerStatsChanged`를 `StatChanged`에 연결합니다.

| 최종 스탯 | `ASonheimPlayer::StatChanged` 적용 |
|---|---|
| `HP` | `UHealthComponent::SetMaxHP` |
| `Attack` · `Defense` | `m_Attack` · `m_Defence` |
| `RunSpeed` | `CharacterMovement::MaxWalkSpeed` |
| `JumpHeight` | `CharacterMovement::JumpZVelocity` |

위 표는 **현재 `StatChanged`에서 실제로 처리하는 스탯**입니다. 작업 속도·스태미나·최대 무게 등의 최종 값은 PlayerState에 계산·보관되지만, 이 함수의 switch에 모두 포함된 것은 아닙니다.

이를 통해 **장비 Modifier와 스탯 계산은 PlayerState 측**, **전투·이동 값의 적용은 Pawn 측**에서 수행합니다.

---

## Action State로 입력 범위 제어

`ASonheimPlayer`는 `EPlayerState`와 `FActionRestrictions`를 사용해 동작별 허용 여부를 조회합니다.

~~~cpp
// SonheimPlayer.h — 주요 필드
struct FActionRestrictions
{
    bool bCanLook = true;
    bool bCanMove = true;
    bool bCanRotate = true;
    bool bCanOnlyRotate = false;
    bool bCanAction = true;
};
~~~

| 상태 | 이동 | 행동 | 실제 처리 의도 |
|---|---|---|---|
| `NORMAL` | 허용 | 허용 | 일반 플레이 |
| `ONLY_ROTATE` | 제한 | 제한 | 이동 입력을 위치 이동 대신 대상 방향 회전으로 사용 |
| `ACTION` | 허용 | 제한 | 새 행동 시작 제한 |
| `CANACTION` | 제한 | 허용 | 연계 행동·회피 등을 위한 상태 |
| `DIE` | 제한 | 제한 | 사망 |
| `GLIDING` | 제한 | 제한 | 일반 이동 입력 대신 글라이더 로직 |

*`ACTION`의 `bCanMove=false`는 현재 코드에서 주석 처리돼 있습니다. 따라서 이 상태가 이동까지 차단한다고 설명하지 않습니다.*

### 상태와 실제 입력 검사

`InitializeStateRestrictions`가 상태별 설정을 채우고, `CanPerformAction(CurrentPlayerState, "Move")`처럼 필요한 제약을 조회합니다.

~~~cpp
// SonheimPlayer.cpp — Move 내부
if (CanPerformAction(CurrentPlayerState, "Move"))
{
    AddMovementInput(ForwardDirection, MovementVector.Y);
    AddMovementInput(RightDirection, MovementVector.X);
}
if (CanPerformAction(CurrentPlayerState, "OnlyRotate"))
{
    LookAtLocation(targetLocation, EPMRotationMode::Speed, 1000.f);
}
~~~

`SetPlayerState`는 현재 상태를 변경하고 `bOrientRotationToMovement`를 갱신합니다. **이동 가능 여부는 `Move`의 입력 검사에서 적용**하며, 상태 전환만으로 `CharacterMovement::DisableMovement`를 호출하는 방식은 아닙니다.

### Montage와 행동 가능 시점

`USetPlayerStateNotify`는 Montage의 타이밍에 맞춰 `ACTION / CANACTION / NORMAL` 등으로 상태를 전환합니다.

~~~text
공격 Montage
  → ACTION : 새 행동 제한
  → CANACTION : 연계 행동 허용
  → NORMAL : 일반 동작 복귀
~~~

이 흐름은 해당 Notify를 배치한 공격의 **입력 가능 구간을 애니메이션 타임라인에서 조절**하기 위한 것입니다. 실제 스킬 실행과 타격 Notify는 [[05. Combat, Skill & Animation|05_Combat_Skill_Animation]]에서 설명합니다.

---

## Condition Bitmask와 시간 관리

`UConditionComponent`는 조합 가능한 캐릭터 상태를 `uint32 ConditionFlags`로 관리합니다.

~~~cpp
// SonheimGameType.h
enum class EConditionBitsType : uint32
{
    None       = 0,
    Dead       = 1 << 0,
    Invincible = 1 << 1,
    Hidden     = 1 << 2,
};
~~~

`AddCondition`은 서버에서 `ConditionFlags`에 비트를 추가하고, `RemoveCondition`은 해당 비트를 제거합니다. `AAreaObject::CanAttack`·`TakeDamage` 등은 `HasCondition`을 이용해 사망·무적·숨김 상태를 확인합니다.

### 같은 종류의 Condition을 다시 적용할 때

`UConditionComponent::AddCondition(Type, Duration)`에서 같은 비트가 이미 켜져 있고 Duration이 양수라면 **기존 Timer를 지우고 새 만료 Timer를 설정**합니다. Duration이 끝나면 `RemoveConditionInternal`이 비트를 제거합니다.

| 역할 | 현재 구현 |
|---|---|
| 중복 Condition 표시 | 비트 하나로 존재 여부 표현 |
| Duration | `TMap<EConditionBitsType, FTimerHandle>` |
| 복제 | `ConditionFlags` 프로퍼티 복제 |
| 클라이언트 반영 | `OnRep_ConditionFlags`에서 추가·제거된 비트 계산 |

**한 종류의 Condition에 여러 출처의 독립 만료 시간을 쌓는 구조는 아닙니다.** 같은 종류는 Timer 하나를 갱신하는 방식입니다.

---

## 이동·무기 상태와 복제

위치·이동은 `CharacterMovement`를 사용하고, 프로젝트 고유의 현재 행동 표현은 `ASonheimPlayer`의 복제 변수와 `OnRep`로 처리합니다.

| 복제 변수 | 화면·이동에 반영되는 내용 |
|---|---|
| `bIsSprinting` | `OnRep_IsSprinting → ApplySprinting`으로 이동 속도 적용 |
| `bIsLockOn` | `OnRep_IsLockOn`으로 회전 정책·Animation 상태 적용 |
| `bIsGliding` | `OnRep_IsGliding → ApplyGliderState`로 글라이더 Mesh·Movement 설정 |
| `SelectedWeaponSlot` · `CurrentWeaponItemID` | `OnRep_SelectedWeapon`으로 무기 Mesh·Animation 갱신 |
| `bWeaponVisible` | `OnRep_WeaponVisible`로 무기 표시 전환 |
| `CurrentWeaponType` | `COND_OwnerOnly` 복제, 로컬 크로스헤어 갱신 |

`Sprint_Pressed / Released`는 로컬에서도 `ApplySprinting`을 먼저 호출해 입력 피드백을 주고, `Server_SetSprinting` 이후 복제값을 반영합니다. 서버 `Sprint_Triggered`는 스태미나를 소모하고 부족하면 Sprint를 종료합니다.

글라이더는 `Server_SetGliding`에서 허용 여부를 검사한 뒤 상태를 복제합니다. `ApplyGliderState`는 글라이더 Mesh·Animation·중력·공중 제어를 변경하고, `UpdateGliding`은 활성 상태의 이동 속도를 갱신합니다.

이 문서는 **플레이어 상태가 이동·외형에 미치는 영향**에 초점을 둡니다. 네트워크 전송 조건·예측의 상세는 [[12. Multiplayer Synchronization|12_Multiplayer_Synchronization]]에 있습니다.

---

## PlayerState 연결 시점

Pawn과 PlayerState의 초기화 시점이 항상 같지는 않습니다. 특히 원격 클라이언트의 `PlayerState`가 늦게 준비되는 경우를 위해 **서버 Possess 경로와 클라이언트 복제 경로**에서 연결을 수행합니다.

~~~mermaid
flowchart TB
    SERVER["서버 PossessedBy"] --> BIND["S_PlayerState 확보<br/>InitializeByPlayerState"]
    SERVER --> DELEGATE["BindDelegates<br/>OnWeaponChanged · OnPlayerStatsChanged"]
    CLIENT["클라이언트 OnRep_PlayerState"] --> SET["S_PlayerState 확보"]
    SET --> INIT["InitializeByPlayerState"]
    INIT --> PAL["PalCaptureComponent / PalPartnerSkillComponent"]
    SET --> HUD["로컬 InitializeHUD"]
~~~

`ASonheimPlayer::InitializeByPlayerState`는 다음 두 컴포넌트에 PlayerState를 다시 연결합니다.

~~~cpp
PalCaptureComponent->InitializeWithPlayerState(S_PlayerState);
PalPartnerSkillComponent->InitializeWithPlayerState(S_PlayerState);
~~~

Pawn이 PlayerState를 참조해야 하는 지점을 명시적으로 두어 **Pal 보유 기록과 현재 Pawn의 포획·소환 기능을 연결**합니다. Pal의 실제 서버 판정과 소유 목록 변경은 [[08. Pal Capture & Partner Lifecycle|08_Pal_Capture_Partner_Lifecycle]]에서 설명합니다.

---

## 설계 선택과 비용

| 선택 | 얻은 효과 | 관리할 부분 |
|---|---|---|
| **PlayerState / Pawn 상태 분리** | 플레이어별 보유 기록과 현재 캐릭터 실행 상태 분리 | Possess·복제 시 PlayerState 재연결 |
| **장비 이벤트 기반 연동** | 무기 하나를 바꿔도 스탯·스킬·Mesh가 각 담당 코드에서 갱신 | 이벤트 연결·초기 동기화 시점 |
| **SourceID Modifier** | 장비 출처별 보너스 제거 | 아이템 ID·무기 슬롯 구분과 Modifier 갱신 |
| **Action State + Restrictions** | 입력별 bool 분기 대신 상태별 허용 정책 조회 | 상태 전환과 입력 검사 위치 관리 |
| **ConditionFlags + Timer** | 조합 가능한 상태와 시간 만료 관리 | 같은 Condition에 출처별 독립 Duration은 없음 |
| **상태 복제 + OnRep** | 클라이언트 무기·이동·외형 갱신 | 로컬 입력 피드백과 서버 결과 정합성 |

장비·행동·조건 상태를 하나의 거대한 Player 클래스에 모두 저장하지 않고, **각 상태의 원본은 컴포넌트에 두고 캐릭터의 실제 행동 변경만 Pawn에 반영**하도록 구성했습니다.

## 관련 코드 및 문서

- [SonheimPlayerState.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/AreaObject/Player/SonheimPlayerState.cpp) — 보유 컴포넌트·스탯 계산·이벤트
- [SonheimPlayer.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/AreaObject/Player/SonheimPlayer.cpp) · [SonheimPlayer.h](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/AreaObject/Player/SonheimPlayer.h) — 상태 제한·무기 외형·복제·연결
- [AreaObject.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/AreaObject/Base/AreaObject.cpp) — 캐릭터 공통 컴포넌트
- [StatBonusComponent.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/AreaObject/Attribute/StatBonusComponent.cpp) — Modifier·Source·활성 무기 슬롯
- [InventoryComponent.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/AreaObject/Player/Utility/InventoryComponent.cpp) — 무기 전환·Stat/Skill 연동
- [ConditionComponent.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/AreaObject/Attribute/ConditionComponent.cpp) — ConditionFlags·Timer
- [SetPlayerStateNotify.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/Animation/Player/SetPlayerStateNotify.cpp) — Animation에 따른 행동 상태 전환
- [[02. Gameplay Architecture|02_Gameplay_Architecture]] — PlayerState·Pawn·컴포넌트 책임
- [[05. Combat, Skill & Animation|05_Combat_Skill_Animation]] — Skill Grant·Montage Notify·타격 판정
- [[07. Inventory & Crafting|07_Inventory_Crafting]] — 아이템·장비·제작
- [[08. Pal Capture & Partner Lifecycle|08_Pal_Capture_Partner_Lifecycle]] — Pal 소유·포획·Partner 기능
- [[12. Multiplayer Synchronization|12_Multiplayer_Synchronization]] — 복제 범위와 서버 권한
