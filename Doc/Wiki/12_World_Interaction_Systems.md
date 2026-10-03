# 12. World Interaction Systems

> **이 문서가 답하는 질문**
>
> “Player가 Item, Container, Crafting Station, Dungeon Lever처럼 서로 다른 World Object를 만날 때 Player 코드가 각 concrete type을 모두 알아야 하는가?”

Sonheim은 World interaction을 두 종류로 구분합니다.

```text
의도 기반 Interaction
F / Hold / Prompt / Interface

물리 기반 Interaction
Attack / Collision / Damage
```

둘을 한 generic system으로 억지로 합치지 않습니다.


> **코드 표기:** 아래 코드 블록은 `main`의 실제 선언/함수에서 문서 이해에 필요한 부분을 발췌한 것입니다. `UPROPERTY` metadata나 보조 필드는 일부 생략될 수 있으며, 전체 구현은 하단 Source 링크에서 확인할 수 있습니다.
---

## Part 1. 의도 기반 Interaction Contract

실제 interface의 핵심 API:

```cpp
class IInteractableInterface
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent)
    bool CanInteract() const;

    UFUNCTION(BlueprintNativeEvent)
    void OnDetected(bool bDetected);

    UFUNCTION(BlueprintNativeEvent)
    void Interact(ASonheimPlayer* Player);

    UFUNCTION(BlueprintNativeEvent)
    FString GetInteractionName() const;

    UFUNCTION(BlueprintNativeEvent)
    float GetHoldDuration() const;

    UFUNCTION(BlueprintNativeEvent)
    void UpdateHoldProgressUI(
        float Progress,
        EHoldPurpose Purpose);

    UFUNCTION(BlueprintNativeEvent)
    bool CanHoldCancel() const;

    UFUNCTION(BlueprintNativeEvent)
    void ExecuteCancel(ASonheimPlayer* Player);
};
```

Player는 “이게 Item인가 CraftingStation인가”보다 이 contract를 만족하는지 봅니다.

---

## Part 2. 전체 Pipeline

```text
Detection
 ↓
CanInteract
 ↓
OnDetected
 ↓
Prompt / Hold Duration
 ↓
Input
 ↓
Server_TryInteract
 ↓
Interact_Implementation
```

Detection, UI feedback, actual execution을 구분합니다.

---

## Part 3. Context UI도 Target이 정보를 제공한다

Detect Widget이 concrete type을 검사해:

```text
Item이면 "줍기"
Crafting이면 "제작"
Container면 "열기"
```

를 결정하지 않습니다.

Target의 `GetInteractionName()`, `GetHoldDuration()` 같은 interface data를 소비합니다.

새 Dungeon Lever가 추가돼도 Widget에 새로운 class branch를 추가할 필요가 없습니다.

---

## Part 4. Instant / Hold

`GetHoldDuration() == 0`이면 즉시 상호작용, 값이 있으면 Hold progress를 사용합니다.

같은 input layer에서:

- 즉시 줍기
- 길게 눌러 줍기
- Crafting 작업
- Cancel hold

같은 UX를 처리합니다.

---

## Part 5. Item의 “정적 정체성”과 “이번 Spawn 상황”을 분리한다

Item의 `FItemData`는:

- 이름
- 카테고리
- Stack
- Equipment
- Mesh
- Icon

같은 정적 성격을 가집니다.

하지만 같은 ItemID라도 어떤 상황에서 spawn됐는지는 다를 수 있습니다.

그래서 실제 runtime option은 별도 구조체입니다.

```cpp
USTRUCT(BlueprintType)
struct FItemSpawnOptions
{
    GENERATED_BODY()

    bool bRequireInteraction = false;
    EItemInteractionType InteractionType =
        EItemInteractionType::Instant;

    float HoldDuration = 1.0f;
    int32 ItemCount = 1;

    float AutoPickupDelay = 0.0f;

    bool bApplyPhysicsOnDrop = false;
    float DropForce = 600.0f;

    float LifeTime = 0.0f;
};
```

