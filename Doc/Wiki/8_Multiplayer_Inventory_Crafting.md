# 8. Multiplayer Inventory & Crafting

Sonheim의 Inventory, Container, Crafting은 같은 Item data를 사용하지만 **소유권과 replication 범위가 서로 다릅니다.**

이 문서는 기존 Inventory / Container / Crafting / Inventory UI / Crafting UI / 관련 Case Study를 하나의 multiplayer lifecycle 관점으로 통합합니다.

---

## Inventory — Owner 중심 상태

`UInventoryComponent`는 Fast Array를 사용해 slot 변경분을 delta replication합니다.

주요 기능:

- Add / Remove / Consume
- Stack / Split
- Drag & Drop
- Discard / Trash
- Equipment
- Skill Grant 연계
- Client Prediction / Server Reconciliation

### Prediction

Drag & Drop 같은 조작에서 서버 round-trip 동안 UI가 멈춘 것처럼 보이지 않도록 클라이언트가 먼저 예상 결과를 표현합니다.

최종 상태는 서버 authoritative result로 조정합니다.

---

## Shared Container — 구독 기반 복제

Container는 여러 플레이어가 동시에 열 수 있는 shared state입니다.

모든 Container item state를 항상 모든 client에게 보내지 않고, 실제 사용자가 존재할 때 replication을 활성화하는 방향으로 구성했습니다.

즉:

- Player Inventory → owner 중심
- Container → subscriber 중심

으로 같은 item model이라도 replication policy를 다르게 적용합니다.

---

## Crafting Station — 공유 World Actor

`ACraftingStation`은 여러 player가 접근할 수 있는 shared actor입니다.

서버가 다음 상태를 관리합니다.

- Recipe validation
- Resource consumption
- Queue
- Cumulative progress
- Collect
- Cancel
- Interaction / UI owner
- Completion

동시에 여러 player가 같은 station을 조작할 수 있으므로, state transition을 서버에서 직렬화하고 중복 요청을 방어해야 합니다.

---

## Resource Abstraction

Crafting UI와 station이 `UInventoryComponent`의 내부 구현에 직접 의존하지 않도록 resource 조회와 규칙 일부를 별도 utility/provider로 분리했습니다.

- `InventoryResourceProvider`
- `InventoryRulesLibrary`

이렇게 하면 “재료가 충분한가?”라는 질문과 실제 inventory slot 구현을 분리할 수 있습니다.

---

## UI는 Gameplay State의 소비자

Inventory / Crafting UI는 slot, queue, recipe state를 표현하고 interaction intent를 전달하지만 authoritative item/crafting state를 소유하지 않습니다.

기존의 별도 Inventory UI / Crafting UI 문서를 독립적으로 유지하기보다, 이 문서에서 **데이터 소유권과 UI 역할을 함께 설명**합니다.

---

## 관련 코드

- [InventoryComponent](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Player/Utility/InventoryComponent.h)
- [ContainerComponent](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/GameObject/Buildings/Utility/ContainerComponent.h)
- [CraftingStation](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/GameObject/Buildings/Crafting/CraftingStation.h)
- [Inventory UI](https://github.com/chungheonLee0325/Sonheim/tree/main/Sonheim/Source/Sonheim/UI/Widget/Player/Inventory)
- [Crafting UI](https://github.com/chungheonLee0325/Sonheim/tree/main/Sonheim/Source/Sonheim/UI/Widget/GameObject/Crafting)
