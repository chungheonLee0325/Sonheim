# 12. Multiplayer Synchronization

Sonheim의 multiplayer 구현은 **Server가 gameplay 결과를 확정하고, Client는 요청과 presentation을 담당하는 구조**를 사용합니다.

이 문서의 핵심은 Unreal RPC/Replication API 자체보다 다음 네 가지 문제를 어떻게 나눴는지에 있습니다.

1. **요청과 상태를 분리한다.**
2. **데이터를 필요한 Client 범위에만 동기화한다.**
3. **입력 반응성이 필요한 부분에는 Prediction을 제한적으로 사용한다.**
4. **현재 상태와 일회성 연출을 서로 다른 방식으로 전달한다.**

---

## 1. State와 Request를 분리한다

### Request — RPC

Client가 “무엇을 하고 싶다”는 의도는 Server RPC로 전달합니다.

예:

- Skill Cast
- Inventory Swap
- Crafting Start
- Container Transfer
- Dungeon Switch
- Capture Attempt

Server는 요청을 받은 뒤 현재 authoritative state를 기준으로 다시 검증합니다.

### State — Replication

나중에 접속하거나 UI가 다시 만들어져도 현재값을 복구해야 하는 정보는 Replication으로 전달합니다.

예:

- HP
- Inventory
- Skill Spec
- Pal Slot
- Dungeon Run Snapshot

```text
Client Input
   ↓ Request
Server Validation / Mutation
   ↓ State
Replication
   ↓
Client Presentation
```

---

## 2. Player Inventory — Owner-only FastArray

Inventory는:

- slot 일부가 자주 바뀌고
- 개인 정보이며
- 다른 Client가 전체 내용을 받을 필요가 없습니다.

그래서 `FRepInventoryList : FFastArraySerializer`와 `COND_OwnerOnly`를 사용합니다.

```text
Server InventoryItems
      ↓ changed slot
FastArray
      ↓ Owner Only
Owning Client
      ↓ OnRep
Local Mirror / UI
```

FastArray는 배열 전체를 매번 전송하는 대신 dirty entry를 중심으로 delta replication합니다.

---

## 3. Skill Spec — Logic UObject가 아니라 필요한 상태만 복제한다

Skill 실행 객체 `UBaseSkill` 자체를 network state로 사용하지 않습니다.

Client가 알아야 할 정보만 `FSonheimSkillSpecItem`으로 복제합니다.

```cpp
struct FSonheimSkillSpecItem : public FFastArraySerializerItem
{
    int32 SkillId;
    int32 Level;
    bool bIsCasting;
    float CooldownEndTime;
};
```

정적 Skill Data와 실행 UObject는 각 machine에서 조회/생성하고, network에는 **보유·casting·cooldown 상태만 전달**합니다.

---

## 4. Shared Container — Viewer가 있을 때만 내부 상태를 복제한다

Container는 Player Inventory와 달리 World shared object입니다.

Owner-only를 사용할 수 없으므로 `Subscribers`를 관리하고, viewer가 하나라도 있을 때 Item FastArray replication을 활성화합니다.

```cpp
const bool bActive = Subscribers.Num() > 0;

DOREPLIFETIME_ACTIVE_OVERRIDE(
    UContainerComponent,
    RepContainerItems,
    bActive);
```

현재 구현은 **property 자체를 활성/비활성화하는 방식**이며, connection별로 서로 다른 Container item payload를 보내는 구조는 아닙니다.

---

## 5. Client Prediction / Reconciliation

Server Authority를 유지하더라도 모든 UI 조작에서 round-trip을 기다리면 입력 반응성이 떨어집니다.

Inventory Slot Swap은 Client가 먼저 local state를 바꿉니다.

```text
Drag A → B
   ↓
Local Prediction
   ↓
ServerSwapItems(A, B)
   ↓
Server Validation / Mutation
   ↓
FastArray Replication
   ↓
Client Reconciliation
```

Pal Slot 전환도 같은 원리로 예상 index를 UI에 먼저 보여주고 Server 결과로 확정합니다.

Prediction은 gameplay 결과를 Client에게 넘기는 것이 아니라 **사용자에게 즉각적인 feedback을 제공하는 범위에 한정**합니다.

---

## 6. Crafting — Shared Workflow의 Server 검증

Crafting은 여러 Player가 같은 Station에 접근하므로 단순 property replication보다 **동시성 제어와 request validation**이 중요합니다.

Server는:

1. Recipe 확인
2. 재료 수량 검증
3. 재료 소모
4. ActiveWork 설정
5. 작업 진행
6. 완료 결과 저장
7. Collect 시 Inventory 지급

을 순서대로 처리합니다.

Recipe 선택/시작 구간에는 `UIOwner`를 사용해 동시에 다른 작업이 시작되는 것을 제한하고, 작업이 시작된 뒤의 Assist/Collect는 다른 Player도 참여할 수 있게 합니다.

