# 11. Player & Character Systems

> **핵심 구현 범위**
>
> “Player의 몸(Pawn), 연결(Controller), 지속 데이터(PlayerState), 그리고 Character 공통 능력은 어디에 두는가?”

이 문서는 전투나 Inventory보다 먼저 **객체 수명과 책임**을 설명합니다.

---

## Part 1. UE Gameplay Framework 기준으로 수명을 나눈다

| 객체 | Sonheim에서의 책임 |
|---|---|
| `ASonheimPlayer` | 이동, Mesh, Animation, World Action |
| `ASonheimPlayerController` | Input, Client RPC, UI bootstrap |
| `ASonheimPlayerState` | Inventory, Stat 등 Player identity에 가까운 data |
| `AAreaObject` | Player / Monster 공통 gameplay facade |

Pawn이 교체될 수 있는 데이터와 World body에 종속된 데이터를 분리합니다.

---

## Part 2. AAreaObject는 공통 Facade다

`AAreaObject`는 여러 component를 조합합니다.

```text
AAreaObject
 ├─ Health
 ├─ Stamina
 ├─ Condition
 ├─ Level
 ├─ Skill
 ├─ Move Utility
 └─ Rotate Utility
```

외부에서는:

- `DecreaseHP`
- `AddCondition`
- `CastSkill`

같은 AreaObject API를 사용하고 실제 state owner는 component입니다.

---

## Part 3. Player Action State

실제 Player state enum:

```cpp
UENUM(BlueprintType)
enum class EPlayerState : uint8
{
    NORMAL,
    ONLY_ROTATE,
    ACTION,
    CANACTION,
    DIE,
    GLIDING,
};
```

각 상태가 허용하는 행동은 별도 구조체로 표현합니다.

```cpp
USTRUCT(BlueprintType)
struct FActionRestrictions
{
    GENERATED_BODY()

    bool bCanLook = true;
    bool bCanMove = true;
    bool bCanRotate = true;
    bool bCanOnlyRotate = false;
    bool bCanAction = true;
};
```

입력 함수마다 `if (bAttacking && !bCanDodge ...)`를 흩뿌리지 않고 현재 state의 restriction을 조회합니다.

---

## Part 4. Animation과 Action State가 연결된다

Combat Montage의 Notify가:

```text
ACTION
 → CANACTION
 → NORMAL
```

을 변경합니다.

그래서:

- 선딜
- Combo 가능
- Dodge cancel
- 이동 복귀

시점을 animation timeline에서 조정할 수 있습니다.

이 흐름은 [[10. Combat, Skill & Animation|10_Combat_Skill_Animation]]과 연결됩니다.

---

## Part 5. Health / Stamina / Condition

### Health

Server 상태가 RepNotify로 Client에 전달되고 Delegate가 UI를 갱신합니다.

```text
Server HP
 → Replication
 → OnRep
 → OnHealthChanged
```

### Condition

Dead / Invincible / Hidden은 bitmask입니다.

```cpp
enum class EConditionBitsType : uint32
{
    None       = 0,
    Dead       = 1 << 0,
    Invincible = 1 << 1,
    Hidden     = 1 << 2,
};
```

여러 bool property를 각각 늘리지 않고 bit operation으로 조합합니다.

---

## Part 6. Stat Bonus는 Source를 추적한다

실제 modifier:

```cpp
USTRUCT(BlueprintType)
struct FStatModifier
{
    GENERATED_BODY()

    EAreaObjectStatType StatType;
    float Value;
    EStatModifierType ModifierType;
    int SourceID;
};
```

`SourceID`가 있기 때문에:

- Item A가 준 HP
- Buff B가 준 HP
- Weapon C가 준 Attack

을 같은 stat에 더하면서도 특정 source만 제거할 수 있습니다.

---

## Part 7. 왜 Stat을 PlayerState 쪽에 두는가

장비/성장 데이터는 Pawn mesh보다 Player identity에 가깝습니다.

따라서 PlayerState 쪽에서 modifier를 계산하고 Pawn의 runtime component에 결과를 반영합니다.

예:

```text
Equipment Change
  ↓
StatBonusComponent
  ↓
Final MaxHP
  ↓
Pawn HealthComponent
```

---

## Part 8. Equipment는 여러 시스템을 연결한다

무기 하나를 장착하면:

```text
Inventory
  ├─ Equipped Slot
  ├─ Stat Modifier
  ├─ Mesh
  └─ Skill Grant
```

이 흐름 때문에 Inventory / Stat / Skill은 서로 완전히 독립된 섬이 아니라 명확한 orchestration point를 갖습니다.

---

## Part 9. Movement 관련 Custom Replication

CharacterMovement가 기본 이동 동기화를 담당하고 프로젝트 고유 state만 별도로 복제합니다.

예:

- `bIsSprinting`
- `bIsGliding`
- `bIsLockOn`
- Current Weapon
- Weapon Visibility

RepNotify에서 Client presentation을 적용합니다.

---

## Part 10. HUD 초기화도 Lifecycle 문제다

Remote Client에서 Controller와 PlayerState의 Replication 순서는 고정되지 않습니다.

따라서:

```text
OnRep_Controller ─┐
                  ├→ Init Gate
OnRep_PlayerState ┘
```

구조를 사용합니다.

Host는 Server `PossessedBy`에서 Client RPC로 초기화합니다.

자세한 UI 흐름은 [[5. Multiplayer State & UI Pipeline|5_Multiplayer_State_UI_Pipeline]]에서 설명합니다.

---

## Trade-offs

### Condition Timer
현재 같은 Condition type을 서로 다른 source가 독립 duration으로 중첩하는 데 제한이 있습니다.

### PlayerState 집중
지속 데이터에는 적합하지만 모든 gameplay state를 PlayerState에 넣으면 World/Pawn 책임이 흐려질 수 있어 movement/animation은 Pawn에 유지합니다.

---

## 연관 문서

- Skill/Combat 실행 → [[10. Combat, Skill & Animation|10_Combat_Skill_Animation]]
- Inventory/Equipment → [[8. Multiplayer Inventory & Crafting|8_Multiplayer_Inventory_Crafting]]
- Pal ownership → [[9. Pal Capture & Partner Lifecycle|9_Pal_Capture_Partner_Lifecycle]]

---

## 관련 코드

- [SonheimPlayer.h](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Player/SonheimPlayer.h)
- [SonheimPlayerState.h](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Player/SonheimPlayerState.h)
- [AreaObject.h](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Base/AreaObject.h)
- [StatBonusComponent.h](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Attribute/StatBonusComponent.h)
