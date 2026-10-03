# 10. Combat, Skill & Animation

> **Skill의 정적 데이터, 네트워크 상태, 실행 객체와 Animation timing을 분리하고, 최종 Damage pipeline까지 하나의 흐름으로 연결합니다.**

Sonheim의 전투는 “Input에서 바로 Trace 후 Damage”로 끝나지 않습니다.

```text
Input / AI Decision
      ↓
USonheimSkillComponent
      ↓
Skill Spec + Server Validation
      ↓
UBaseSkill Logic
      ↓
Montage / AnimNotify
      ↓
Attack Execution
      ↓
FCustomDamageEvent
      ↓
AAreaObject::TakeDamage
      ↓
HP / Condition / Feedback
```

---

## 1. Skill Data / State / Logic 분리

한 Skill을 세 종류의 정보로 나눕니다.

### Data — `FSkillData`

DataTable에 저장되는 정적 값입니다.

예:

- SkillClass
- Montage
- CoolTime
- CastRange
- Costs
- AttackData
- NextSkillID

### State — `FSonheimSkillSpecItem`

Network에서 공유해야 할 lightweight runtime state입니다.

- SkillId
- Level
- bIsCasting
- CooldownEndTime

`FSonheimSkillSpecContainer : FFastArraySerializer`가 변경 entry만 복제합니다.

### Logic — `UBaseSkill`

실제 실행 방식은 UObject instance가 담당합니다.

- Activate
- Fire
- Complete
- Cancel
- Cost 처리
- Skill별 공격 구현

Logic object pointer array 자체를 network state로 사용하지 않고, **복제할 상태와 실행 객체를 분리**합니다.

---

## 2. Skill Instance는 필요할 때 생성한다

`USonheimSkillComponent::EnsureSkillInstance`는 SkillId가 실제 사용될 때 해당 `SkillClass`의 `UBaseSkill`을 생성하고 cache합니다.

```text
TryCastSkillById
    ↓
IsSkillAllowed
    ↓
EnsureSkillInstance
    ↓
CanCastSkill
    ↓
Activate
```

초기 보유 Skill은 Spec만 초기화하고, 모든 Logic UObject를 미리 생성하지 않습니다.

---

## 3. Grant / Revoke — Skill 소유권을 Source 단위로 추적

장비, Skill Tree, Buff처럼 여러 source가 Skill을 추가할 수 있습니다.

단순 `AddSkill / RemoveSkill`만 사용하면 서로 다른 두 source가 같은 Skill을 부여했을 때 한쪽 해제로 다른 쪽 Skill까지 사라질 수 있습니다.

이를 위해:

- `FGuid GrantId`
- `GrantsById`
- `GrantRefCounts`

를 사용합니다.

```text
Weapon A ─┐
          ├─ Skill 101 (RefCount 2)
Buff B  ──┘

Weapon A 해제
→ RefCount 1
→ Skill 유지

Buff B 해제
→ RefCount 0
→ 더 이상 허용되지 않음
```

무기 교체처럼 한 source의 skill set 전체가 바뀌는 경우 `ReplaceGrant`로 같은 Grant identity를 유지하면서 교체합니다.

---

## 4. Server-authoritative Cast

Client가 `TryCastSkillById`를 호출하면 허용 Skill인지 local에서 먼저 확인하고 Server RPC를 보냅니다.

Server는 다시:

1. Skill 허용 여부
2. Instance 준비
3. `CanCastSkill`
4. Cost
5. Target/Range 등 Skill 조건

을 확인한 뒤 실제 `Activate`를 수행합니다.

성공하면 FastArray의 casting state를 변경하고 다른 machine에 cast presentation을 전파합니다.

---

## 5. Skill Phase

`UBaseSkill`은 실행 흐름을 phase로 관리합니다.

```text
Ready
  ↓ Activate
Casting
  ↓ Fire
PostCasting
  ↓ Complete / Cancel
CoolTime
  ↓
Ready
```

Skill 종류는 달라도 기본 lifecycle을 공유합니다.

---

## 6. Cost — Check와 Apply를 분리

Cost는 “사용 가능한가?”와 “실제로 차감한다”를 분리합니다.

### `CheckCosts`

상태를 변경하지 않고:

- Stamina
- Item

부족 여부를 확인합니다.

UI나 cast pre-check에서 사용할 수 있습니다.

### `ApplyCosts`

