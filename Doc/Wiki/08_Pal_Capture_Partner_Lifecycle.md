# 08. Pal Capture & Partner Lifecycle

Pal 시스템은 **Wild Monster → Capture → Ownership → Storage → Selection → Summon → Partner AI**로 이어지는 lifecycle을 관리합니다.

- <code>UPalCaptureComponent</code> — 포획 판정과 Reveal
- <code>UPalInventoryComponent</code> — 소유 Pal 목록과 선택 Slot
- <code>UPalPartnerSkillComponent</code> — 소환/회수와 Partner action
- <code>ABaseMonster</code> — ownership, active state, AI/IFF

---

## 전체 Lifecycle

~~~mermaid
flowchart LR
    WILD["<b>Wild Monster</b>"]
    TRY["<b>Capture Attempt</b>"]
    REVEAL["<b>Capture Reveal</b>"]
    OWNED["<b>Owned Pal</b>"]
    STORED["<b>Stored / Selected</b>"]
    SUMMON["<b>Summon</b>"]
    PARTNER["<b>Partner AI</b>"]
    COMBAT["<b>Combat</b>"]

    WILD --> TRY
    TRY --> REVEAL
    REVEAL -->|"Success"| OWNED
    REVEAL -->|"Fail"| WILD
    OWNED --> STORED
    STORED --> SUMMON
    SUMMON --> PARTNER
    PARTNER --> COMBAT
    COMBAT -->|"Recall"| STORED
~~~

주요 책임:

