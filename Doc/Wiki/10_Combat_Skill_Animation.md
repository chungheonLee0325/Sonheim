# 10. Combat, Skill & Animation

기존 Skill / Animation / Combat / Melee Attack 문서를 하나의 **실행 파이프라인**으로 통합합니다.

---

## 전체 흐름

```text
Skill Data / Spec
      ↓
USonheimSkillComponent
      ↓
Server Validation
      ↓
Skill Instance
      ↓
Animation / Montage
      ↓ AnimNotify
Attack Execution
      ↓
Damage Context
      ↓
Health / Condition / Feedback
```

---

## Skill State

`USonheimSkillComponent`는 Fast Array 기반 skill spec을 복제합니다.

- default skills
- Grant / Revoke
- reference-counted grant source
- atomic grant replacement
- active / cooldown state
- server cast validation

장비나 외부 시스템이 skill을 부여할 때 “현재 스킬 목록을 직접 수정”하기보다 Grant 단위를 관리합니다.

---

## Data / State / Logic 분리

스킬의 수치와 참조 데이터, 현재 activation/cooldown 상태, 실제 실행 로직을 한 객체에 모두 넣지 않습니다.

이 분리는 replication에서 UObject instance 자체보다 필요한 lightweight state만 전달하기에도 유리합니다.

---

## Animation-driven Timing

공격 판정 시점을 임의의 코드 delay보다 AnimNotify / NotifyState로 연결합니다.

대표 예:

- SkillFire
- SkillEnd
- MeleeAttack window
- Combo window
- RotateToTarget
- AddMovement
- CameraShake

애니메이션과 실제 공격 타이밍을 같은 timeline에서 조정할 수 있습니다.

---

## Attack

공격 실행기는 melee, projectile, shotgun 등 서로 다른 방식을 지원합니다.

Hit shape도 Line / Sphere / Capsule / Box 등 목적에 따라 사용합니다.

Attack data가 damage와 속성 context를 전달하고 실제 대상의 `TakeDamage` pipeline으로 이어집니다.

---

## Feedback

Combat 결과는 HP 변화로 끝나지 않습니다.

- hit reaction
- camera shake
- floating damage
- VFX
- element feedback

등 presentation이 필요한 context를 전달합니다.

---

## 관련 코드

- [Skill](https://github.com/chungheonLee0325/Sonheim/tree/main/Sonheim/Source/Sonheim/AreaObject/Skill)
- [Animation](https://github.com/chungheonLee0325/Sonheim/tree/main/Sonheim/Source/Sonheim/Animation)
- [AreaObject](https://github.com/chungheonLee0325/Sonheim/tree/main/Sonheim/Source/Sonheim/AreaObject)
