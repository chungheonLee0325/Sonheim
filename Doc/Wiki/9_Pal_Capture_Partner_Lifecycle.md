# 9. Pal Capture & Partner Lifecycle

> **야생 Monster 하나가 Capture 대상에서 Player 소유 Pal이 되고, 보관·선택·소환·Partner AI로 이어지는 전체 lifecycle을 세 개의 Player Component와 Monster state가 협력해 관리합니다.**

주요 책임은 다음과 같이 나뉩니다.

- `UPalCaptureComponent`: 조준, 포획률, 서버 판정, reveal
- `UPalInventoryComponent`: 소유 Pal 목록과 선택
- `UPalPartnerSkillComponent`: 소환/회수와 Partner action
- `ABaseMonster`: ownership, active/inactive state, AI/IFF

---

## 1. Capture Rate — HP와 종별 저항을 데이터로 계산

포획률은 Monster의 현재 HP와 종 데이터에 따라 계산합니다.

`FAreaObjectData`의:

- `bCapturable`
- `CaptureBase`
- `CaptureLowHPThreshold`
- `CaptureResist`

를 사용합니다.

HP가 `CaptureLowHPThreshold` 아래로 내려가면 base curve가 최대치에 가까워지고, 마지막에 종별 resistance를 곱해 최종 확률을 만듭니다.

Client는 조준 중 같은 계산을 이용해 예상 Capture Rate를 UI에 보여주지만, **성공 여부의 random roll은 Server가 결정**합니다.

---

## 2. Capture 시도 중 Target을 Gameplay에서 격리

같은 Monster에 여러 Capture가 겹치면 ownership 경쟁이 생길 수 있습니다.

Capture attempt가 시작되면 Target을 `DeactivateMonster()`로 전환합니다.

`DeactivateMonster`는:

- FSM 정지
- 현재 Skill 정리
- `Hidden` Condition 추가
- 렌더링 숨김
- Collision 비활성
- HP Widget 비활성
- AI Controller Tick 비활성

을 수행합니다.

별도의 “CapturingOwner” lock만 두는 대신 **대상 자체를 gameplay에 참여하지 않는 상태로 전환**해 다른 hit/targeting 경로에서도 자연스럽게 제외합니다.

Capture 실패 시 `ActivateMonster()`로 되돌리고 시도한 Player를 Aggro Target으로 지정합니다.

---

## 3. 결과 판정과 연출 종료를 분리

Server는 Capture 요청을 받으면 결과를 먼저 판정하지만, ownership을 즉시 적용하지 않습니다.

```text
Server_AttemptCapture
    ↓
Capture Rate + Random Roll
    ↓
Reveal Parameter 생성
    ↓ Multicast
Client Reveal Animation
    ↓
Reveal duration 경과
    ↓
Server_ApplyCaptureOutcome
```

이를 통해 화면에는 Capture 연출이 진행 중인데 이미 Monster ownership이 바뀌는 어색한 상태를 피합니다.

---

## 4. 확률을 Reveal 구조에도 사용

성공/실패 bool만 보내지 않고 Server가 `FPalCaptureRevealParams`를 구성합니다.

- Guess
- Segment count
- Segment duration
- Start / Inter-stage / End delay
- 실패할 Segment
- 최종 성공 여부

Capture Rate가 높을수록 적은 segment로 빠르게 결과에 도달하도록 하고, 실패의 경우 `PickFailStage`가 어디까지 진행 후 실패할지 정합니다.

모든 Client가 같은 parameter를 받기 때문에 각자 random reveal을 새로 계산하지 않습니다.

---

## 5. Outcome 적용 시 조건을 다시 확인

Reveal이 재생되는 동안 다른 gameplay state가 변할 수 있습니다.

예를 들어 시작 시에는 Pal Inventory 공간이 있었지만 연출 중 다른 Pal이 추가될 수 있습니다.

따라서 `Server_ApplyCaptureOutcome`에서 **MaxPalCount를 다시 확인**합니다.

성공 가능한 경우:

1. `TargetPal->SetPartnerOwner(OwnerPlayer)`
2. `PalInventory->AddPal(TargetPal)`

