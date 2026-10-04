# 10. Boss Encounter Runtime

> **핵심 구현 범위**
>
> “일반 Monster Skill 몇 개를 순서대로 호출하는 수준을 넘어, Telegraph·Pattern·Phase·Down·Capture Window를 가진 Boss를 어떻게 데이터와 Runtime으로 구성했는가?”

Guardian은 Dungeon Boss Stage의 gameplay runtime입니다.

전체 모델:

```text
UBossPatternDataAsset
   ↓
UBossFSM
   ↓
Pattern Select
   ↓
Telegraph / Track
   ↓
Strike
   ↓
Recovery / Move
   ↓
Phase / Down / Exhaust
   ↓
Replicated Boss Status
```

---

## Part 1. Pattern은 실제로 어떤 데이터인가

### 1.1 한 번의 Strike

실제 구조 일부:

```cpp
USTRUCT(BlueprintType)
struct FBossStrike
{
    GENERATED_BODY()

    float MarkSeconds = 0.f;
    float StrikeSeconds = 1.f;

    int32 MinPhase = 1;
    bool bReaim = false;

    EBossAreaShape Shape = EBossAreaShape::Circle;
    EBossAreaAnchor Anchor = EBossAreaAnchor::Boss;

    float Radius = 400.f;
    float InnerRadius = 0.f;
    float HalfAngle = 45.f;
    float HalfWidth = 100.f;
    float ForwardOffset = 0.f;

    FAttackData Attack;

    TSubclassOf<ABaseElement> Projectile;

    int32 Count = 1;
    float Scatter = 0.f;
    float SpreadDegrees = 0.f;
};
```

한 Strike가:

- 언제 warning을 보여줄지
- 언제 실제 hit이 발생할지
- 어떤 shape인지
- 어디를 anchor로 할지
- Damage context
- projectile 여부

를 함께 정의합니다.

---

### 1.2 Pattern

```cpp
USTRUCT(BlueprintType)
struct FBossPattern
{
    GENERATED_BODY()

    FGameplayTag PatternId;
    TObjectPtr<UAnimMontage> Montage;

    TArray<FBossSectionCue> Cues;
    TArray<FBossStrike> Strikes;

    float Seconds = 3.f;
    float TrackSeconds = 0.5f;
    float TurnDegreesPerSecond = 300.f;

    float MinRange = 0.f;
    float MaxRange = 1500.f;
    float Weight = 1.f;
    float Cooldown = 0.f;
    int32 MinPhase = 1;

    float RootMotionScale = 1.f;

    float LeapStartSeconds = 0.f;
    float LeapEndSeconds = 0.f;
    float LeapHeight = 300.f;
};
```

Boss code는 Pattern마다 별도 함수 이름을 hardcode하기보다 이 데이터를 공통 executor가 해석합니다.

---

## Part 2. Pattern 선택

Boss는 현재 Target distance, Phase, Cooldown과 Weight를 보고 candidate를 고릅니다.

```text
현재 Phase
 + Target Range
 + Pattern Cooldown
       ↓
Candidate Patterns
       ↓ Weight
Selected Pattern
```

Pattern을 “애니메이션 하나”가 아니라 **선택 조건 + timing + attack data를 가진 행동 단위**로 봅니다.

---

## Part 3. Telegraph와 실제 Strike는 같은 데이터를 사용한다

Telegraph가 Circle인데 실제 Damage가 다른 Radius라면 player가 화면을 믿을 수 없습니다.

그래서 같은 `FBossStrike`의:

- Shape
- Radius
- Angle
- Width
- Anchor

를 Telegraph와 실제 Hit 모두에서 사용합니다.

```text
FBossStrike
  ├─ ABossTelegraph
  └─ Actual Strike
```

presentation과 gameplay의 geometry source를 하나로 둡니다.

---

## Part 4. Re-aim / Tracking

모든 공격을 Pattern 시작 순간에 완전히 lock하면 moving target에 지나치게 쉽게 빗나갑니다.

반대로 strike 직전까지 무한 tracking하면 Telegraph를 보고 피할 수 없습니다.

그래서:

- Pattern 전체 tracking 시간
- Strike 사이의 `bReaim`

