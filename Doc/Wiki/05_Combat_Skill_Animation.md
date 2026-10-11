# 05. Combat, Skill & Animation

Sonheim의 전투는 **스킬 사용 가능 여부와 비용을 서버에서 검사하고, Montage Notify가 정한 시점에 효과·타격 판정을 실행**합니다. 스킬 설정은 `FSkillData`, 보유·시전 상태는 `USonheimSkillComponent`, 실제 실행은 `UBaseSkill`이 담당하며, 타격 결과는 `FAttackData → FCustomDamageEvent → TakeDamage` 경로로 처리합니다.

## 전투 시연과 핵심 구현

![Montage의 SkillFire·근접 공격 구간·행동 상태 Notify](https://github.com/user-attachments/assets/aa7f6632-c6a6-498a-8eab-9692620a507d)

*Montage에서 발동·타격 구간·행동 가능 상태 전환 시점을 지정합니다. 타격은 Notify로 활성화한 판정 구간에서 서버가 수행합니다.*

> **플레이 영상 자리 — 스킬 사용에서 타격 결과까지**  
> `Doc/Media/Wiki/05_Combat_Skill_Animation/05_skill_cast_to_damage.mp4`  
> 실제 플레이에서 공격 입력 → Montage/Notify → 근접 타격 → HP 감소·피격 반응을 한 흐름으로 보여주고, 가능하면 같은 공격의 서버 판정 디버그 화면도 포함합니다. 확보 후 이 자리를 실제 영상으로 대체합니다.

| 구분 | 담당 코드 | 구현 내용 |
|---|---|---|
| **정의 데이터** | `FSkillData` / `FAttackData` | 스킬 클래스, Phase별 비용, Montage, 타격 형태·데미지 |
| **허용 여부·요약 상태** | `USonheimSkillComponent` | 기본 보유·Grant 검증, `SkillSpecs` FastArray |
| **스킬 실행** | `UBaseSkill` | `CanCast → Activate → Fire → Complete/Cancel`, 쿨타임 |
| **애니메이션 시점** | AnimNotify / NotifyState | 발사·타격 구간·행동 가능 상태 전환 |
| **근접 판정** | `UMeleeAttack` | Socket 기반 Sweep, 보간, 중복 타격 방지 |
| **데미지** | `AAreaObject::CalcDamage / TakeDamage` | 피아식별, 속성·약점·방어, HP/스태미나, 효과 |
| **네트워크** | FastArray + `UBaseSkill` 서브오브젝트 | 스킬 요약 상태와 실행 객체의 복제 상태 구분 |

## 목차

- [스킬 실행 흐름](#스킬-실행-흐름)
- [스킬 정의·상태·실행 객체](#스킬-정의상태실행-객체)
- [기본 스킬과 장비 Grant](#기본-스킬과-장비-grant)
- [스킬 사용 조건과 Phase별 비용](#스킬-사용-조건과-phase별-비용)
- [Montage와 AnimNotify](#montage와-animnotify)
- [근접 공격의 판정과 프레임 보간](#근접-공격의-판정과-프레임-보간)
- [데미지 계산과 피격 처리](#데미지-계산과-피격-처리)
- [스킬 상태의 네트워크 동기화](#스킬-상태의-네트워크-동기화)
- [구현 선택과 비용](#구현-선택과-비용)
- [관련 코드 및 문서](#관련-코드-및-문서)

---

## 스킬 실행 흐름

~~~mermaid
flowchart TB
    INPUT["플레이어 입력 / AI 요청"] --> TRY["USonheimSkillComponent<br/>TryCastSkillById"]
    TRY --> SERVER["서버 허용 여부 · CanCast 검사"]
    DEF["FSkillData<br/>비용 · Montage · AttackData"] --> SERVER
    SERVER --> ACT["UBaseSkill::Activate<br/>OnActivate 비용 적용"]
    ACT --> SPEC["SkillSpecs 상태 갱신"]
    ACT --> MONTAGE["MultiCast_CastSkill<br/>Montage 재생"]
    MONTAGE --> NOTIFY["AnimNotify / NotifyState"]
    NOTIFY --> FIRE["서버 UBaseSkill::Fire<br/>OnFire 비용 적용"]
    NOTIFY --> WINDOW["UMeleeAttack<br/>판정 구간 시작·종료"]
    WINDOW --> TICK["서버 Tick · Socket Sweep"]
    TICK --> DAMAGE["CalcDamage → FCustomDamageEvent"]
    DAMAGE --> TARGET["TakeDamage<br/>HP · Stamina · 피격 효과"]
    MONTAGE --> FINISH["Complete / Cancel<br/>OnComplete 비용 · CoolTime"]
    FINISH --> SPEC
~~~

서버 요청과 실제 실행은 별개입니다. `TryCastSkillById`는 클라이언트에서 `Server_TryCastSkill`을 호출하며, 서버가 `IsSkillAllowed`와 `CanCastSkill`을 다시 확인합니다. 실행에 성공하면 `OnServerSkillActivated`로 복제 상태를 갱신하고 `AAreaObject::MultiCast_CastSkill`로 Montage를 재생합니다.

`AAreaObject::Tick`은 서버에서 현재 `UBaseSkill::Tick`을 호출합니다. 따라서 근접 공격은 **Montage Notify가 판정 구간을 열고, 서버 Tick이 해당 구간의 충돌을 검사**하는 구조입니다.

---

## 스킬 정의·상태·실행 객체

### DataTable — FSkillData

`FSkillData : FTableRowBase`는 여러 스킬이 공통으로 갖는 정의 필드입니다.

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

한 행의 `SkillClass`가 실행 로직을 정하고, `Montage`와 `AttackData`가 공격 시점·범위·데미지를 연결합니다. `NextSkillID`는 연계 스킬을 지정합니다. 행 구조와 에셋 분류 기준은 [[03. Data & Content Architecture|03_Data_Content_Architecture]]에서 설명합니다.

### SkillSpecs — 보유·시전 요약 상태

`USonheimSkillComponent`는 다음 `FSonheimSkillSpecItem`을 FastArray로 관리합니다.

~~~cpp
struct FSonheimSkillSpecItem : public FFastArraySerializerItem
{
    int32 SkillId = 0;
    int32 Level = 1;
    bool bIsCasting = false;
    float CooldownEndTime = 0.0f;
};
~~~

`OnServerSkillActivated`, `OnServerSkillCooldownStart`, `OnServerSkillCancelled`가 서버에서 엔트리를 갱신하고 `MarkItemDirty`를 호출합니다.

### UBaseSkill — 실행과 Phase

`UBaseSkill`은 `CanCast`·`Activate`·`Fire`·`Complete`·`Cancel`을 제공하고, `UMeleeAttack` 등의 파생 클래스가 타격 방식을 구현합니다.

~~~text
Ready
  ↓ Activate
Casting
  ↓ Fire
PostCasting
  ↓ Complete
CoolTime
  ↓ 쿨타임 종료
Ready

Cancel → CoolTime
~~~

이 Phase는 실행 객체의 상태입니다. 위의 `SkillSpecs`는 별도의 시전 여부·종료 시각 요약을 저장합니다. **두 구조를 하나의 상태 객체로 취급하지 않습니다.**

---

## 기본 스킬과 장비 Grant

### 스킬 인스턴스의 생성·재사용

`USonheimSkillComponent::EnsureSkillInstance`는 `SkillId`에 해당하는 객체가 있으면 재사용하고, 없다면 DataTable의 `SkillClass`로 `UBaseSkill`을 생성해 `SkillInstances`에 보관합니다.

~~~cpp
// SonheimSkillComponent.cpp — 생성부 발췌
const FString NameStr = FString::Printf(TEXT("BaseSkill_%d"), SkillId);
UBaseSkill* NewSkill =
    NewObject<UBaseSkill>(OwnerArea, Data->SkillClass, *NameStr);
NewSkill->InitSkill(Data);
SkillInstances.Add(SkillId, NewSkill);
~~~

생성 시점은 다음과 같습니다.

- **기본 보유 스킬:** `AAreaObject::BeginPlay`에서 `m_OwnSkillIDSet`을 순회하며 `EnsureSkillInstance` 호출
- **장비 등으로 부여한 스킬:** `GrantSkills / ReplaceGrant` 시점에 생성·캐시
- **추가 조회:** `GetSkillByID / TryCastSkillById`에서도 필요하면 `EnsureSkillInstance` 호출

즉, **모든 스킬을 사용 직전에만 생성하는 완전한 지연 생성 방식은 아닙니다.** 보유 스킬 집합·Grant를 기준으로 준비하면서 누락된 객체를 필요 시 생성하는 구조입니다.

### GrantId와 RefCount

장비를 변경할 때 다른 경로로 부여받은 스킬까지 해제하지 않도록 Grant를 구분합니다.

~~~cpp
TMap<FGuid, TArray<int32>> GrantsById;
TMap<int32, int32> GrantRefCounts;
~~~

~~~text
무기 A ─────┐
            ├── Skill 101 · RefCount 2
다른 Grant ┘

무기 A 해제 → RefCount 1 · 사용 허용 유지
마지막 Grant 해제 → RefCount 0 · 사용 허용 목록에서 제외
~~~

`USonheimSkillComponent::IsSkillAllowed`는 기본 `OwnedSkillIds` 또는 `GrantRefCounts`로 사용 가능 여부를 결정합니다. `ReplaceGrant`는 특정 Grant의 스킬 목록만 바꿉니다.

**RefCount가 0이 됐다고 `SkillInstances`의 UObject를 즉시 Destroy하는 것은 아닙니다.** 현재 구현의 회수는 주로 **사용 권한의 변경**이며, 저장한 인스턴스와 허용 여부를 구별해야 합니다.

---

## 스킬 사용 조건과 Phase별 비용

### 서버 실행 조건

`UBaseSkill::CanCast`는 현재 Phase가 `Ready`인지, `OnActivate` 단계의 스태미나·아이템을 지불할 수 있는지, 대상이 사거리 안인지 확인합니다. `AAreaObject::CanCastSkill`은 다른 스킬이 실행 중인지도 검사합니다.

~~~text
Server_TryCastSkill
    ↓ IsSkillAllowed
CanCastSkill
    ↓ Current Skill / Ready / Cost / Range
UBaseSkill::Activate
    ↓ ApplyCosts(OnActivate)
SkillSpecs 갱신 + Montage 재생
~~~

클라이언트에서 먼저 검사하거나 RPC를 보냈더라도, **서버에서 `Activate`가 성공해야 실제 시전 상태로 들어갑니다.**

### OnActivate·OnFire·OnComplete

`FSkillStaminaCost`와 `FSkillItemCost`는 비용을 소모할 Phase를 지정합니다.

| Phase | 발생 시점 | 사용 사례 |
|---|---|---|
| `OnActivate` | `Activate` | 스킬 시작 시 스태미나·아이템 비용 |
| `OnFire` | `Fire` | 발사 시점 탄약·소모 아이템 |
| `OnComplete` | `Complete` | 동작 완료 시점의 후속 비용 |

`CheckCosts`는 비용을 실제로 차감하지 않고 충족 여부를 검사합니다. `ApplyCosts`는 **서버 권한 경로**에서 비용을 실제로 적용합니다.

`ApplyCosts`는 같은 Phase에서 여러 아이템 중 일부를 차감한 뒤 실패하면 **그때까지 차감한 아이템을 `AddItem`으로 돌려줍니다.** 단, 앞서 소모한 스태미나까지 함께 되돌리지는 않으므로 전체 비용에 대한 원자적 트랜잭션은 아닙니다.

`OnFire` 비용 적용에 실패하면 `Fire`는 `Cancel`을 호출합니다. 실패·취소 후에는 스킬 실행 상태와 쿨타임을 처리합니다.

---

## Montage와 AnimNotify

타격 프레임·취소 가능 구간을 C++ 고정 타이머에만 의존하지 않고 **Animation 타임라인에 배치한 Notify**로 제어합니다.

| Notify | 실제 역할 |
|---|---|
| `USkillFireNotify` | 현재 스킬의 `Fire` 시점 전달. 서버면 직접 호출, 클라이언트면 `Server_NotifySkillFire` 요청 |
| `UMeleeAttackNotifyState` | 서버에서 근접 판정 구간 시작·종료 및 공격 데이터 인덱스 지정 |
| `USetPlayerStateNotify` | `NORMAL / ACTION / CANACTION` 등의 플레이어 행동 상태 전환 |
| `UAddConditionNotify` | Invincible 등 Condition 적용 |

~~~text
Montage
 ├─ 행동 시작 / ACTION
 ├─ SkillFireNotify → Fire
 ├─ MeleeAttackNotifyState Begin → 타격 구간
 │     └─ Server Tick → ProcessHitDetection
 ├─ MeleeAttackNotifyState End → 판정·LOD 복원
 ├─ CANACTION → 연계 입력 / 회피 가능 상태
 └─ NORMAL → 일반 조작 복원
~~~

위 순서는 **타임라인에서 구성할 수 있는 역할의 예시**이며, 모든 Montage에 동일한 Notify 배치가 강제되는 것은 아닙니다.

특히 `USkillFireNotify`는 Authority에서 `Fire`를 직접 실행하고, 비권한 측에서는 현재 스킬 ID를 서버에 전달합니다. 근접 판정 NotifyState의 `SetCasterMesh`와 `ResetCollisionData`는 서버에서만 처리합니다.

### 행동·취소 가능 구간

`USetPlayerStateNotify`는 `ASonheimPlayer::SetPlayerState`를 호출해 공격 중 이동·다음 행동 제한을 조정합니다. `CANACTION`과 `NORMAL` 전환 시에는 `ClearCurrentSkill`도 호출합니다.

이처럼 Animation은 **언제 행동 제한을 풀지**를 결정하고, 캐릭터 상태와 `FActionRestrictions`가 **실제로 어떤 입력이 가능한지**를 관리합니다. 플레이어 상태 규칙은 [[04. Player & Character Systems|04_Player_Character_Systems]]에서 다룹니다.

---

## 근접 공격의 판정과 프레임 보간

### NotifyState가 서버 판정 구간을 연다

`UMeleeAttackNotifyState::NotifyBegin`이 `AttackDataIndex`를 이용해 `UMeleeAttack::SetCasterMesh`를 호출합니다. 이후 `AAreaObject::Tick`에서 호출되는 `UMeleeAttack::Tick`이 해당 인덱스의 `ProcessHitDetection`을 실행합니다. `NotifyEnd`는 판정 데이터를 정리합니다.

~~~mermaid
flowchart TB
    BEGIN["NotifyBegin · AttackDataIndex"] --> SETUP["SetCasterMesh<br/>Source Mesh · Socket · LOD 설정"]
    SETUP --> LOOP["서버 Tick<br/>ProcessHitDetection"]
    LOOP --> TRACE["Line / Sphere / Capsule / Box"]
    TRACE --> FILTER["HitActors · ProcessedActors<br/>중복 제거"]
    FILTER --> DMG["CalcDamage"]
    ENDING["NotifyEnd / Cancel / Complete"] --> RESET["ResetCollisionData<br/>LOD 복원 · 판정 종료"]
    LOOP -. "판정 창 유지" .-> LOOP
~~~

`FHitBoxData`는 판정 방식, 메시 컴포넌트 태그, 시작·끝 Socket, 크기와 보간 여부를 정의합니다. 지정된 `MeshComponentTag`로 무기 Mesh의 Socket도 선택할 수 있습니다.

| `EHitDetectionType` | 실제 호출 |
|---|---|
| `Line` | `LineTraceMultiByChannel` |
| `Sphere` | `SweepMultiByChannel` / Sphere |
| `Capsule` | `SweepMultiByChannel` / Capsule |
| `Box` | `SweepMultiByChannel` / Box |

### 빠른 공격의 프레임 간격 보완

빠른 무기 Swing은 프레임 사이에 Socket이 크게 움직입니다. 현재 프레임에서만 Sweep하면 이전 위치와 현재 위치 사이를 건너뛰는 판정이 생길 수 있습니다.

~~~text
이전 프레임 Socket ● ─ ─ ─ X(대상) ─ ─ ─ ● 현재 프레임

bUseInterpolation = true
이전 Socket ● ─ ● ─ ● ─ ● ─ 현재 Socket
               중간 위치별 추가 Sweep
~~~

`UMeleeAttack::ProcessHitDetection`은 `bUseInterpolation`이 켜져 있고 이전 Socket 위치가 유효하면 시작·끝 위치와 회전을 `InterpolationSteps`만큼 보간해 단계별 충돌 검사를 수행합니다. 첫 프레임처럼 이전 위치가 없는 경우에는 현재 위치에서 검사합니다.

~~~cpp
// MeleeAttack.cpp — 주요 로직 발췌
const int32 Steps = FMath::Max(2,
    AttackData->HitBoxData.InterpolationSteps);
for (int32 i = 1; i <= Steps; ++i)
{
    const float Alpha = float(i) / Steps;
    const FVector Start = FMath::Lerp(PreviousStart, CurrentStart, Alpha);
    const FVector End = FMath::Lerp(PreviousEnd, CurrentEnd, Alpha);
    // 보간 위치·회전에서 PerformCollisionCheck(...)
}
~~~

실제 필드명과 판정 흐름에 맞춘 **설명용 축약 코드**입니다. 실행 가능한 원본 함수는 아래 소스 링크에서 확인할 수 있습니다.

### 다중 Sweep의 중복 타격 방지

- `ProcessedActors`: **현재 `ProcessHitDetection` 호출**에서 같은 Actor를 중복 처리하지 않음
- `AttackCollision::HitActors`: **하나의 Notify 판정 구간 전체**에서 이미 타격한 Actor를 제외

따라서 보간 단계 수가 늘어나더라도 **동일 판정 창에서 같은 대상에 중복 데미지가 들어가지 않도록** 처리합니다. 판정 창이 새로 열리면 별도 기록을 사용합니다.

### Socket 정확도와 LOD

서버는 화면에 보이지 않는 캐릭터라도 타격 Socket 위치가 필요합니다.

~~~cpp
// AreaObject.cpp
GetMesh()->VisibilityBasedAnimTickOption =
    EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
GetMesh()->bEnableUpdateRateOptimizations = false;
~~~

또한 근접 판정 창에서 Source SkeletalMesh에 `SetForcedLOD(1)`을 적용해 가장 높은 LOD(LOD0)의 Socket 위치를 사용하고, `ResetCollisionData`에서 이전 LOD 설정으로 복원합니다.

이는 **서버의 타격 판정 정확도를 높이는 대신 Animation·Bone 갱신과 추가 Sweep 비용을 부담**하는 선택입니다.

> **디버그 캡처 자리 — Socket 보간과 Hit Shape**  
> `Doc/Media/Wiki/05_Combat_Skill_Animation/05_melee_trace_interpolation.png`  
> 실제 `bShowDebug` 상태에서 동일 공격의 판정 Sphere/Line 등과 대상 적중 위치가 보이도록 촬영합니다. 보간 적용 여부·`InterpolationSteps`는 사용한 스킬의 실제 DataTable 값과 함께 기록합니다.

---

## 데미지 계산과 피격 처리

공격의 타격 정보는 `FAttackData`에 정의합니다.

~~~cpp
// SonheimGameType.h — 주요 필드 발췌
struct FAttackData
{
    float HealthDamageAmountMin = 0.f;
    float HealthDamageAmountMax = 0.f;
    float StaminaDamageAmount = 0.f;
    EElementalAttribute AttackElementalAttribute;
    FHitBoxData HitBoxData;
    bool bEnableHitStop = false;
    float HitStopDuration = 0.1f;
    float KnockBackForce = 0.f;
    // VFX / SFX 필드 등
};
~~~

타격에 성공하면 `AAreaObject::CalcDamage`가 공격 가능 대상인지 먼저 검사하고, 공격자의 데미지 보정 및 동일 속성 보너스를 적용한 뒤 `FCustomDamageEvent`에 `FAttackData`·`HitInfo`·계산한 Damage를 담아 대상의 `TakeDamage`에 전달합니다.

~~~mermaid
flowchart TB
    HIT["UMeleeAttack / Projectile / Boss Strike"] --> CALC["AAreaObject::CalcDamage<br/>IFF 검사 · 기본 피해 · 동일 속성 보너스"]
    CALC --> EVENT["FCustomDamageEvent<br/>FAttackData + HitInfo + Damage"]
    EVENT --> TARGET["AAreaObject::TakeDamage<br/>서버 권한 · 피아식별 · 생존/상태 검사"]
    TARGET --> REACTION["Custom Damage 확인<br/>Hit Stop · 넉백"]
    REACTION --> MITIGATE["방어 보정 → 약점 ×1.5<br/>속성 상성 배율"]
    MITIGATE --> HP["Health / Stamina 갱신<br/>사망 판정"]
    HP --> FEEDBACK["MulticastDamageEffect<br/>피격 숫자 · VFX/SFX"]
~~~

`CalcDamage`의 **동일 속성 보너스는 공격자의 방어 속성 목록에 공격 속성이 있으면 1.2배**를 적용하는 현재 코드 기준입니다. 대상 `TakeDamage`는 별도로 방어 보정·약점(1.5배)·대상의 방어 속성에 대한 상성 배율을 적용합니다.

**Hit Stop과 넉백은 `TakeDamage` 내부에서 `FCustomDamageEvent` 정보를 확인한 뒤 처리**하며, 이후 실제 HP·Stamina 갱신과 데미지 표시·효과를 전달합니다. 단순 float 데미지뿐 아니라 공격의 맥락을 Target에 보존하는 구조입니다.

보스의 `ABossMonster::LandStrike`도 `FAttackData`와 기존 `CalcDamage`를 사용합니다. 보스가 추가하는 책임은 **어떤 패턴을 언제 어디에 실행할지**이며, 실제 대상 피해 처리 경로는 공유합니다. [[10. Boss Encounter Runtime|10_Boss_Encounter_Runtime]] 참고.

---

## 스킬 상태의 네트워크 동기화

**`FSonheimSkillSpecItem`만 복제하고 `UBaseSkill`은 복제하지 않는 구조가 아닙니다.** 현재 스킬은 두 복제 경로를 함께 사용합니다.

| 경로 | 복제 필드·대상 |
|---|---|
| `USonheimSkillComponent::SkillSpecs` | `SkillId`, `Level`, `bIsCasting`, `CooldownEndTime` — FastArray |
| `AAreaObject::ReplicateSubobjects` → `UBaseSkill` | `m_CurrentPhase`, `m_CurrentCoolTime`, `m_TargetPos`, `m_NextSkillID` — 실행 객체 프로퍼티 |
| `AAreaObject::MultiCast_CastSkill` | 스킬 사용 Montage 재생 — 순간 연출 |

`EnsureSkillInstance`는 `BaseSkill_<ID>`의 안정적인 UObject 이름을 사용하고, `AAreaObject::GetSubobjectsWithStableNamesForNetworking`에서 스킬 객체를 등록합니다. 실제 복제는 `AAreaObject::ReplicateSubobjects`에서 `Channel->ReplicateSubobject`를 호출합니다.

따라서 **서버가 실행을 승인하는 권한**, **클라이언트에 유지하는 상태**, **몽타주를 시작하는 Multicast**를 각각 구분해야 합니다. 네트워크 관련성·OwnerOnly·Snapshot과의 비교는 [[12. Multiplayer Synchronization|12_Multiplayer_Synchronization]]에서 다룹니다.

---

## 구현 선택과 비용

| 선택 | 구현상 이점 | 관리 비용·주의점 |
|---|---|---|
| **정의 / 실행 객체 / 복제 상태 분리** | DataTable로 서로 다른 스킬을 구성하고 서버 실행·화면 상태 구분 | SkillSpecs·UBaseSkill 사이 일부 상태 중복 |
| **기본 스킬 선생성 + 필요 시 인스턴스 확보** | 보유 스킬과 Grant 스킬을 재사용 | Grant 권한 변화와 캐시된 객체의 수명 구분 |
| **GrantId + RefCount** | 여러 출처가 같은 스킬을 제공해도 개별 회수 | Grant ID·카운트 일관성 |
| **Phase별 비용** | 시작·발사·완료 시점을 분리해 소모 | 스태미나 포함 전체 원자적 롤백은 아님 |
| **Montage Notify** | 공격 프레임과 입력·Cancel Window를 애니메이션에서 조절 | 서버 Anim/Bone 갱신과 Notify 타이밍 검증 |
| **프레임 간 Socket 보간** | 빠른 Swing의 타격 누락 위험 감소 | 추가 Sweep·중복 제거·디버깅 비용 |
| **동일한 Damage Context** | 플레이어·몬스터·보스의 피해 처리 공유 | AttackData의 범위·Socket·Index 설정 검증 필요 |

핵심은 **타격 시점은 Montage에 맞추되 실제 판정·비용·피해는 서버 게임플레이 경로에서 확정**한다는 점입니다. 콘텐츠는 `FSkillData`와 `FAttackData`로 조정하고, 복제·시각 효과는 각자의 목적에 맞는 경로로 전달합니다.

## 관련 코드 및 문서

- [SonheimGameType.h](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/ResourceManager/SonheimGameType.h) — `FSkillData`·`FAttackData`·`FHitBoxData`
- [SonheimSkillComponent.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/AreaObject/Skill/SonheimSkillComponent.cpp) — SkillSpecs·Grant·RPC·인스턴스
- [BaseSkill.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/AreaObject/Skill/Base/BaseSkill.cpp) — Phase·비용·완료·복제
- [AreaObject.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/AreaObject/Base/AreaObject.cpp) — 서버 Tick·시전·서브오브젝트 복제·데미지
- [MeleeAttack.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/AreaObject/Skill/Common/MeleeAttack.cpp) — Socket Sweep·보간·중복 타격 방지
- [SkillFireNotify.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/Animation/Common/Notify/SkillFireNotify.cpp) — Fire Notify 및 서버 경로
- [MeleeAttackNotifyState.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/Animation/Common/Notify/MeleeAttackNotifyState.cpp) — 서버 타격 구간
- [SetPlayerStateNotify.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/Animation/Player/SetPlayerStateNotify.cpp) — 공격 상태·취소 가능 시점
- [[03. Data & Content Architecture|03_Data_Content_Architecture]] — DataTable·정의 데이터 기준
- [[04. Player & Character Systems|04_Player_Character_Systems]] — Action State·장비·스탯
- [[07. Inventory & Crafting|07_Inventory_Crafting]] — 장비 교체·Skill Grant
- [[10. Boss Encounter Runtime|10_Boss_Encounter_Runtime]] — Boss Pattern·Strike에서 Damage Context 재사용
- [[12. Multiplayer Synchronization|12_Multiplayer_Synchronization]] — 스킬·상태 복제·RPC
