# 9. Pal Capture & Partner Lifecycle

Pal 시스템은 **포획 판정 → 연출 → 보관 → 소환 → Partner AI**까지 이어지는 lifecycle로 구성됩니다.

---

## Capture Authority

포획 성공 여부는 서버가 계산합니다.

Client는 조준 중 capture 정보를 표시하고 요청을 보낼 수 있지만 결과를 직접 확정하지 않습니다.

---

## Reveal와 실제 Outcome 분리

`UPalCaptureComponent`는 capture reveal용 파라미터를 multicast해 참가자가 같은 연출을 보고, 실제 outcome 적용은 서버가 수행합니다.

```text
Client Aim / Throw
      ↓
Server Capture Validation
      ↓
Server Outcome
      ├─ Multicast Reveal
      └─ Authoritative State Mutation
```

Presentation timing과 authoritative result를 분리한 구조입니다.

---

## Capture Reveal

Reveal packet은 다음 값을 가집니다.

- initial guess
- segment count
- segment time
- start/inter-stage/end delay
- fail stage
- final success

클라이언트는 이 값으로 capture UI 연출을 재생합니다.

---

## Pal Inventory / Ownership

성공한 Pal은 Pal inventory와 ownership flow로 들어갑니다.

보유 한도 등 서버 조건을 통과하지 못하면 해당 player에게 Notice를 표시합니다.

---

## Partner AI

소환된 Pal은 야생 Monster와 같은 기반을 재사용하지만 Partner용 FSM branch에서 다른 행동을 수행합니다.

- Owner follow
- Target cooperation
- Partner Skill
- Resource work 일부

Wild/Partner 행동을 하나의 거대한 조건문으로 섞기보다 state branch를 분리했습니다.

---

## Dungeon / Boss 연계

Guardian Boss도 별도 capture 시스템을 만들지 않습니다.

Boss Runtime이 “현재 capture 가능한가?”를 결정하고 실제 capture는 기존 Pal Capture pipeline을 사용합니다.

기존 gameplay system이 새 vertical slice에서 재사용된 사례입니다.

---

## 관련 코드

- [PalCaptureComponent](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Player/Utility/PalCaptureComponent.h)
- [PalInventoryComponent](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Player/Utility/PalInventoryComponent.h)
- [PartnerSkillComponent](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Player/Utility/PalPartnerSkillComponent.h)
- [Monster AI](https://github.com/chungheonLee0325/Sonheim/tree/main/Sonheim/Source/Sonheim/AreaObject/Monster/AI)