---

## 7. Capture — 결과와 Reveal을 분리한다

Capture 성공 여부는 Server가 먼저 판정합니다.

하지만 Monster ownership을 즉시 변경하지 않고 Reveal parameter를 Multicast한 뒤 연출 시간이 끝나는 시점에 실제 결과를 적용합니다.

```text
Server Result
   ├─ Multicast Reveal
   └─ Delay
        ↓
   Apply Ownership
```

여기서:

- **성공 여부 / ownership** → authoritative gameplay state
- **capture reveal** → 순간 presentation

으로 역할을 분리합니다.

---

## 8. Dungeon — 복합 상태를 Snapshot으로 복제한다

Dungeon은 Stage change RPC를 여러 개 보내는 방식보다 현재 Run 전체를 `FDungeonStageRuntimeState`로 구성합니다.

```text
Dungeon Runtime
   ↓ Publish
GameState.DungeonStageState
   ↓ Replication
Client Presenter
```

Snapshot에는:

- Stage / Revision
- Objective
- Branch
- Timer
- Barrier
- Participant
- Boss state
- Reward / Result

가 함께 들어갑니다.

UI가 늦게 생성되거나 다시 만들어져도 현재 Snapshot을 기준으로 복구할 수 있습니다.

---

## 9. Transient RPC와 Relevancy

Multicast는 “현재 상태”를 보존하지 않습니다.

Guardian wake 연출에서 Player teleport 직후 Boss가 아직 해당 Client에 net-relevant하지 않은 시점에 Multicast가 발생하면 Client가 RPC를 받지 못하는 문제가 있었습니다.

이 사례는:

```text
지속되어야 하는 정보 → Replicated State
그 순간에만 필요한 연출 → RPC
```

구분이 필요한 이유를 보여줍니다.

현재 상태 복구가 필요한 정보를 Multicast 하나에만 의존하지 않습니다.

---

## 10. UE Networking에 종속된 부분과 일반화되는 부분

이 프로젝트의 실제 전송 구현은 **Unreal RPC / Replication / FastArray를 사용하는 Listen Server 구조**입니다.

따라서 외부 전용 게임 서버와의 socket protocol, packet serialization, reconnect/session recovery를 구현한 프로젝트와 동일하게 볼 수는 없습니다.

반면 다음 설계 문제는 전송 계층이 바뀌어도 그대로 남습니다.

| Sonheim의 구현 | 일반적인 Client/Server 문제 |
|---|---|
| Server RPC | Client command / request |
| Replicated Property | Authoritative state update |
| FastArray delta | Collection delta synchronization |
| Client Prediction | Latency hiding |
| Reconciliation | Server result로 local state 보정 |
| GameState Snapshot | 복합 state snapshot / model update |
| Presenter/ViewData | Network model과 UI model 분리 |

즉 Unreal Networking API 자체보다 **Request / State / Prediction / Presentation 경계를 나눈 경험**을 중심으로 봅니다.

---

## 설계 선택과 비용

| 문제 | 선택 | 비용 | 적용 범위 |
|---|---|---|---|
| 공유 결과 일관성 | **Server Authority** | round-trip 발생 | Combat, Crafting, Capture, Dungeon |
| 개인 배열 동기화 | **Owner-only FastArray** | dirty/local mirror 관리 | Inventory, Skill Spec |
| 공유 Container | **Subscriber 기반 활성화** | subscribe lifecycle 관리 | Container item state |
| UI 입력 반응성 | **제한적 Prediction** | reconciliation 경로 필요 | Inventory/Pal slot |
| 복합 콘텐츠 복구 | **Replicated Snapshot** | snapshot/mapping code 추가 | Dungeon |
| 순간 연출 | **RPC / Multicast** | 유실 가능성·relevancy 고려 | Capture reveal, 일부 VFX |

---

## 연관 문서

- [[2. Gameplay Architecture|02_Gameplay_Architecture]] — 네트워크 이전의 상태 소유권·수명·의존 구조
- [[7. Inventory & Crafting|07_Inventory_Crafting]] — FastArray, Subscriber, Prediction의 실제 gameplay 적용
- [[8. Pal Capture & Partner Lifecycle|08_Pal_Capture_Partner_Lifecycle]] — Capture 결과와 presentation 분리
- [[11. Client State & Presentation Pipeline|11_Client_State_Presentation_Pipeline]] — 복제 상태가 UI model로 변환되는 과정

---

## 관련 코드

- [InventoryComponent](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Player/Utility/InventoryComponent.h)
- [ContainerComponent](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/GameObject/Buildings/Utility/ContainerComponent.h)
- [SonheimSkillComponent](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Skill/SonheimSkillComponent.h)
- [SonheimGameState](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/GameManager/SonheimGameState.h)