Server에서 실제 비용을 차감합니다.

Cost는 phase별로 지정할 수 있습니다.

- OnActivate
- OnFire
- OnComplete

예를 들어 탄약은 Fire 시점, 다른 자원은 Activate 시점에 소모할 수 있습니다.

### 현재 Rollback 범위

Item cost 여러 개를 순서대로 차감하다 중간 Item에서 실패하면 이미 차감한 **Item cost는 다시 AddItem으로 복원**합니다.

다만 Stamina가 먼저 차감된 뒤 이후 Item cost에서 실패하는 경우 Stamina까지 transaction 전체를 rollback하지는 않습니다.

따라서 현재 구현은 “모든 Cost의 완전한 atomic transaction”이 아니라 **Item 부분 소비에 대한 rollback을 제공하는 구조**입니다.

---

## 7. Animation이 Gameplay Timing을 결정한다

Attack timing을 `Delay(0.3f)`처럼 C++에 고정하지 않습니다.

Montage의 실제 motion에 맞춰 Notify를 배치합니다.

### One-shot Notify

- `USkillFireNotify`: 실제 Fire timing
- `USetPlayerStateNotify`: 행동 가능 상태 변경
- `UAddConditionNotify`: Invincible 등 Condition 적용
- `USkillEndNotify`: 종료 timing

### NotifyState

- `UMeleeAttackNotifyState`: 공격 판정이 살아있는 구간

Skill logic은 “어떻게 공격하는가”를 알고, Animation은 “언제 실행하는가”를 결정합니다.

---

## 8. Client Notify와 Server Authority

Animation은 각 machine에서 재생되지만 gameplay 판정은 Authority에 최종 책임이 있습니다.

`USkillFireNotify`는:

- Server owner라면 Skill `Fire()`
- Client라면 `Server_NotifySkillFire`

경로로 연결합니다.

Melee hit detection NotifyState는 Authority에서 판정을 시작/종료합니다.

Animation timeline을 gameplay trigger로 사용하면서도 Client가 최종 Damage를 직접 적용하지 않습니다.

---

## 9. Player Action State로 Cancel Window를 표현

Player는 `EPlayerState`와 `FActionRestrictions`를 사용합니다.

대표 상태:

- NORMAL
- ONLY_ROTATE
- ACTION
- CANACTION
- DIE
- GLIDING

Montage 특정 시점에 `SetPlayerStateNotify`를 배치하여:

```text
ACTION
  ↓
공격 판정
  ↓
CANACTION  ← 다음 Combo / Dodge 허용
  ↓
NORMAL     ← 이동 포함 일반 행동 복귀
```

처럼 선딜/판정/캔슬/후딜 경계를 animation asset에서 조정할 수 있습니다.

---

## 10. Melee Attack — AttackData를 Animation Window와 연결

`UMeleeAttack`은 한 클래스에서 여러 근접 공격을 처리합니다.

`FAttackData`의 HitBoxData가 정의하는 값:

- DetectionType
- MeshComponentTag
- Start / End Socket
- Radius / HalfHeight / BoxExtent
- Interpolation 여부 / Steps

Montage의 `MeleeAttackNotifyState`에는 `AttackDataIndex`를 지정합니다.

따라서 3연타라면 각 타격 window가 서로 다른 AttackData를 사용할 수 있습니다.

---

## 11. Character Mesh와 Weapon Mesh를 같은 로직으로 처리

`MeshComponentTag == NAME_None`이면 Character Mesh를 사용하고, 값이 있으면 해당 tag의 SkeletalMeshComponent를 찾습니다.

예:

- 맨손 → hand socket
- Pickaxe → WeaponMesh의 collision socket

Skill class가 특정 Weapon class를 직접 참조하지 않습니다.

---

## 12. Line / Sphere / Capsule / Box 판정

`PerformCollisionCheck`는 `EHitDetectionType`에 따라 공통 interface로 여러 shape를 처리합니다.

- Line
- Sphere Sweep
- Capsule Sweep
- Box Sweep

Attack content는 Data만 바꿔 동일 `UMeleeAttack` logic을 재사용합니다.

---

## 13. 빠른 공격의 Frame 누락 — 위치 보간 Sweep

무기 socket이 한 frame 사이에 target을 통과하면 현재 위치에서 한 번 Trace하는 방식은 hit을 놓칠 수 있습니다.

