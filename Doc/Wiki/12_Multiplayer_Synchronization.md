# 12. Multiplayer Synchronization

Sonheim의 실제 네트워크 구현은 **Unreal Engine Listen Server + RPC / Replication / FastArray**를 사용합니다.

이 문서에서는 API 자체보다 gameplay 상태를 동기화할 때 반복해서 마주친 문제를 다룹니다.

- **Request와 State를 분리**
- **State를 필요한 소비자 범위에만 전달**
- **입력 반응성이 필요한 곳만 Prediction**
- **현재 상태와 순간 연출을 분리**
- **UI는 동기화 모델을 그대로 그리지 않고 Presentation Model로 변환**

---

## 전체 동기화 모델

~~~mermaid
flowchart LR
    INPUT["Client Input"]
    REQUEST["<b>Request</b><br/>Server RPC"]
    AUTH["Authoritative Gameplay"]
    STATE["<b>State Update</b><br/>Replication / FastArray / Snapshot"]
    LOCAL["Client Model"]
    VIEW["Presentation"]

    INPUT --> REQUEST
    REQUEST --> AUTH
    AUTH --> STATE
    STATE --> LOCAL
    LOCAL --> VIEW

    INPUT -. "제한적 Prediction" .-> LOCAL
    STATE -. "Reconciliation" .-> LOCAL
~~~

Client가 요청을 보내는 것과 Server가 확정한 현재 상태를 받는 것을 같은 통신으로 취급하지 않습니다.

---

## 1. Request와 State

### Request

Client가 실행 의도를 전달합니다.

예:

- Skill Cast
- Inventory Slot Swap
- Container Transfer
- Crafting Start / Assist / Collect
- Capture Attempt
- Dungeon Interaction

이 요청 자체가 gameplay 결과는 아닙니다.

Server는 현재 상태에서 다시 조건을 검사하고 mutation을 수행합니다.

### State

나중에 UI가 생성되거나 Client가 해당 Actor 상태를 다시 보게 되어도 복구해야 하는 값은 Replication으로 유지합니다.

예:

- Health
- Inventory
- Equipment
- Skill Spec
- Pal selection
- Boss Status
- Dungeon Run Snapshot

~~~text
Client Command
     ↓
Server Validation
     ↓
Authoritative Mutation
     ↓
State Synchronization
~~~

---

## 2. 동기화 범위는 데이터의 소비자를 기준으로 선택

모든 replicated property를 모든 Client에게 같은 방식으로 보내지 않습니다.

| 상태 | 소비 범위 | 구현 |
|---|---|---|
| Player Inventory | 소유 Player | Owner-only FastArray |
| Skill Spec | 해당 Character를 표현할 Client | FastArray |
| Pal Inventory / Selected Slot | 소유 Player | Owner-only Replication |
| Container Item | Container 사용 상황 | Subscriber 존재 시 property 활성화 |
| Character Movement | 주변 Client | UE CharacterMovement |
| Dungeon Run | 참여 화면/World에서 공유 | GameState Snapshot |
| Boss Status | Boss를 표현할 Client | Actor Replication + Dungeon Snapshot |

핵심은 “FastArray가 좋다”가 아니라 **누가 이 상태를 실제로 소비하는가에 따라 scope와 형태를 선택**한 것입니다.

---

## 3. 변경이 잦은 Collection — FastArray

Player Inventory는 일부 slot만 자주 바뀝니다.

~~~text
Inventory[42]
  Slot 3 Count 변경
       ↓
Dirty Entry
       ↓
FastArray Delta
~~~

전체 배열을 매번 다시 보내는 대신 변경 entry를 표시합니다.

Skill도 실행 UObject를 복제하지 않고 작은 Spec만 FastArray에 둡니다.

~~~cpp
struct FSonheimSkillSpecItem
    : public FFastArraySerializerItem
{
    int32 SkillId;
    int32 Level;
    bool bIsCasting;
    float CooldownEndTime;
};
~~~

**실행 object와 network state를 같은 것으로 만들지 않습니다.**