을 데이터로 조절합니다.

이렇게 “언제까지 Boss가 Player를 따라보는가”도 Pattern tuning 값이 됩니다.

---

## Part 5. Montage Section Cue

Pattern은 Montage를 처음부터 끝까지 수동 Timer로만 다루지 않습니다.

```cpp
USTRUCT(BlueprintType)
struct FBossSectionCue
{
    GENERATED_BODY()

    float Seconds = 0.f;
    FName Section;
};
```

Charge loop → Release 같은 Montage section 전환을 Pattern timeline에 넣습니다.

Wake/Down 같은 공용 Boss Montage도:

- Sleep
- Wake
- Roar
- Fall
- Down
- GetUp
- Land

section convention을 사용합니다.

---

## Part 6. Phase 2

`UBossPatternDataAsset`에는 전체 Boss fight tuning도 있습니다.

대표 필드:

```cpp
float PhaseTwoHealth = 0.6f;
float RoarSeconds = 1.8f;
float PhaseTwoTempo = 1.2f;
int32 PhaseTwoExtraCount = 2;
```

HP threshold를 넘으면:

- Roar
- Pattern tempo 증가
- multi strike/projectile count 증가
- Rage VFX / Overlay
- HUD Phase

가 같은 phase state를 기준으로 전환됩니다.

---

## Part 7. Down과 Exhaust는 다른 상태다

Boss의 “공격 불가”를 하나의 stun bool로 처리하지 않습니다.

### Down
누적 break damage로 발생하는 knockdown window.

### Exhaust
특정 낮은 HP threshold에서 발생하며 **Capture 가능 상태**가 됩니다.

Boss Runtime이:

```text
지금 Capture 가능한가?
```

를 결정하고, 실제 ownership 변경은 기존 Pal Capture pipeline에 맡깁니다.

---

## Part 8. 기존 Combat Data를 재사용한다

`FBossStrike` 안에는 별도 BossDamage 구조체가 아니라:

```cpp
FAttackData Attack;
```

이 들어갑니다.

따라서 Boss도 일반 Combat과 같은:

- Element
- Hit Stop
- Knockback
- Damage Feedback

context를 사용할 수 있습니다.

Boss 전용 Runtime은 “언제/어디서 공격하는가”를 확장하고, Damage semantics 자체는 기존 전투 기반을 사용합니다.

---

## Part 9. UI에 필요한 Boss 상태만 Snapshot으로 보낸다

Dungeon Snapshot에는 대표적으로:

- BossHealth
- BossActionId
- BossActionStart/End Server Time
- BossPhase
- bBossVulnerable
- BossBreak

가 들어갑니다.

Widget이 `UBossFSM` 자체를 참조하지 않습니다.

---

## Trade-offs

### Data tuning 폭이 넓다
Pattern authoring 실수 가능성이 높아 validation이 중요합니다.

### Telegraph와 실제 판정이 Server authoritative다
Network delay가 있어도 gameplay 결과 기준은 Server이며, Client presentation은 replicated/status time을 따라갑니다.

### Boss 전용 Runtime이 추가된다
일반 Monster Skill system만으로 모든 Boss를 해결하지 않는 대신 Boss-specific pacing/phase 상태를 명시적으로 표현합니다.

---

## 연관 문서

- 기반 Combat 구조 → [[5. Combat, Skill & Animation|5_Combat_Skill_Animation]]
- Exhaust Capture가 연결되는 방식 → [[8. Pal Capture & Partner Lifecycle|8_Pal_Capture_Partner_Lifecycle]]
- Boss 상태가 HUD로 가는 방식 → [[11. Client State & Presentation Pipeline|11_Client_State_Presentation_Pipeline]]
- Pattern validation → [[13. Content Authoring & Validation|13_Content_Authoring_Validation]]

---

## 관련 코드

- [BossPatternDataAsset.h](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Monster/Boss/BossPatternDataAsset.h)
- [BossFSM.h](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Monster/Boss/BossFSM.h)
- [BossMonster.h](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Monster/Boss/BossMonster.h)
- [BossTelegraph.h](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Monster/Boss/BossTelegraph.h)
