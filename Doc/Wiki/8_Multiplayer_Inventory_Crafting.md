# 8. Multiplayer Inventory & Crafting

> **같은 Item 데이터를 다루더라도 개인 Inventory, 공유 Container, Crafting Station은 소유권과 동시성 조건이 다릅니다.**

이 시스템의 핵심은 Item 처리 코드를 하나로 만드는 것이 아니라, **상태를 누가 보고 누가 변경할 수 있는지에 따라 network policy와 UI 책임을 구분하는 것**입니다.

---

## 1. Player Inventory — Owner-only FastArray

`UInventoryComponent`는 authoritative 배열과 network용 FastArray를 함께 관리합니다.

```text
Server InventoryItems
      ↓ dirty entry
FRepInventoryList (FastArray)
      ↓ COND_OwnerOnly
Owning Client
      ↓ OnRep_RepItems
Local InventoryItems mirror
      ↓ Delegate
UI
```

### 왜 FastArray인가

Inventory는 슬롯 일부만 바뀌는 일이 많습니다.

- Item count 변화
- 두 슬롯 swap
- 한 슬롯 remove
- 한 슬롯 insert

전체 배열을 매번 다시 보내기보다 변경 entry를 dirty 처리합니다.

또한 개인 Inventory는 다른 Client가 볼 이유가 없으므로 owner-only 조건으로 복제합니다.

---

## 2. Prediction / Request / Reconciliation

Drag & Drop의 응답성을 위해 일부 operation은 Client Prediction을 지원합니다.

Swap 예:

```text
SlotWidget Drop
    ↓
UInventoryComponent::SwapItems
    ├─ Client: PerformClientPrediction_SwapItems
    └─ RPC: ServerSwapItems
                ↓
         Server authoritative swap
                ↓
          FastArray replication
                ↓
        OnRep → local mirror rebuild
```

Prediction은 UI 전용 가짜 배열을 별도로 두는 방식이 아니라 Client local mirror를 먼저 변경합니다. 서버 결과가 다르면 `OnRep_RepItems`가 authoritative state로 다시 구성합니다.

### Trade-off

Prediction 로직과 Server mutation 로직이 따로 존재하므로 두 경로가 같은 규칙을 따라야 합니다. 복잡한 operation일수록 misprediction 가능성이 증가하기 때문에, 현재는 반응성이 중요한 제한된 조작에 적용합니다.

---

## 3. “Inventory 변경”과 “새 Item 획득”은 다른 Event다

같은 `AddItem`이라도 UI 관점에서는 의미가 다릅니다.

예:

- 필드 Item 획득 → 획득 popup 필요
- 장비 해제 후 Inventory 복귀 → popup 불필요
- 슬롯 이동 → popup 불필요

`IsDirectAcquisition`과 별도 delegate를 통해 이를 구분합니다.

- `OnInventoryChanged`: 슬롯 상태가 바뀌면 항상
- `OnItemAdded`: 직접 획득 의미가 있을 때만

단순 data mutation과 player-facing semantic event를 분리한 사례입니다.

---

## 4. Equipment와 Skill Grant 연결

장비 변경은 Item 위치만 바꾸지 않습니다.

Inventory가 장비 state를 변경하면:

- Stat bonus 적용/해제
- Weapon mesh / HUD 갱신
- 활성 무기에 따른 Skill Grant 교체

같은 후속 처리가 이어집니다.

`ActiveWeaponGrantId`를 보관하여 무기 교체 시 `ReplaceGrant`로 해당 source가 부여한 skill set을 교체합니다.

---

## 5. Shared Container — Subscriber-based FastArray

Container는 Player Inventory와 달리 월드에 놓인 공유 state입니다.

모든 Client가 모든 Container 내부를 항상 받을 필요는 없습니다.

`UContainerComponent::PreReplication`:

```cpp
const bool bActive = Subscribers.Num() > 0;
DOREPLIFETIME_ACTIVE_OVERRIDE(
    UContainerComponent,
    RepContainerItems,
    bActive);
```

Player가 Container를 열면 `ServerSubscribeViewer`, 닫으면 `ServerUnsubscribeViewer`로 viewer set을 갱신합니다.

### Inventory와 Container의 차이