---

## 4. 개인 State와 공유 World State는 같은 Item이어도 정책이 다르다

Player Inventory:

~~~text
Owner-only
→ 다른 Player가 전체 내용을 알 필요 없음
~~~

Container:

~~~text
World shared object
→ Owner 한 명으로 표현할 수 없음
→ viewer가 있을 때 내부 Item replication 활성
~~~

현재 Container 구현은 \`Subscribers.Num() > 0\`일 때 property replication 자체를 활성화합니다.

~~~cpp
const bool bActive = Subscribers.Num() > 0;

DOREPLIFETIME_ACTIVE_OVERRIDE(
    UContainerComponent,
    RepContainerItems,
    bActive);
~~~

이 방식은 **per-connection payload filtering**까지 수행하지는 않습니다.

규모가 커지고 Container가 많아지면 Replication Graph나 connection별 policy를 고려할 수 있지만, 현재 프로젝트 범위에서는 “사용되지 않는 공유 내부 상태를 항상 활성화하지 않는다”는 수준을 선택했습니다.

---

## 5. Prediction은 조작 피드백이 중요한 곳만 사용

Server Authority가 필요한 것과 Client가 즉시 보여줄 수 있는 것은 별개입니다.

Inventory Slot Swap:

~~~text
Drag A → B
   ↓
Client Local Swap
   ↓
Server Request
   ↓
Authoritative Swap
   ↓
FastArray
   ↓
Reconciliation
~~~

Pal Slot도 예상 index를 HUD에 먼저 반영한 뒤 Server 상태로 확정합니다.

반면 Crafting 재료 소비나 Capture 성공 판정처럼 **잘못 예측했을 때 gameplay 결과가 크게 달라지는 상태는 Client가 먼저 확정하지 않습니다.**

Prediction 적용 범위를 UX 효과와 복구 비용을 비교해 제한했습니다.

---

## 6. 현재 상태와 순간 연출은 서로 다른 수단을 사용

### 지속되어야 하는 현재 상태

- Boss Phase
- Boss Action
- Dungeon Stage
- Inventory
- HP

→ Replication

### 그 순간만 필요한 연출

- Capture Reveal
- 일부 VFX / SFX
- Montage trigger

→ RPC / Multicast

이 구분이 없으면 RPC를 놓친 Client가 현재 상태를 복구하지 못합니다.

---

## 7. Net Relevancy로 놓친 Boss Wake 사례

Dungeon 입장 직후 Server는 Player를 이동시키고 Boss Wake 연출을 시작했습니다.

이 시점에 Server가 아직 해당 Client를 Boss의 network-relevant connection으로 판단하지 않으면 순간 Multicast가 전달되지 않을 수 있습니다.

~~~text
Teleport
   ↓
Boss Wake Multicast
   ↓
Client relevancy가 아직 갱신되지 않음
   ↓
RPC 유실
~~~

이 문제는 “Reliable이면 반드시 나중에 받는다”는 식으로 해결할 수 있는 문제가 아니었습니다.

그 결과 Boss에서는:

- **현재 Action / Phase / Break** → Replicated Status
- **순간 Montage / Effect** → Presentation Event

를 구분합니다.

현재 상태를 반드시 알아야 하는 정보가 순간 RPC 하나에만 의존하지 않도록 했습니다.

---

## 8. Capture — 판정 State와 Reveal Event 분리

Capture는 Server가 성공 여부를 먼저 확정하지만 ownership을 즉시 적용하지 않습니다.

~~~text
Server Capture Result
       ↓
Reveal Params Multicast
       ↓
Animation
       ↓
Server Apply Outcome
       ↓
PartnerOwner / PalInventory
~~~

여기서:

- Capture 성공 여부와 Ownership → gameplay result
- Segment animation → transient presentation

입니다.

모든 Client가 같은 Reveal parameter를 받되, 실제 ownership mutation은 Server가 수행합니다.

---

## 9. Dungeon — 복합 상태는 Snapshot으로 전달

Dungeon은 Stage 변화마다 작은 RPC를 여러 개 쏘는 대신 현재 Run 전체를 \`FDungeonStageRuntimeState\`로 발행합니다.

~~~text
Server Dungeon Runtime
      ↓
Run Snapshot
      ↓ GameState Replication
Client Presenter
      ↓
ViewData
~~~

Snapshot은:

- Stage / Revision
- Objective
- Branch
- Timer
- Barrier
- Participants
- Boss
- Reward / Result

를 같은 Run 기준으로 묶습니다.

HUD가 늦게 생성되거나 다시 생성돼도 “과거 Event 목록”을 재생하지 않고 **현재 Snapshot으로 복원**할 수 있습니다.

Presentation 변환은 [[11. UI Architecture & Client Presentation|11_Client_State_Presentation_Pipeline]]에서 설명합니다.

---

## 10. UE 구현과 일반 Client/Server 문제의 대응

Sonheim이 구현한 전송 계층은 Unreal Networking입니다.

외부 전용 게임 서버에서 흔히 필요한:

- 별도 socket protocol 설계
- packet serialization / framing
- reconnect protocol
- 서버 프로세스 간 routing

을 Sonheim에서 구현했다고 표현하지 않습니다.

다만 Client가 다루는 상태 문제는 다음처럼 대응됩니다.

| Sonheim | 일반적인 Client / Server 문제 |
|---|---|
| Server RPC | Client Command / Request |
| Replicated Property | Authoritative State Update |
| FastArray Delta | Collection Delta Synchronization |
| Client Prediction | Latency Hiding |
| Reconciliation | Server 결과로 Local State 보정 |
| GameState Snapshot | Composite State Snapshot |
| Presenter / ViewData | Network/Game Model과 UI Model 분리 |
| Multicast | Transient Event / Presentation |

따라서 이 프로젝트의 네트워크 경험은 **UE API 사용 경험과 함께 Request / State / Scope / Prediction / Presentation의 경계를 설계한 경험**으로 한정해 설명합니다.

---

## 설계 선택과 비용

| 선택 | 얻은 것 | 비용 / 제약 |
|---|---|---|
| **Server Authority** | 공유 결과를 한 곳에서 확정 | round-trip latency |
| **Owner-only / conditional scope** | 불필요한 state 소비 범위 제한 | ownership/subscriber lifecycle 관리 |
| **FastArray** | 변경이 잦은 collection delta | dirty entry / local mirror 관리 |
| **제한적 Prediction** | Drag/Slot UX의 즉각성 | reconciliation과 동일 규칙 유지 필요 |
| **Replicated Snapshot** | 복합 콘텐츠의 현재 상태 복원 | snapshot/presentation mapping 증가 |
| **Transient RPC 분리** | 순간 연출을 state와 분리 | relevancy와 유실 가능성 고려 필요 |

---

## 연관 문서

- [[02. Gameplay Architecture|02_Gameplay_Architecture]] — 네트워크 이전의 State Ownership / Lifetime 경계
- [[05. Combat, Skill & Animation|05_Combat_Skill_Animation]] — Skill Spec과 authoritative cast
- [[07. Inventory & Crafting|07_Inventory_Crafting]] — Inventory / Container / Crafting의 실제 상태
- [[08. Pal Capture & Partner Lifecycle|08_Pal_Capture_Partner_Lifecycle]] — Capture result / Reveal / Ownership
- [[11. UI Architecture & Client Presentation|11_Client_State_Presentation_Pipeline]] — Snapshot에서 Client UI model로 변환

---

## 관련 코드

- [InventoryComponent](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Player/Utility/InventoryComponent.h)
- [ContainerComponent](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/GameObject/Buildings/Utility/ContainerComponent.h)
- [SonheimSkillComponent](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Skill/SonheimSkillComponent.h)
- [PalCaptureComponent](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Player/Utility/PalCaptureComponent.h)
- [SonheimGameState](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/GameManager/SonheimGameState.h)