| 시스템 | 책임 |
|---|---|
| \`UPalCaptureComponent\` | 조준, 포획률, 결과 판정, Reveal |
| \`UPalInventoryComponent\` | 소유 Pal 목록과 선택 Slot |
| \`UPalPartnerSkillComponent\` | 소환/회수, Partner Skill |
| \`ABaseMonster\` | Ownership, Active/Inactive, AI/IFF |

---

## 시연 영상

기존 Pal Capture 시연 영상입니다. Capture 시도부터 Reveal과 성공/실패 연출까지 실제 사용자 흐름을 확인할 수 있습니다.

https://github.com/user-attachments/assets/57246d79-bd3b-473f-85fc-762670023729

---

## 1. Capture 가능 여부와 확률을 분리

먼저 Monster가 현재 Capture 대상이 될 수 있는지 검사합니다.

일반 Monster의 조건:

~~~cpp
bool ABaseMonster::CanCapture() const
{
    return !IsDie()
        && !PartnerOwner
        && !HasCondition(EConditionBitsType::Hidden);
}
~~~

Boss는 이 조건을 확장해 **Resting / Exhaust 상태일 때만** Capture 가능하도록 제한합니다.

실제 성공 확률은 Monster 종 데이터와 현재 HP를 사용합니다.

~~~text
CaptureBase
 + Current HP Ratio
 + Low HP Threshold
 + Capture Resist
        ↓
Final Capture Rate
~~~

HP가 낮아질수록 성공률이 높아지고, 종별 resistance를 마지막에 적용합니다.

Client는 같은 계산으로 예상 확률을 화면에 보여줄 수 있지만 **성공 random roll은 Server가 수행**합니다.

---

## 2. Capture 시도 중 Monster를 Gameplay에서 격리

Sphere가 Target에 닿고 Capture가 시작되면 Monster를 즉시 일반 전투 상태에 그대로 두지 않습니다.

\`DeactivateMonster()\`:

~~~text
FSM Stop
Skill 정리
Hidden Condition
Rendering Off
Collision Off
HP Widget Off
AI Tick Off
~~~

Capture 연출 중 같은 Target이:

- 다시 공격받거나
- 다른 Targeting에 잡히거나
- AI 행동을 계속하거나
- 중복 Capture 대상이 되는

경로를 줄입니다.

별도 “Capturing bool”만 추가하기보다 **Monster 자체를 현재 World gameplay에서 빠진 상태로 전환**합니다.

---

## 3. 결과 판정과 Ownership 적용 시점을 분리

Capture 결과를 판정하자마자 Monster를 Partner로 바꾸지 않습니다.

~~~mermaid
sequenceDiagram
    participant S as Server
    participant C as Clients
    participant M as Monster

    S->>S: Capture Rate / Random Roll
    S->>S: Reveal Params 생성
    S->>C: Multicast Capture Reveal
    C->>C: Segment Animation
    S->>S: Reveal 시간 대기
    S->>M: Apply Capture Outcome
~~~

화면에서는 아직 Capture 연출 중인데 gameplay에서는 이미 Ownership이 바뀐 상태를 피하기 위한 처리입니다.

---

## 4. Reveal도 각 Client가 따로 Randomize하지 않는다

Server가 \`FPalCaptureRevealParams\`를 만들어 모든 Client에 전달합니다.

대표 정보:

- 예상 Capture Rate
- Segment Count
- Segment Duration
- Start / Inter-stage / End Delay
- 실패 Segment
- 최종 성공 여부

Capture Rate가 높을수록 더 적은 Segment로 결과에 도달할 수 있고, 실패 시 어느 단계까지 진행한 뒤 실패할지도 Server가 정합니다.

같은 Capture를 보는 Client마다 다른 연출 결과를 계산하지 않습니다.

---

## 5. Outcome 적용 시 다시 현재 상태를 검증

Capture 시작 시 Inventory에 자리가 있었더라도 Reveal이 진행되는 동안 다른 Pal이 들어올 수 있습니다.

그래서 실제 결과 적용 시점에 다시 \`MaxPalCount\`를 확인합니다.

~~~cpp
if (bSuccess)
{
    if (PalInventory->GetOwnedPalCount() <
        PalInventory->MaxPalCount)
    {
        TargetPal->SetPartnerOwner(OwnerPlayer);
        PalInventory->AddPal(TargetPal);
    }
    else
    {
        bSuccess = false;
        NotifyPartyFull();
    }
}
~~~

성공 판정이 났다는 이유만으로 **몇 초 전의 조건을 신뢰하지 않고 state mutation 직전에 다시 확인**합니다.

실패하면 Monster를 다시 활성화하고 Capture를 시도한 Player를 Aggro Target으로 설정합니다.

---

## 6. Ownership 변경과 보관을 분리

Capture 성공 시 두 변화가 일어납니다.

~~~text
Monster
 └─ PartnerOwner = Player

PlayerState
 └─ PalInventory.AddPal(Monster)
~~~

Monster는 자신이 누구에게 소유됐는지 알고, Player 쪽은 어떤 Pal을 보유하고 선택할 수 있는지 관리합니다.

Ownership과 selection state를 한 Actor 안에 섞지 않습니다.

---

## 7. Pal 목록은 작은 배열에 맞는 단순 Replication을 사용

Item Inventory는 slot 수와 변경 빈도가 높아 FastArray를 사용하지만, Pal 목록은 최대 개수가 작은 collection입니다.

\`OwnedPals\`는 일반 replicated array를 사용하고, \`OnRep_OwnedPals()\`에서 이전 목록과 비교합니다.

~~~text
Old == New
→ 변화 없음

Old만 존재
→ OnPalRemoved

New가 추가/교체
→ OnPalAdded
~~~

초기 전체 sync와 이후 변경도 구분합니다.

**같은 프로젝트 안에서도 collection 규모와 변경 패턴에 따라 replication primitive를 다르게 선택**한 사례입니다.

---

## 8. Selected Pal은 입력 반응성을 위해 먼저 표시

Pal slot을 넘길 때 HUD가 매번 Server round-trip을 기다리면 선택감이 둔해집니다.

Client는 예상 index를 먼저 계산해 \`OnSelectedPalChanged\`를 발생시키고 Server에 요청합니다.

~~~text
Slot Input
   ↓
Predicted Index
   ↓
HUD Update
   ↓
Server Request
   ↓
CurrentPalIndex Replication
   ↓
Final UI State
~~~

최종 index는 Server state로 다시 맞춥니다.

세부 동기화 원리는 [[12. Multiplayer Synchronization|12_Multiplayer_Synchronization]]에서 설명합니다.

---

## 9. Summon Animation과 실제 World 상태 전환을 맞춘다

소환 버튼을 누른 순간 Pal을 먼저 활성화하지 않습니다.

~~~text
Toggle Summon
   ↓
Server가 Pending Pal 결정
   ↓
Summon Montage Multicast
   ↓
Montage End
   ↓
Authority: ProcessPendingSummon
   ↓
Activate / Deactivate Pal
~~~

Animation이 끝난 시점에 Authority가 실제 World participation을 바꿉니다.

표현과 gameplay 결과가 서로 다른 시점에 적용되는 문제를 줄입니다.

---

## 10. Destroy / Respawn 대신 같은 Monster Actor를 활성/비활성화

보관된 Pal을 매번 Destroy하고 다시 Spawn하지 않습니다.

### Deactivate

- FSM Stop
- Current Skill 정리
- Hidden
- Collision Off
- UI Off
- AI Tick Off

### Activate

- Hidden Condition 제거
- FSM을 다시 Select Mode로 전환
- Rendering / Collision / UI / AI 복구
- PartnerOwner가 있으면 Owner 근처로 위치 보정

같은 Actor identity와 ownership을 유지한 채 **World participation만 전환**합니다.

---

## 11. 같은 Monster가 Ownership에 따라 Partner AI로 동작

Capture 성공 후 별도의 “PartnerMonster” class로 교체하지 않습니다.

\`PartnerOwner\`가 있으면 기존 Monster의 FSM이 Partner 경로를 사용합니다.

대표 행동:

~~~text
Owner와 매우 멂
→ Teleport

Owner와 일정 거리 이상
→ Follow

Valid Aggro Target 존재
→ Combat

평상시
→ Owner 주변 Patrol
~~~

Capture 전후에 class identity는 유지하고 **ownership이 behavior context를 바꾸는 구조**입니다.

---

## 12. IFF도 Ownership 관계를 기준으로 판단

\`CanAttack()\`은 Target이 Monster / Player인지와 \`PartnerOwner\` 관계를 함께 봅니다.

대표적으로:

| 공격 주체 | Target | 결과 |
|---|---|---|
| Wild Monster | Player Partner | 공격 가능 |
| Partner | Wild Monster | 공격 가능 |
| Partner | 다른 Owned Pal | 공격 제한 |
| Partner | Player | 공격 제한 |
| Wild Monster | 다른 Wild Monster | 공격 제한 |

Capture로 ownership이 바뀌면 같은 Monster의 피아식별 규칙도 함께 바뀝니다.

---

## 13. Boss Capture는 같은 Lifecycle의 입구만 제한

Guardian Boss는 평상시 Sphere가 닿아도 \`CanCapture()\`에서 차단됩니다.

~~~cpp
bool ABossMonster::CanCapture() const
{
    return Super::CanCapture()
        && Status.IsVulnerable();
}
~~~

Boss Runtime이 Exhaust/Resting 상태를 만들면 기존 Sphere → Capture Component → Reveal → Ownership 흐름을 사용할 수 있습니다.

Boss 전용 Capture ownership 시스템을 따로 만들지 않고 **기존 Capture lifecycle의 eligibility만 Boss 상태로 제한**합니다.

---

## 설계 선택과 비용

| 선택 | 얻은 것 | 비용 / 제약 |
|---|---|---|
| **Capture 중 Monster Deactivate** | AI/Collision/Targeting을 한 번에 격리 | 실패 시 원래 gameplay 상태를 정확히 복구해야 함 |
| **판정과 Outcome 적용 분리** | Reveal과 Ownership 전환 시점 일치 | 연출 중 조건 변화에 대한 재검증 필요 |
| **작은 Pal array의 단순 Replication** | FastArray 복잡도 없이 변경 추적 | 목록이 커지면 delta 효율 재검토 필요 |
| **Actor Activate / Deactivate** | Ownership / Actor identity 유지 | 비활성 상태에서 꺼야 할 subsystem을 일관되게 관리해야 함 |
| **PartnerOwner 기반 AI/IFF** | Wild/Partner를 같은 Monster class에서 재사용 | ownership 조건이 combat/AI 판단에 추가됨 |

---

## 연관 문서

- [[04. Player & Character Systems|04_Player_Character_Systems]] — PlayerState의 Pal Inventory와 Pawn 기능 연결
- [[05. Combat, Skill & Animation|05_Combat_Skill_Animation]] — Partner가 재사용하는 Monster combat 기반
- [[10. Boss Encounter Runtime|10_Boss_Encounter_Runtime]] — Guardian Resting / Capture Window
- [[12. Multiplayer Synchronization|12_Multiplayer_Synchronization]] — Reveal / Owner state / slot prediction 동기화

---

## 관련 코드

- [PalCaptureComponent.cpp](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Player/Utility/PalCaptureComponent.cpp)
- [PalInventoryComponent.cpp](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Player/Utility/PalInventoryComponent.cpp)
- [PalPartnerSkillComponent.cpp](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Player/Utility/PalPartnerSkillComponent.cpp)
- [BaseMonster.cpp](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Monster/BaseMonster.cpp)
- [BossMonster.cpp](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Monster/Boss/BossMonster.cpp)