공간이 사라졌다면 성공 판정을 그대로 강행하지 않고 실패로 처리하고 Notice를 보냅니다.

---

## 6. Pal Inventory — 작은 배열에는 단순한 Replication + Diff

Item Inventory는 수십 슬롯이므로 FastArray를 사용하지만 Pal 목록은 최대 개수가 작습니다.

따라서 `OwnedPals`는 일반 replicated array를 유지하고 `OnRep_OwnedPals`에서 이전 목록 `PrevOwnedPals`와 비교합니다.

```text
OldPal == NewPal → no event
OldPal only      → OnPalRemoved
NewPal only      → OnPalAdded
```

초기 sync와 이후 변경을 구분해 최초 UI 구성에서 불필요한 획득 animation도 억제합니다.

복잡한 replication primitive를 무조건 사용하는 대신 데이터 규모에 맞춘 선택입니다.

---

## 7. Pal Slot 전환 — Client Prediction

선택 Pal index는 입력 반응성이 중요합니다.

Client는 방향 입력을 받으면 예상 index를 즉시 `OnSelectedPalChanged`로 UI에 보여주고 Server RPC를 전송합니다.

Server가 확정한 `CurrentPalIndex`가 owner에게 복제되며 `OnRep_CurrentPalIndex`가 최종 상태를 다시 알립니다.

Inventory slot swap과 같은 **prediction → request → replication** 패턴입니다.

---

## 8. 소환은 Animation과 Gameplay 적용 시점을 맞춘다

Pal summon/release는 Multicast로 summon Montage를 재생하고, Montage 종료 시점에 Server만 실제 `ProcessPendingSummon`을 실행합니다.

즉:

```text
Summon Request
   ↓
Multicast Montage
   ↓
Montage End
   ↓ Authority only
Activate / Deactivate Pal
```

Gameplay 결과를 animation 시작 순간에 먼저 적용하지 않아 “연출 전에 Pal이 나타나는” 문제를 줄입니다.

---

## 9. Destroy / Spawn 대신 Activate / Deactivate

보관한 Pal을 매번 Destroy/Spawn하지 않고 같은 Actor를 비활성화/재활성화합니다.

### Deactivate
- FSM Stop
- Hidden Condition
- Hidden In Game
- Collision Off
- UI Off
- AI Tick Off

### Activate
- Hidden Condition 제거
- FSM을 SelectMode로 복귀
- Collision / Render / UI / AI 활성화
- PartnerOwner가 있으면 Owner 근처로 이동

Actor identity를 유지하면서 World participation만 끄는 방식입니다.

---

## 10. Partner AI와 IFF

Partner가 되었다고 별도의 Monster class로 교체하지 않습니다.

`PartnerOwner` 유무를 기준으로 FSM의 Partner Patrol 경로가:

- Owner와 거리가 매우 멀면 teleport
- 일정 거리 밖이면 follow
- Aggro Target이 있으면 combat state로 전환
- 평시에는 Owner 주변 patrol

을 수행합니다.

`CanAttack`은 ownership 관계를 확인하여:

- Wild ↔ Partner
- Partner ↔ Wild
- Partner ↔ 다른 Partner

사이의 피아식별 규칙을 적용합니다.

---

## 11. Guardian Boss도 같은 Capture Pipeline을 사용

Guardian은 평상시 Capturable 대상이 아닙니다.

Boss Runtime이 Exhaust 등 특정 vulnerable state에서만 Capture를 허용하고, 실제 Capture 결과 적용은 같은 `UPalCaptureComponent` pipeline을 사용합니다.

Boss를 위해 별도 ownership/capture system을 하나 더 만들지 않고 기존 lifecycle에 조건을 추가했습니다.

---

## 관련 코드

- [PalCaptureComponent](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Player/Utility/PalCaptureComponent.h)
- [PalInventoryComponent](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Player/Utility/PalInventoryComponent.h)
- [PalPartnerSkillComponent](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Player/Utility/PalPartnerSkillComponent.h)
- [BaseMonster](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Monster/BaseMonster.cpp)
- [Partner AI](https://github.com/chungheonLee0325/Sonheim/tree/main/Sonheim/Source/Sonheim/AreaObject/Monster/AI/Derived/AiMonster/BasePartnerAiState)
