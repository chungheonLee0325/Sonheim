# 12. World Interaction Systems

기존 Interaction / Item / Resource / Contextual UI 문서를 **플레이어가 월드 객체와 상호작용하는 하나의 계층**으로 통합합니다.

---

## Interaction Component

Player의 `UInteractionComponent`가 주변 interactable을 탐색하고 `IInteractableInterface`를 통해 상호작용합니다.

Player code가 Item / Container / Crafting Station / Dungeon Portal / Switch의 구체 타입을 각각 알 필요가 없습니다.

---

## Contextual Presentation

상호작용 대상이 자신의 prompt/context를 제공하도록 구성해 Detect UI가 대상 타입마다 분기 로직을 계속 늘리지 않도록 합니다.

```text
Player Detection
      ↓
Interactable Contract
      ├─ Prompt
      ├─ CanInteract
      └─ Interact
```

---

## Item

World Item은 정적 Item data와 실제 spawn/runtime state를 분리합니다.

같은 Item data를 Inventory representation과 World Actor가 공유하되, 두 상태를 동일 객체로 취급하지 않습니다.

---

## Resource

Resource Object는 기존 damage / health pipeline을 재사용합니다.

따라서 채집을 위해 별도의 완전히 다른 hit framework를 만들지 않고, 도구 공격에 반응하는 대상이라는 형태로 확장했습니다.

---

## Container / Crafting

상호작용 진입점은 같지만 이후 shared state와 UI ownership은 각 시스템이 담당합니다.

Interaction layer는 “무엇과 상호작용했는가” 이후의 모든 gameplay를 소유하지 않습니다.

---

## Dungeon 재사용

2025년에 만든 Interaction contract는 2026년 Dungeon Portal / Shortcut Switch에도 그대로 연결됐습니다.

이 점은 abstraction의 목적이 단순 코드 정리가 아니라, **새 콘텐츠 타입을 기존 interaction contract 안에 추가하는 것**임을 보여줍니다.

---

## 관련 코드

- [InteractionComponent](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Player/Utility/InteractionComponent.h)
- [InteractableInterface](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/GameObject/InteractableInterface.h)
- [Items](https://github.com/chungheonLee0325/Sonheim/tree/main/Sonheim/Source/Sonheim/GameObject/Items)
- [ResourceObject](https://github.com/chungheonLee0325/Sonheim/tree/main/Sonheim/Source/Sonheim/GameObject/ResourceObject)
- [Dungeon Objects](https://github.com/chungheonLee0325/Sonheim/tree/main/Sonheim/Source/Sonheim/GameObject/Dungeon)
