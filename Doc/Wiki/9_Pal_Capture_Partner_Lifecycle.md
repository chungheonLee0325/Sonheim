# 9. Pal Capture & Partner Lifecycle

> **이 문서가 답하는 질문**
>
> “야생 Monster가 Capture 대상에서 Player 소유 Pal이 되고, 보관·선택·소환·Partner AI로 이어지는 전체 수명을 어떻게 관리하는가?”

핵심은 “Capture 기능” 하나가 아니라 ownership이 바뀌는 lifecycle입니다.

```text
Wild Monster
   ↓ Capture Attempt
Reveal
   ↓
Owned Pal
   ↓
Stored / Selected
   ↓
Summoned Partner
   ↓
Partner AI
```


> **코드 표기:** 아래 코드 블록은 `main`의 실제 선언/함수에서 문서 이해에 필요한 부분을 발췌한 것입니다. `UPROPERTY` metadata나 보조 필드는 일부 생략될 수 있으며, 전체 구현은 하단 Source 링크에서 확인할 수 있습니다.
---

## Part 1. 역할 분리

| Component / Actor | 책임 |
|---|---|
| `UPalCaptureComponent` | 포획률, Server 판정, Reveal |
| `UPalInventoryComponent` | Owned Pal 목록, 선택 slot |
| `UPalPartnerSkillComponent` | 소환/회수 |
| `ABaseMonster` | PartnerOwner, Active/Inactive, AI/IFF |

Player 하나에 모든 Pal logic을 넣지 않습니다.

---

## Part 2. Capture Rate

실제 계산은 Monster 종 데이터와 HP를 사용합니다.

핵심 코드:

```cpp
float UPalCaptureComponent::CalculateCaptureRate(
    ABaseMonster* TargetPal) const
{
    if (!TargetPal) return 0.f;

    const FAreaObjectData& Data = TargetPal->GetAreaObjectData();
    if (!Data.bCapturable) return 0.f;

    const float hpRatio =
        TargetPal->GetHP() / TargetPal->GetMaxHP();

    float rate =
        (hpRatio <= Data.CaptureLowHPThreshold)
        ? 1.f
        : 1.f - (hpRatio - Data.CaptureLowHPThreshold)
              * ((1.f - Data.CaptureBase)
              / (1.f - Data.CaptureLowHPThreshold));

    const float resist =
        FMath::Clamp(Data.CaptureResist, 0.f, 0.95f);

    rate *= (1.f - resist);

    return FMath::Clamp(rate, 0.f, 1.f);
}
```

Client는 예상 확률을 UI에 보여줄 수 있지만 성공 random roll은 Server가 수행합니다.

---

## Part 3. Capture 진행 중 대상 격리

Capture 도중 같은 Monster에 다른 Player가 다시 상호작용하면 ownership race가 생길 수 있습니다.

그래서 시도 시작 시 Target을 `DeactivateMonster()`로 gameplay에서 격리합니다.

Deactivate 시:

- FSM Stop
- Current Skill 정리
- Hidden Condition
- Render Off
- Collision Off
- HP Widget Off
- AI Tick Off

실패하면 `ActivateMonster()`로 되돌립니다.

---

## Part 4. 결과 판정과 실제 적용 시점을 분리

Server는 결과를 먼저 정하지만 즉시 ownership을 바꾸지 않습니다.

```text
Server_AttemptCapture
  ↓
Success / Fail 결정
  ↓
Reveal Params 생성
  ↓ Multicast
Capture Reveal
  ↓
Reveal duration
  ↓
Server_ApplyCaptureOutcome
```

화면에는 아직 capture animation이 진행 중인데 gameplay에서는 이미 Partner가 된 상태를 피합니다.

---

## Part 5. Reveal도 Server가 같은 parameter를 배포한다

Server가 구성하는 값:

- Guess
- Segment Count
- Segment Time
- Delay
- Fail Stage
- Success bool

각 Client가 별도 random 연출을 만들지 않습니다.

따라서 같은 Capture Attempt를 보는 Player들이 서로 다른 reveal progression을 계산하지 않습니다.

---

## Part 6. Outcome 시 다시 검증한다

