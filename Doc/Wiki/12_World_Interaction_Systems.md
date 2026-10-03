# 12. World Interaction Systems

> **Player는 “어떤 종류의 Actor인가”보다 “어떤 Interaction contract를 제공하는가”를 기준으로 World object와 상호작용합니다.**

Item, Container, Crafting Station, Dungeon Portal, Shortcut Switch는 동작이 서로 다르지만 Player의 탐지/입력 계층에서 타입별 분기를 계속 추가하지 않습니다.

---

## 1. 의도 기반 Interaction Pipeline

`UInteractionComponent`가 interaction 대상 탐지와 hold state를 관리합니다.

```text
Detection
   ↓
IInteractableInterface
   ↓
CanInteract
   ↓
Prompt / Hold Duration
   ↓
Input
   ↓
Server_TryInteract
   ↓
Interact_Implementation
```

Player는 최종 대상의 concrete class를 몰라도 interface contract를 통해 상호작용합니다.

---

## 2. Detection

`PerformDetection`은 후보를 찾고 interaction 가능한 대상 중 현재 target을 결정합니다.

Target이 바뀌면 이전 대상/새 대상에 detection state를 전달하고 UI 쪽에 현재 interactable 변경을 알립니다.

Detection, UI, gameplay execution을 한 함수에 섞지 않습니다.

---

## 3. Instant / Hold Interaction

같은 F input이라도 대상이 요구하는 interaction 방식이 다를 수 있습니다.

- 즉시 실행
- 일정 시간 Hold
- Hold cancel 가능 여부

Hold progress는 Player interaction state가 관리하고, 실제 완료 action은 Server에서 target의 `Interact_Implementation`으로 연결됩니다.

---

## 4. Context UI — 정보 제공 책임을 Target으로 이동

DetectWidget이:

```text
if Item ...
else if CraftingStation ...
else if Container ...
```

처럼 타입별 text를 결정하면 object 종류가 늘 때마다 UI가 수정됩니다.

대신 `IInteractableInterface`가 interaction name/context를 제공하고 UI는 현재 target이 제공한 값을 표시합니다.

Dungeon Portal / Switch가 추가되어도 DetectWidget의 type branch를 늘릴 필요가 없습니다.

---

## 5. Item — 정적 Data와 Spawn Context 분리

같은 ItemID라도 World에 생성된 이유에 따라 동작이 달라질 수 있습니다.

예:

- Monster Drop → 잠시 후 auto pickup
- Player Drop → 직접 Hold해서 다시 획득
- 특정 Item → Physics impulse
- 일정 시간이 지나면 expire

정적 `FItemData`와 runtime `FItemSpawnOptions`를 분리합니다.

`FItemSpawnOptions` 예:

- bRequireInteraction
- InteractionType
- LifeTime
- AutoPickupDelay
- bApplyPhysicsOnDrop
- DropForce

“곡괭이란 어떤 Item인가”와 “이번 곡괭이는 어떤 상황으로 spawn됐는가”를 나눈 구조입니다.

---

## 6. Item Spawn Preset

반복되는 runtime option 조합은 helper로 만듭니다.

- `MakeDropped`
- `MakeInteractable`

Caller가 여러 bool/float 조합을 매번 직접 만들지 않도록 semantic preset을 제공합니다.

---

## 7. Resource — Damage Pipeline을 Harvest로 해석

Resource Object는 별도의 Harvest 입력 framework를 만들지 않고 `AActor::TakeDamage`를 재정의합니다.

Player의 Pickaxe Skill은 일반 공격처럼 hit을 발생시키고, Target이 Resource라면 그 Damage를 채집 진행으로 해석합니다.

```text
Pickaxe Skill
   ↓
Hit Detection
   ↓
ApplyDamage
   ↓
ABaseResourceObject::TakeDamage
   ↓
HP Segment 감소
   ↓
Resource Spawn
```

공격자는 Target이 Monster인지 Resource인지에 따라 별도 호출을 할 필요가 없습니다.

---

## 8. HP Segment 기반 부분 보상

Resource는 파괴될 때 한 번만 보상을 주는 대신 HP가 일정 구간을 통과할 때 partial resource를 spawn합니다.

예:

```text
HP 95% → Segment 9
HP 75% → Segment 7

Lost Segment = 2
→ Partial Resource × 2
```

한 번에 큰 Damage가 들어가 여러 threshold를 건너뛰더라도 잃은 segment 수를 계산해 중간 보상이 누락되지 않게 합니다.

---

## 9. Interaction과 Physical Hit은 분리된 Pipeline

Item pickup / Crafting / Portal은 “사용 의도”가 중심이라 Interface 기반 Interaction을 사용합니다.

Monster attack / Resource harvest는 collision과 Damage가 중심이라 Combat pipeline을 사용합니다.

Player에게는 둘 다 World interaction이지만 기술적으로 다른 문제이므로 한 generic function에 억지로 합치지 않습니다.

---

## 10. Dungeon 확장

동일 Interaction layer를 다음에 연결했습니다.

- Dungeon Entrance / Portal
- Shortcut Lever
- Reward Chest

Dungeon Runtime은 “Player가 Switch를 사용했다”는 gameplay event만 받고, detection/prompt/input 처리 자체는 기존 Interaction Component를 사용합니다.

---

## Trade-offs

### Interface가 모든 Context를 해결하지는 않는다
복잡한 interaction UI가 필요하면 단순 text/prompt contract 이상으로 ViewData가 필요할 수 있습니다.

### Resource가 Damage system을 재사용하면 Combat 변경의 영향을 받을 수 있다
공통 entry를 사용하는 대신 Resource의 TakeDamage override에서 전투 대상과 다른 규칙을 명확히 격리해야 합니다.

### Item runtime option이 늘면 조합 복잡도가 증가한다
Preset/helper와 validation으로 유효한 조합을 제한할 필요가 있습니다.

---

## 관련 코드

- [InteractionComponent](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Player/Utility/InteractionComponent.h)
- [InteractableInterface](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/GameObject/InteractableInterface.h)
- [BaseItem](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/GameObject/Items/BaseItem.h)
- [ResourceObject](https://github.com/chungheonLee0325/Sonheim/tree/main/Sonheim/Source/Sonheim/GameObject/ResourceObject)
- [Dungeon Objects](https://github.com/chungheonLee0325/Sonheim/tree/main/Sonheim/Source/Sonheim/GameObject/Dungeon)
