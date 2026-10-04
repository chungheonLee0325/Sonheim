# 06. World Interaction Systems

Item, Container, Crafting Station, Dungeon Lever는 **<code>IInteractableInterface</code> 기반의 공통 Detection / Prompt / Hold / 실행 흐름**을 사용합니다.

Detection·Prompt·Hold·Server 요청은 \`UInteractionComponent\`가 공통으로 처리하고, 실제 결과는 \`IInteractableInterface\`를 구현한 Actor가 결정합니다.

---

## 전체 구조

~~~mermaid
flowchart LR
    INPUT["<b>Player Input</b>"]
    COMP["<b>Interaction Component</b><br/>Detection · Hold · Request"]
    API["<b>IInteractableInterface</b><br/>공통 Contract"]

    ITEM["<b>Item</b><br/>획득"]
    BOX["<b>Container</b><br/>보관함 열기"]
    CRAFT["<b>Crafting Station</b><br/>Recipe · Work · Collect"]
    LEVER["<b>Dungeon Lever</b><br/>Branch Event"]
    PORTAL["<b>Dungeon Entrance</b><br/>Run Start"]

    UI["<b>Context UI</b><br/>Prompt · Hold Progress · Cancel"]

    INPUT --> COMP
    COMP --> API
    API --> ITEM
    API --> BOX
    API --> CRAFT
    API --> LEVER
    API --> PORTAL

    API --> UI
~~~

Player는 “현재 대상이 Item인가 Lever인가”를 판단하지 않습니다.  
대상이 **무엇을 보여주고, 얼마나 눌러야 하며, 실행되면 무엇을 할지**를 contract로 제공합니다.

---

## 1. Interaction Contract

핵심 interface는 다음 책임을 제공합니다.

~~~cpp
class IInteractableInterface
{
public:
    bool CanInteract() const;
    void OnDetected(bool bDetected);

    void Interact(ASonheimPlayer* Player);

    FString GetInteractionName() const;
    float GetHoldDuration() const;

    void UpdateHoldProgressUI(
        float Progress,
        EHoldPurpose Purpose);

    bool CanHoldCancel() const;
    void ExecuteCancel(ASonheimPlayer* Player);

    int32 GetInteractionModeCode() const;
};
~~~

각 함수는 서로 다른 목적을 가집니다.

| Contract | 역할 |
|---|---|
| \`CanInteract\` | 현재 상태에서 실행 가능한지 |
| \`OnDetected\` | 탐지/해제 시 highlight·prompt 반응 |
| \`GetInteractionName\` | 대상이 UI에 표시할 이름/행동 |
| \`GetHoldDuration\` | 즉시 실행인지 Hold인지 |
| \`UpdateHoldProgressUI\` | 대상 UI의 진행 표시 |
| \`Interact\` | 실제 gameplay 동작 |
| \`CanHoldCancel / ExecuteCancel\` | 취소 가능한 작업 처리 |
| \`GetInteractionModeCode\` | Hold 중 대상 상태가 바뀌었는지 감지 |

입력 처리와 대상별 gameplay 로직을 같은 class에 넣지 않습니다.

---

## 2. Player 쪽 흐름은 대상 종류와 무관하다

\`UInteractionComponent::TryInteract()\`는 concrete class를 검사하지 않고 interface를 호출합니다.

~~~cpp
void UInteractionComponent::TryInteract()
{
    if (GetOwnerRole() != ROLE_Authority)
    {
        Server_TryInteract(CurrentInteractable);
        return;
    }

    if (!CurrentInteractable ||
        !IInteractableInterface::Execute_CanInteract(CurrentInteractable))
        return;

    IInteractableInterface::Execute_Interact(
        CurrentInteractable,
        OwnerPlayer);
}
~~~

실행 흐름:

~~~text
Detect Actor
   ↓
Implements IInteractableInterface?
   ↓
CanInteract
   ↓
Prompt / Hold
   ↓
Player Input
   ↓
Server Request
   ↓
IInteractableInterface::Execute_Interact
   ↓
대상 Actor의 Interact_Implementation
~~~

새 상호작용 Actor를 추가해도 Player 쪽에 class별 \`if / else\`를 추가할 필요가 없습니다.

---

## 3. 같은 Contract가 서로 다른 Gameplay를 실행

### Item — 획득

\`ABaseItem::Interact_Implementation()\`:

~~~cpp
void ABaseItem::Interact_Implementation(ASonheimPlayer* Player)
{
    if (CanInteract_Implementation() &&
        CanBeCollectedBy(Player))
    {
        OnCollected(Player);
        Multicast_OnCollected();
    }
}
~~~

Interaction 결과는 Item 획득과 Inventory 보상으로 이어집니다.

### Container — 보관함 열기

\`ABaseContainer\`는 현재 다른 Player가 사용 중인지 확인하고, 상호작용하면 Container UI를 엽니다.

~~~cpp
bool ABaseContainer::CanInteract_Implementation() const
{
    return !bIsOpen || CurrentUser == nullptr;
}

void ABaseContainer::Interact_Implementation(
    ASonheimPlayer* Player)
{
    if (!CanInteract_Implementation() || !Player)
        return;

    OpenContainer(Player);
}
~~~

### Crafting Station — 현재 작업 상태에 따라 기능 변경

같은 Station도 상태에 따라 Prompt와 실제 결과가 달라집니다.

~~~text
Idle
→ "레시피 선택"
→ Recipe UI Open

Work 진행 중
→ "제작"
→ Work 추가

완료 결과 존재
→ "취득"
→ Collect
~~~

\`IInteractableInterface\`가 “Actor 종류”뿐 아니라 **같은 Actor의 상태 변화**도 공통 입력 흐름 안에서 처리합니다.

### Dungeon Lever — 콘텐츠 Event 발생

Lever 자체는 다음 Stage를 결정하지 않습니다.

~~~cpp
void ADungeonShortcutSwitch::Interact_Implementation(
    ASonheimPlayer* Player)
{
    if (HasAuthority())
        GetWorld()
            ->GetSubsystem<UDungeonStageRuntimeSubsystem>()
            ->TryInteractSwitch(
                this, Player, TestArea, SourceId);
}
~~~

Lever는 자신의 \`SourceId\`와 상호작용 사실만 Dungeon Runtime에 전달하고, 실제 Branch 규칙은 Dungeon Definition이 결정합니다.

### Dungeon Entrance — Run 시작

Dungeon Entrance도 같은 interface를 사용합니다.

~~~cpp
void ADungeonTestArea::Interact_Implementation(
    ASonheimPlayer* Player)
{
    if (HasAuthority())
        GetWorld()
            ->GetSubsystem<UDungeonStageRuntimeSubsystem>()
            ->TryStart(this, Player);
}
~~~

Interaction 계층은 Dungeon의 내부 진행 규칙을 알지 않습니다.

---

## 4. Context UI도 같은 Contract에서 정보를 받는다

Prompt Widget이 대상 class를 보고 문구를 선택하지 않습니다.

~~~text
Item
→ Item Name

Container
→ Container Name + "열기"

Crafting Station
→ "레시피 선택" / "제작" / "취득"

Shortcut Lever
→ Owner 여부 / 현재 Stage에 맞는 문구

Dungeon Entrance
→ 요구 Level / Run 상태에 맞는 문구
~~~

대상이 \`GetInteractionName()\`, \`CanInteract()\`, \`GetHoldDuration()\` 등 필요한 정보를 제공합니다.

따라서 새 Actor가 추가될 때 **UI Widget에 대상 class 분기문을 추가하는 대신 대상이 자신의 interaction context를 구현**합니다.

---

## 5. Instant / Hold / Cancel을 같은 입력 흐름에서 처리

대상에 따라 즉시 실행하거나 일정 시간 Hold할 수 있습니다.

~~~text
GetHoldDuration() <= 0
→ 즉시 Interact

GetHoldDuration() > 0
→ Hold Progress
→ 완료 시 Interact
~~~

Crafting처럼 진행 중 작업을 취소할 수 있는 대상은:

- \`CanHoldCancel()\`
- \`GetCancelHoldDuration()\`
- \`ExecuteCancel()\`

을 통해 같은 component에서 Cancel Hold까지 처리합니다.

Interaction UI의 progress도 대상이 \`UpdateHoldProgressUI()\`로 반영합니다.

---

## 6. Hold 중 대상의 의미가 바뀌면 자동으로 중단

Crafting Station처럼 interaction 의미가 상태에 따라 바뀌는 Actor에서는 Hold 도중:

~~~text
"제작"
   ↓ 작업 완료
"취득"
~~~

처럼 action mode가 바뀔 수 있습니다.

Hold를 시작할 때 \`GetInteractionModeCode()\`를 저장하고, 진행 중 mode가 달라지면 현재 Hold를 중단합니다.

~~~cpp
HoldInitialModeCode =
    IInteractableInterface::Execute_GetInteractionModeCode(
        CurrentInteractable);

...

if (CurrentMode != HoldInitialModeCode)
{
    StopHoldInteraction(Purpose);
    return;
}
~~~

상태가 “제작”에서 “수령”으로 바뀌었는데 같은 키 입력이 자동으로 다음 행동까지 연쇄 실행되는 것을 막습니다.

---

## 7. Detection과 실제 실행을 분리

\`OnDetected()\`는 gameplay 결과를 만들지 않습니다.

주요 역할:

- Prompt 표시/숨김
- Highlight
- Hold progress 초기화
- 현재 state에 맞는 문구 갱신

실제 gameplay 결과는 Authority에서 \`Interact()\`를 실행할 때만 발생합니다.

~~~text
Detection
→ Local Feedback

Interaction
→ Server-authoritative Gameplay
~~~

탐지 UX와 상태 변경 책임을 분리합니다.

---

## 8. Item은 정적 정의와 Spawn Context도 분리

Item 자체의 정보와 “이번에 어떻게 월드에 등장했는가”는 다른 문제입니다.

\`FItemData\`:

- 이름
- Item Category
- Stack
- Equipment
- Mesh / Icon

\`FItemSpawnOptions\`:

~~~cpp
struct FItemSpawnOptions
{
    bool bRequireInteraction = false;
    EItemInteractionType InteractionType;

    float HoldDuration = 1.f;
    int32 ItemCount = 1;

    float AutoPickupDelay = 0.f;

    bool bApplyPhysicsOnDrop = false;
    float DropForce = 600.f;

    float LifeTime = 0.f;
};
~~~

같은 ItemID라도:

- Monster Drop → Auto Pickup / Physics / Lifetime
- Player Drop → Interaction / Hold

처럼 다른 runtime context를 가질 수 있습니다.

자주 쓰는 조합은 \`MakeDropped()\`, \`MakeInteractable()\` 같은 helper로 제공합니다.

---

## 9. Dungeon 확장에서 기존 Interaction 계층을 그대로 재사용

초기 Item / Container / Crafting에 사용하던 Interaction 구조가 이후 Dungeon에서도:

- Entrance
- Shortcut Lever
- Reward Chest

로 확장됐습니다.

Dungeon 기능을 추가하면서 Player Input / Detection / Prompt 체계를 새로 만들지 않았다는 점이 이 interface 경계의 실제 재사용 사례입니다.

---

## 보조 사례 — 공격 기반 Resource 상호작용

Resource 채집은 명시적 Interaction 입력이 아니라 실제 무기 Hit으로 발생하므로 Combat/Damage Pipeline을 사용합니다.

~~~text
Melee Skill
 → Collision
 → Damage
 → Resource::TakeDamage
 → Resource Drop
~~~

Player 관점에서는 모두 World와의 상호작용이지만, **명시적인 사용 의도는 Interaction Interface**, **물리적 Hit은 Combat/Damage Pipeline**에서 처리합니다.

이 구분은 Interaction contract의 범위를 필요 이상으로 넓히지 않기 위한 선택입니다.

---

## 설계 선택과 비용

| 선택 | 얻은 것 | 비용 / 제약 |
|---|---|---|
| **Interface 기반 실행** | Player가 concrete Actor 종류를 모르고 동일 입력 흐름 사용 | Interface contract가 커지면 역할 재분리 필요 |
| **Prompt / Hold 정보도 Target이 제공** | 새 Actor 추가 시 UI type branch 감소 | Target이 presentation context 일부를 제공 |
| **InteractionModeCode** | 상태 변경 중 Hold의 잘못된 자동 연쇄 방지 | 각 stateful Actor가 mode 변화를 정의해야 함 |
| **Detection / Execution 분리** | Local feedback과 authoritative 결과 분리 | 두 lifecycle을 함께 관리해야 함 |
| **Resource는 Damage Pipeline 유지** | Interaction Interface가 물리 Hit 책임까지 떠안지 않음 | World interaction이 기술적으로 두 경로로 나뉨 |

---

## 연관 문서

- [[07. Inventory & Crafting|07_Inventory_Crafting]] — Container / Crafting Station의 실제 상태 변화
- [[09. Branching Dungeon Runtime|09_Branching_Dungeon_Runtime]] — Lever / Entrance가 전달한 Event 이후 콘텐츠 진행
- [[12. Multiplayer Synchronization|12_Multiplayer_Synchronization]] — Interaction 요청의 Server 처리
- [[05. Combat, Skill & Animation|05_Combat_Skill_Animation]] — Resource 채집이 재사용하는 Damage Pipeline

---

## 관련 코드

- [InteractableInterface.h](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/GameObject/InteractableInterface.h)
- [InteractionComponent.cpp](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Player/Utility/InteractionComponent.cpp)
- [BaseItem.cpp](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/GameObject/Items/BaseItem.cpp)
- [BaseContainer.cpp](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/GameObject/Buildings/Storage/BaseContainer.cpp)
- [CraftingStation.cpp](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/GameObject/Buildings/Crafting/CraftingStation.cpp)
- [DungeonShortcutSwitch.cpp](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/GameObject/Dungeon/DungeonShortcutSwitch.cpp)