---

## Part 6. 같은 Item도 Spawn Context에 따라 다르다

예:

### Monster Drop
- Auto Pickup
- Physics
- Lifetime

### Player가 버린 Item
- 직접 Interaction 필요
- Hold
- 다시 주울 수 있음

이 차이를 ItemData 자체에 넣으면 같은 Item 종류가 spawn 상황 때문에 중복 정의됩니다.

---

## Part 7. 자주 쓰는 조합은 Preset으로 만든다

실제 helper:

```cpp
FItemSpawnOptions MakeDropped(...);
FItemSpawnOptions MakeInteractable(...);
```

Caller가 여러 bool/float 값을 매번 직접 맞추지 않고 의미가 드러나는 factory helper를 사용합니다.

---

## Part 8. 물리 기반 Interaction은 Damage Pipeline을 사용한다

나무를 도끼로 치는 것을 F키 Interaction으로 처리하지 않습니다.

```text
Melee Skill
 ↓
Collision
 ↓
ApplyDamage
 ↓
Resource::TakeDamage
 ↓
Harvest Progress
```

공격이라는 물리적 행위는 Combat system을 그대로 사용하고, Target이 Damage의 의미를 해석합니다.

---

## Part 9. Resource는 Damage를 Harvest로 해석한다

Resource Object는 `TakeDamage`를 override합니다.

Monster라면 HP 감소/Death로 이어지는 같은 entry가 Resource에서는:

- HP Segment 감소
- Resource Drop
- Harvest Feedback

으로 이어집니다.

공격자는 Target concrete type에 따라 “Harvest()”와 “Damage()”를 나누지 않습니다.

---

## Part 10. HP Segment 기반 Reward

큰 Damage가 여러 threshold를 한 번에 지나갈 수 있습니다.

따라서 “이번 hit에 threshold를 넘었는가” bool만 보지 않고:

```text
Previous Segment = 9
Current Segment  = 7

Lost = 2
→ Partial Reward ×2
```

처럼 손실 segment 수를 계산합니다.

---

## Part 11. Dungeon도 같은 Interaction Contract를 사용한다

- Portal
- Shortcut Switch
- Reward Chest

가 기존 interaction layer에 들어갑니다.

Dungeon Runtime은 “누가 Switch를 사용했다”는 event만 받고:

- Detection
- Prompt
- Hold
- Server interaction

은 일반 시스템을 재사용합니다.

---

## Trade-offs

### Interface contract가 커질 수 있다
Interaction 종류가 계속 늘면 모든 구현체가 사용하지 않는 함수도 생길 수 있어 역할 분리가 필요합니다.

### Resource가 Damage를 재사용한다
Combat pipeline 변경이 Resource에도 영향을 줄 수 있으므로 Target-specific response 경계를 유지해야 합니다.

### Spawn Options 조합
Option이 너무 늘어나면 잘못된 조합이 가능해지므로 preset/validation이 중요합니다.

---

## 이 문서 다음에 읽기

- Damage가 실제로 처리되는 방식 → [[10. Combat, Skill & Animation|10_Combat_Skill_Animation]]
- Container/Crafting interaction → [[8. Multiplayer Inventory & Crafting|8_Multiplayer_Inventory_Crafting]]
- Dungeon switch가 event로 연결되는 방식 → [[4. Branching Dungeon Runtime|4_Branching_Dungeon_Runtime]]

---

## 관련 코드

- [InteractableInterface.h](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/GameObject/InteractableInterface.h)
- [InteractionComponent](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Player/Utility/InteractionComponent.h)
- [BaseItem.h](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/GameObject/Items/BaseItem.h)
- [BaseResourceObject](https://github.com/chungheonLee0325/Sonheim/tree/main/Sonheim/Source/Sonheim/GameObject/ResourceObject)