| | Player Inventory | Container |
|---|---|---|
| 소유 범위 | 한 Player | World shared |
| Replication | Owner-only | Viewer가 있을 때 활성 |
| Delta | FastArray | FastArray |
| Client local mirror | 있음 | 있음 |

같은 Item 구조라도 소유권에 맞춰 replication policy를 다르게 선택합니다.

---

## 6. Crafting Station — 상태에 따라 같은 Interaction을 다르게 해석

Crafting Station은 한 개의 F 키가 항상 “UI 열기”를 의미하지 않습니다.

`Interact_Implementation`은 현재 station state를 보고 행동을 라우팅합니다.

```text
Interact
   ↓
Completed result? → Collect
   ↓ no
Active work?      → Add Work
   ↓ no
Idle              → Open Recipe UI
```

이를 통해 별도 “돕기 버튼”, “수령 버튼”을 월드에 추가하지 않고 현재 상태를 interaction 의미에 반영합니다.

---

## 7. Recipe 선택 구간의 동시성 제어

여러 Player가 동시에 Recipe UI를 열고 서로 다른 작업을 시작하면 재료와 ActiveWork가 충돌할 수 있습니다.

`UIOwner`는 **Recipe 선택/시작 구간에 대한 exclusive owner** 역할을 합니다.

```cpp
if (UIOwner && UIOwner != Player)
    return;

UIOwner = Player;
```

중요한 점은 Station 전체를 잠그지 않는다는 것입니다.

작업이 시작된 뒤 다른 Player는:

- 작업 돕기
- 완료 결과 수령

같은 collaborative interaction을 계속 할 수 있습니다.

즉 lock의 범위를 데이터 경쟁이 실제로 발생하는 구간으로 제한합니다.

---

## 8. Server-authoritative Crafting Lifecycle

제작 요청 시 Server는 Client UI의 계산값을 그대로 믿지 않습니다.

1. Recipe 조회
2. Server 기준 최대 제작 가능 수량 계산
3. Inventory 재료 검증/소모
4. `ActiveWork` 설정
5. 작업량 누적
6. Unit completion
7. `CompletedToCollect` 증가
8. 수령 시 결과 Item 지급

작업을 취소하면 이미 완료된 Unit은 유지하고, **미완료 Unit에 해당하는 재료만 환불**합니다.

---

## 9. Resource 계산을 UI와 분리

Crafting Widget도 “재료가 충분한가?”를 알아야 하지만 Server Station의 구현 세부를 복사하지 않습니다.

`UInventoryResourceProvider`가:

- Item count
- Required materials
- Max craftable

같은 read-only 계산을 제공합니다.

Server는 최종 validation을 다시 수행하고, UI는 동일 규칙을 사용자 피드백에 활용합니다.

---

## 10. Crafting UI — 정적/동적 갱신 분리

Recipe detail에서 모든 child widget을 매번 다시 만들 필요는 없습니다.

`UCraftingWidget`은:

### Recipe가 바뀔 때
- 결과 Item name/icon
- Required Material row 구성
- icon/layout

### Inventory / Quantity가 바뀔 때
- 보유 수량
- 필요 수량
- 가능/불가능 색
- button enable

만 갱신합니다.

Required Material row는 local pool에 보관해 필요한 개수까지 생성하고 나머지는 숨깁니다.

---

## 11. Crafting Queue — Event + 최소 Tick

`UCraftingQueueWidget`은 `OnWorkChanged`, `OnCompletedChanged`에 bind해 Item / count / 상태를 갱신합니다.

연속적으로 변하는 progress visual만 현재 work progress를 읽어 표시합니다.

즉 모든 데이터를 Tick polling하지 않고 **이산 상태 변화와 연속 presentation을 구분**합니다.

---

## 관련 코드

- [InventoryComponent](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Player/Utility/InventoryComponent.h)
- [ContainerComponent](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/GameObject/Buildings/Utility/ContainerComponent.h)
- [CraftingStation](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/GameObject/Buildings/Crafting/CraftingStation.h)
- [CraftingWidget](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/UI/Widget/GameObject/Crafting/CraftingWidget.cpp)
- [InventoryResourceProvider](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/Utilities/InventoryResourceProvider.h)
