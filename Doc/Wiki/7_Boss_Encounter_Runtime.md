# 7. Case Study — Boss Encounter Runtime

Forgotten Ruins의 Guardian은 기존 일반 몬스터에 스킬 몇 개만 추가한 형태가 아니라, **보스 전용 Pattern Runtime과 replicated presentation state**를 갖습니다.

---

## 구성

- `ABossMonster`
- `UBossFSM`
- `UBossPatternDataAsset`
- `ABossTelegraph`
- Boss AnimInstance / Montage
- Replicated Boss Status

---

## Data-driven Pattern

각 Pattern은 다음을 데이터로 정의합니다.

- PatternId
- Montage / Section Cue
- Telegraph timing
- Strike timing / Area shape
- Projectile / Count / Spread
- Range / Weight / Cooldown
- Phase condition
- Tracking / Re-aim
- Root Motion scale
- Leap / Charge presentation

Boss code에 패턴별 if/switch를 계속 추가하기보다, 공통 실행기가 pattern data를 소비합니다.

---

## Telegraph → Strike

공격은 먼저 플레이어가 읽을 수 있는 ground telegraph를 만들고 지정된 timing에 strike를 실행합니다.

```text
Pattern Select
      ↓
Telegraph
      ↓
Track / Re-aim
      ↓
Strike
      ↓
Recovery
```

Telegraph와 실제 공격 shape가 같은 Pattern data를 사용해 시각 정보와 실제 판정이 서로 다른 값을 갖지 않도록 합니다.

---

## Pattern 사이 이동

Boss는 공격만 연속 실행하지 않습니다.

Pattern 사이에:

- Target approach
- Back-off
- Strafe / Hop
- Re-target

을 수행해 Arena 안에서 위치 관계를 다시 만듭니다.

---

## Phase 2

HP threshold에서 Phase 2로 전환합니다.

- Roar
- Pattern tempo 증가
- 일부 strike count 증가
- Rage Niagara / Overlay
- HUD Phase 표시

Phase 전환은 Boss gameplay state와 presentation state 양쪽에서 같은 기준을 공유합니다.

---

## Break / Down / Exhaust / Capture

Boss는 단순 HP race가 아니라 여러 상태를 갖습니다.

- 누적 damage → Down
- Down 동안 free-hit window
- 특정 low-health threshold → Exhaust
- Exhaust 상태에서만 Capture 가능

일반 Pal Capture pipeline을 재사용하되, Boss가 capture 가능한 상태인지는 Boss Runtime이 결정합니다.

---

## Animation Section

Wake / Roar / Pattern / Down Montage는 section을 활용합니다.

- Sleep → Wake → Roar
- Pattern charge/release
- Fall → Down → GetUp

특히 loop section은 상태가 끝날 때 section link를 따라 다음 구간으로 이동하도록 관리합니다.

---

## 관련 코드

- [BossMonster](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Monster/Boss/BossMonster.h)
- [BossFSM](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Monster/Boss/BossFSM.h)
- [BossPatternDataAsset](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Monster/Boss/BossPatternDataAsset.h)
- [BossTelegraph](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Monster/Boss/BossTelegraph.h)