`bUseInterpolation`이 켜진 Attack은:

1. 이전 frame Start/End socket 위치 저장
2. 현재 위치와 Lerp
3. `InterpolationSteps`만큼 중간 위치 생성
4. 각 위치에서 collision check

를 수행합니다.

```text
Previous Socket ──•──•──•── Current Socket
                  ↑ 각 지점에서 Sweep
```

고속 swing에서 frame rate에 따른 판정 누락을 줄이기 위한 선택입니다.

---

## 14. 한 판정 Window에서 중복 Hit 방지

Interpolation을 사용하면 한 Actor가 여러 sweep result에 반복 등장할 수 있습니다.

두 집합을 이용해 중복을 제거합니다.

- `ProcessedActors`: 현재 ProcessHitDetection 호출 안에서 중복 제거
- `AttackCollision.HitActors`: 해당 Notify window 전체에서 이미 맞은 Actor 제거

따라서 interpolation step 수를 늘려도 같은 target에 의도치 않은 다단 damage가 발생하지 않습니다.

---

## 15. Animation Optimization과 Server Hit Detection 충돌

AnimNotify와 socket 위치를 gameplay 판정에 사용하면 **Visibility Based Animation Ticking**이 gameplay correctness에 영향을 줄 수 있습니다.

비가시 Monster의 animation/bone update가 줄어들면 Server에서 Notify나 socket position이 예상대로 갱신되지 않을 수 있습니다.

이를 막기 위해 `AAreaObject` mesh는:

```cpp
VisibilityBasedAnimTickOption =
    EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
```

를 사용합니다.

또한 Melee 판정 window 동안 source mesh에 최고 LOD를 강제하고 window 종료 시 원래 값으로 복원해 socket/bone 안정성을 확보합니다.

이 선택은 CPU 비용과 gameplay 정확성 사이의 명시적인 trade-off입니다.

---

## 16. Damage Pipeline — 공격 Context를 끝까지 전달

단순 float Damage만 넘기면:

- Element
- Knockback
- Hit Stop
- Weak Point
- VFX/SFX

정보를 피격 대상까지 전달하기 어렵습니다.

`FCustomDamageEvent`에 `FAttackData`를 담아 `AAreaObject::TakeDamage`까지 전달합니다.

Server의 Damage 흐름:

```text
Attack Hit
   ↓
FCustomDamageEvent
   ↓
IFF / Dead / Invincible / Hidden 검사
   ↓
Defence calculation
   ↓
Weak Point
   ↓
Element multiplier
   ↓
HP / Stamina damage
   ↓
Death
   ↓
Hit Stop / Knockback / Multicast Feedback
```

공격자는 Target 종류마다 별도 처리하지 않고 Damage Event를 전달하고, Target이 자신의 방어/상태 규칙을 적용합니다.

---

## 17. Resource Object도 Damage Entry를 재사용

전투 대상이 아닌 Resource도 `TakeDamage` entry를 재정의해 채집 hit을 처리합니다.

즉 “도끼로 나무를 친다”를 별도 Harvest 입력 pipeline으로 만들기보다 기존 Attack → Damage 흐름에서 Target의 response만 다르게 만듭니다.

자세한 내용은 [[World Interaction Systems|12_World_Interaction_Systems]]에서 설명합니다.

---

## Trade-offs

### Animation에 Gameplay Timing을 맡기면 Server Animation Tick 비용이 올라간다
Notify 정확성을 위해 off-screen animation도 필요한 범위에서 갱신해야 합니다.

### AttackData 자유도가 높을수록 Validation 필요성이 커진다
잘못된 socket/tag/index는 runtime 오류로 이어질 수 있어 content validation을 더 강화할 여지가 있습니다.

### Cost transaction은 아직 완전 원자적이지 않다
Item 부분 rollback은 있지만 Stamina + Item을 포함한 전체 transaction rollback은 추가 개선 대상입니다.

---

## 관련 코드

- [SonheimSkillComponent](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Skill/SonheimSkillComponent.h)
- [BaseSkill](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Skill/Base/BaseSkill.cpp)
- [MeleeAttack](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Skill/Common/MeleeAttack.cpp)
- [Animation Notify](https://github.com/chungheonLee0325/Sonheim/tree/main/Sonheim/Source/Sonheim/Animation)
- [AreaObject Damage](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Base/AreaObject.cpp)