Reveal 동안 Pal Inventory가 가득 찰 수 있습니다.

그래서 결과 적용 시점에 다시 capacity를 확인합니다.

성공 시:

```text
SetPartnerOwner(Player)
 → PalInventory.AddPal(Target)
```

공간이 없어졌다면 성공을 강행하지 않고 실패 경로로 되돌립니다.

---

## Part 7. Pal Inventory는 왜 FastArray가 아닌가

Item Inventory는 슬롯이 많고 변경도 빈번하지만 Pal 목록은 작습니다.

`OwnedPals`는 일반 replicated array를 사용하고 `OnRep_OwnedPals`에서 이전 상태와 비교합니다.

개념적으로:

```cpp
OldPal == NewPal
    → no event

OldPal != nullptr && NewPal == nullptr
    → OnPalRemoved

NewPal != nullptr && OldPal != NewPal
    → OnPalAdded
```

초기 전체 sync와 이후 diff도 구분합니다.

작은 collection에 FastArray 복잡도를 무조건 적용하지 않은 선택입니다.

---

## Part 8. 선택 Slot은 Prediction

Pal slot 변경은 즉각적인 HUD 반응이 중요합니다.

Client:

```text
입력
 → 예상 Index 계산
 → OnSelectedPalChanged
 → Server RPC
```

Server:

```text
CurrentPalIndex 확정
 → Owner replication
 → OnRep_CurrentPalIndex
```

Inventory swap과 같은 prediction/reconciliation 원리입니다.

---

## Part 9. Summon은 Animation 종료와 실제 상태를 맞춘다

소환 입력 즉시 Pal을 활성화한 뒤 Animation을 재생하지 않습니다.

```text
Summon Request
 → Multicast Montage
 → Montage End
 → Authority: ProcessPendingSummon
 → Activate / Deactivate
```

presentation과 gameplay state transition의 시점을 맞춥니다.

---

## Part 10. Actor를 Destroy/Spawn하지 않고 상태를 전환한다

Owned Pal은 actor identity를 유지한 채 world participation을 끕니다.

### Deactivate
- AI Stop
- Hidden
- Collision Off
- UI Off

### Activate
- Hidden 해제
- FSM SelectMode
- Collision/Render/UI/AI 복구
- Owner 근처 위치 보정

---

## Part 11. Partner AI

같은 `ABaseMonster`가 `PartnerOwner` 여부에 따라 다른 behavior route를 탑니다.

Partner patrol은:

- 너무 멀면 teleport
- 일정 거리면 follow
- Aggro Target 있으면 combat
- 평소에는 Owner 주변 patrol

을 수행합니다.

---

## Part 12. IFF

Partner가 되면 단순 team enum 하나만 보는 것이 아니라 `PartnerOwner` 관계를 이용해 공격 가능 여부를 판단합니다.

예:

- Partner → Wild Monster: 공격 가능
- Wild Monster → Partner: 공격 가능
- Partner → 다른 Owned Partner: 제한

이 규칙은 `CanAttack` 계층에서 적용됩니다.

---

## Part 13. Boss Capture

Guardian은 항상 Capturable하지 않습니다.

Boss Runtime이 Exhaust 등 특정 state에서만 `bBossVulnerable`을 활성화하고, 실제 Capture result/ownership pipeline은 동일한 `UPalCaptureComponent`를 사용합니다.

Boss를 위해 별도 Capture ownership model을 만들지 않습니다.

---

## 이 문서 다음에 읽기

- 일반 Combat 기반 → [[10. Combat, Skill & Animation|10_Combat_Skill_Animation]]
- Player data/lifecycle → [[11. Player & Character Systems|11_Player_Character_Systems]]
- Boss Exhaust/Capture → [[7. Boss Encounter Runtime|7_Boss_Encounter_Runtime]]

---

## 관련 코드

- [PalCaptureComponent.cpp](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Player/Utility/PalCaptureComponent.cpp)
- [PalInventoryComponent.cpp](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Player/Utility/PalInventoryComponent.cpp)
- [PalPartnerSkillComponent](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Player/Utility/PalPartnerSkillComponent.h)
- [BaseMonster.cpp](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Monster/BaseMonster.cpp)
