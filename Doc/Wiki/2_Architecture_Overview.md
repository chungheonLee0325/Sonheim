# 2. Architecture Overview

> **공유 상태는 서버가 소유하고, 객체의 수명에 맞는 UE Framework 계층에 책임을 배치하며, UI와 콘텐츠 로직은 상태/이벤트 경계를 통해 연결합니다.**

Sonheim은 하나의 거대한 전역 아키텍처를 강제하기보다, 시스템마다 먼저 세 가지를 결정합니다.

1. **누가 상태를 소유하는가?**
2. **그 상태는 얼마나 오래 살아야 하는가?**
3. **다른 시스템은 상태 자체가 필요한가, 변화 이벤트만 필요한가?**

이 기준은 Inventory, Crafting, Capture, Dungeon, UI 모두에 공통으로 적용됩니다.

---

## 1. Server Authority: 최종 상태 변경은 서버에서

클라이언트는 행동을 요청할 수 있지만, 공유 게임 결과를 직접 확정하지 않습니다.

예:

- Inventory item 추가/제거/장착
- Crafting 재료 소모와 결과 지급
- Pal Capture 성공 여부
- Dungeon Stage 진행과 Reward
- Boss Damage / Phase / Capture 가능 상태

### 상태와 행위를 구분한다

지속되어야 하는 정보는 Replication으로, 순간적인 요청/연출은 RPC로 처리합니다.

| 성격 | 수단 | 예 |
|---|---|---|
| 현재 상태 | Replication / RepNotify | HP, Inventory, Dungeon Snapshot |
| 클라이언트 요청 | Server RPC | Swap Item, Cast Skill |
| 특정 Client 명령 | Client RPC | Crafting UI Open |
| 순간 연출 | Multicast | Capture Reveal, 일부 VFX/SFX |

이 구분이 중요한 이유는 **late join / relevancy 변화 / UI 재생성** 때문입니다. 순간 RPC만으로 “현재 상태”를 전달하면 그 사건을 놓친 클라이언트가 상태를 복구할 방법이 없습니다.

Dungeon은 이 원칙을 명확하게 보여줍니다.

```text
UDungeonStageRuntimeSubsystem (Server)
              ↓ publish
FDungeonStageRuntimeState
              ↓ replication
ASonheimGameState
              ↓
Clients
```

---

## 2. Replication Policy는 데이터의 소비 범위에 맞춘다

모든 replicated data를 같은 방식으로 보내지 않습니다.

### Player Inventory — Owner-only FastArray

`UInventoryComponent`는 `FRepInventoryList : FFastArraySerializer`를 사용하고 `COND_OwnerOnly`로 복제합니다.

- 슬롯 단위 dirty marking
- 변경된 entry 중심 delta replication
- 다른 플레이어의 개인 Inventory는 전송하지 않음
- Client는 `OnRep_RepItems`에서 local mirror를 다시 구성

### Shared Container — Subscriber-based Replication

Container는 월드의 공유 객체이므로 owner-only로 처리할 수 없습니다. 대신 `PreReplication`에서 viewer가 존재할 때만 item FastArray replication을 활성화합니다.

```cpp
const bool bActive = Subscribers.Num() > 0;
DOREPLIFETIME_ACTIVE_OVERRIDE(
    UContainerComponent,
    RepContainerItems,
    bActive);
```

즉 “개인 데이터”와 “공유 데이터”가 같은 Item 구조를 사용하더라도 **누가 필요로 하는가**에 따라 replication policy가 다릅니다.

---

## 3. Client Prediction은 Authority의 대체가 아니라 보완이다

서버 응답을 기다리면 사용감이 크게 나빠지는 UI 조작에는 제한적으로 prediction을 사용합니다.

Inventory Swap 예:

```text
Drag A → B
   ↓
Client local swap
   ↓
ServerSwapItems(A, B)
   ↓
Server validation / mutation
   ↓
FastArray replication
   ↓
Client reconciliation
```

Prediction 결과가 맞으면 화면 변화가 없고, 서버 결과가 다르면 replicated state가 local mirror를 다시 구성합니다.

중요한 점은 **prediction이 서버 검증을 생략하지 않는다는 것**입니다.

---

## 4. 객체 수명에 맞춰 책임을 배치한다

| UE 계층 | Sonheim에서의 주요 책임 |
|---|---|
| GameInstance | DataTable cache, session-level data |
| GameInstanceSubsystem | Dungeon asset load, persistent progress |
| WorldSubsystem | 현재 World의 Dungeon Runtime |
| GameState | 여러 Client가 공유할 Dungeon Snapshot |
| PlayerState | Pawn 교체와 분리해야 하는 Player data |
| PlayerController | 연결 단위 요청, HUD bootstrap, Client RPC |
| LocalPlayerSubsystem | Dungeon UI routing, Notice |
| Pawn / Actor | 실제 월드 gameplay |
| ActorComponent | Health, Skill, Inventory, Capture, Interaction 등 기능 단위 상태 |

### Pawn / PlayerState 분리

Player가 죽거나 Pawn이 교체될 수 있는 상황에서 Inventory / Stat처럼 player identity에 가까운 상태를 Pawn lifecycle과 묶지 않습니다.

반대로 이동, mesh, animation처럼 월드의 실제 body에 종속된 기능은 Pawn에 둡니다.

---

## 5. Composition + Facade

`AAreaObject`는 Player/Monster 공통 gameplay entry point이지만, Health/Skill/Condition 등의 구현을 모두 직접 소유하지는 않습니다.

```text
AAreaObject
 ├─ UHealthComponent
 ├─ UStaminaComponent
 ├─ UConditionComponent
 ├─ ULevelComponent
 ├─ USonheimSkillComponent
 └─ Move / Rotate utility
```

외부에서는 `DecreaseHP`, `CastSkill`, `AddCondition`처럼 AreaObject를 통해 접근할 수 있고, 실제 상태 책임은 해당 component가 담당합니다.

이 방식은 “상속 대신 무조건 component”가 목적이 아니라, **독립적으로 바뀌는 기능의 책임을 분리하면서 외부 사용 API는 단순하게 유지**하기 위한 선택입니다.

---

## 6. 상태 전달과 변화 통지를 구분한다

UI와 subsystem 사이에서도 항상 같은 연결 방식을 사용하지 않습니다.

### Delegate가 적합한 경우

이미 local object가 존재하고 “값이 바뀌었다”는 통지만 필요한 경우:

- Health
- Inventory Changed
- Equipment Changed
- Crafting Work Changed

### Snapshot이 적합한 경우

여러 필드가 함께 한 시점의 콘텐츠 상태를 구성하고, 늦게 생성된 소비자도 현재 상태를 복원해야 하는 경우:

- Dungeon Run
- Stage / Objective
- Participant
- Boss status
- Result

따라서 Sonheim UI는 단순히 “모든 UI를 event-driven으로 구현”하지 않습니다. **상태의 성격에 따라 Delegate와 Replicated Snapshot을 구분**합니다.

자세한 내용은 [[Multiplayer State & UI Pipeline|5_Multiplayer_State_UI_Pipeline]]에서 다룹니다.

---

## 7. 기존 경계가 새 콘텐츠에서도 유지되는가

설계의 유효성은 패턴 이름보다 실제 확장에서 확인할 수 있습니다.

- `IInteractableInterface` → Item/Container/Crafting뿐 아니라 Dungeon Portal/Switch에도 사용
- Monster death/capture event → Dungeon Objective Tracker가 소비
- Inventory API → Dungeon Reward 지급에 사용
- Pal Capture → Guardian의 Exhaust Capture에 사용
- Dungeon 전용 Toast → 공용 `UNoticeSubsystem`으로 확장

---

## Trade-offs

### Prediction은 reconciliation 경로를 요구한다
로컬 상태를 먼저 바꾸는 만큼 서버 결과가 다른 경우를 항상 허용해야 합니다.

### Component 분리는 의존성 자체를 없애지 않는다
기능 경계를 잘못 나누면 component 간 참조가 오히려 증가할 수 있습니다. Sonheim에서는 delegate/interface/facade를 함께 사용해 직접 결합을 제한합니다.

### Snapshot은 데이터 중복 비용이 있다
Dungeon state를 gameplay runtime과 presentation 사이에서 한 번 더 표현하지만, 그 비용으로 UI lifecycle과 gameplay lifecycle을 분리하고 재구성을 단순화합니다.

---

## 관련 코드

- [AreaObject](https://github.com/chungheonLee0325/Sonheim/tree/main/Sonheim/Source/Sonheim/AreaObject)
- [SonheimGameState](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/GameManager/SonheimGameState.h)
- [InventoryComponent](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Player/Utility/InventoryComponent.h)
- [ContainerComponent](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/GameObject/Buildings/Utility/ContainerComponent.h)
