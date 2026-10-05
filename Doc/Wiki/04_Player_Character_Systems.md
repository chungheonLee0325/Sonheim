# 04. Player & Character Systems

Player의 지속 데이터는 **PlayerState**, 월드의 실제 body 상태는 **Pawn**, 독립 gameplay 기능은 **ActorComponent**가 소유합니다.

- Pawn — 이동, Mesh, Animation, 실제 World action
- PlayerState — Inventory, Pal Inventory, Stat처럼 Pawn 교체와 분리할 데이터
- ActorComponent — Health, Skill, Condition 등 독립적으로 변하는 기능
- PlayerController — Input, connection, Client UI bootstrap

이 구조 위에서 장비·Stat·Skill·Animation이 실제 gameplay로 연결됩니다.

---

## 전체 구조

~~~mermaid
flowchart LR
    INPUT["<b>PlayerController</b><br/>Input · UI Bootstrap"]
    PS["<b>PlayerState</b><br/>Inventory · Pal · Stat"]
    PAWN["<b>Player Pawn</b><br/>Movement · Mesh · Animation"]
    AREA["<b>AAreaObject Components</b><br/>Health · Stamina · Condition · Skill"]

    EQUIP["<b>Equipment</b>"]
    BONUS["<b>StatBonus</b>"]
    SKILL["<b>Skill Grant</b>"]

    INPUT --> PAWN
    PS --> PAWN
    PAWN --> AREA

    PS --> EQUIP
    EQUIP --> BONUS
    EQUIP --> SKILL
    BONUS -->|"Final Stat"| PAWN
    SKILL --> AREA
~~~

Inventory·Stat·Pal처럼 Player에 지속되는 상태와, Health/Movement/Animation처럼 현재 Pawn에 적용되는 상태를 서로 다른 lifecycle로 관리합니다.

---

## 1. PlayerState는 Player identity에 가까운 상태를 소유

\`ASonheimPlayerState\`는 다음 component를 생성합니다.

~~~cpp
ASonheimPlayerState::ASonheimPlayerState()
{
    m_InventoryComponent =
        CreateDefaultSubobject<UInventoryComponent>("Inventory");

    m_StatBonusComponent =
        CreateDefaultSubobject<UStatBonusComponent>("StatBonus");

    m_PalInventoryComponent =
        CreateDefaultSubobject<UPalInventoryComponent>("PalInventory");
}
~~~

Inventory·장비·Pal 보유 목록·Stat modifier는 현재 Pawn의 Mesh나 Animation보다 **Player의 지속 상태**에 가깝습니다.

Pawn이 다시 Possess되더라도 이런 데이터를 World body의 생성/파괴와 같은 수명에 묶지 않습니다.

---

## 2. Pawn은 계산된 결과를 실제 World 상태에 적용

PlayerState가 계산한 Stat을 Pawn이 실제 gameplay component에 반영합니다.

예:

~~~text
Equipment Item
   ↓
StatBonusComponent
   ↓
PlayerState Modified Stat
   ↓
ASonheimPlayer::StatChanged
   ├─ Max HP
   ├─ Attack / Defense
   ├─ Run Speed
   └─ Jump Height
~~~

실제 적용:

~~~cpp
void ASonheimPlayer::StatChanged(
    EAreaObjectStatType StatType,
    float StatValue)
{
    if (!HasAuthority())
        return;

    switch (StatType)
    {
    case EAreaObjectStatType::HP:
        m_HealthComponent->SetMaxHP(StatValue);
        break;

    case EAreaObjectStatType::Attack:
        m_Attack = StatValue;
        break;

    case EAreaObjectStatType::Defense:
        m_Defence = StatValue;
        break;

    case EAreaObjectStatType::RunSpeed:
        GetCharacterMovement()->MaxWalkSpeed = StatValue;
        break;

    case EAreaObjectStatType::JumpHeight:
        GetCharacterMovement()->JumpZVelocity = StatValue;
        break;
    }
}
~~~

**Stat source는 PlayerState에 유지하면서, 실제 결과는 Pawn의 Health/Movement/Combat 상태에 반영**합니다.

---

## 3. Stat Modifier는 값을 만든 Source를 추적

\`FStatModifier\`는 단순 +10 같은 값뿐 아니라 어디서 온 보너스인지 함께 저장합니다.

~~~cpp
struct FStatModifier
{
    EAreaObjectStatType StatType;
    float Value;
    EStatModifierType ModifierType;
    int SourceID;
};
~~~

예를 들어 같은 HP Stat에:

~~~text
Base HP
 + Armor Item
 + Accessory
 + Buff
~~~

가 함께 적용돼도 특정 Item을 해제할 때 해당 Source가 만든 modifier만 제거할 수 있습니다.

\`UStatBonusComponent::RemoveAllBonusesFromSource()\`가 이 경계를 사용합니다.

---

## 4. Weapon Slot은 Stat과 Skill을 함께 바꾼다

장비 변경은 Inventory UI의 slot 이동으로 끝나지 않습니다.

~~~text
Weapon Equip / Switch
      ↓
Inventory Component
      ├─ Equipped Slot 갱신
      ├─ StatBonus 등록/활성화
      ├─ Weapon Mesh / HUD 갱신
      └─ Skill Grant 교체
~~~

Stat 쪽은 현재 활성 Weapon Slot의 modifier를 적용하고, Skill 쪽은 \`ActiveWeaponGrantId\`와 \`ReplaceGrant()\`로 해당 무기가 제공한 Skill set을 교체합니다.

따라서 Weapon이 바뀌면 **외형·능력치·사용 가능한 공격**이 같은 equipment change에서 함께 갱신됩니다.

### Weapon / Skill Switch Runtime

https://github.com/user-attachments/assets/2079af91-4ad9-4f93-99e1-3e21efbca57f

*곡괭이와 Shotgun 장비 전환에 따라 Weapon Mesh/HUD와 실제 공격 방식이 함께 바뀌는 흐름.*

---

## 5. AAreaObject는 공통 Gameplay Facade

Player와 Monster가 공유하는 기능은 \`AAreaObject\` 아래 component로 분리합니다.

~~~text
AAreaObject
 ├─ HealthComponent
 ├─ StaminaComponent
 ├─ ConditionComponent
 ├─ LevelComponent
 ├─ SkillComponent
 └─ Move / Rotate Utility
~~~

외부에서는:

- \`DecreaseHP()\`
- \`AddCondition()\`
- \`CastSkill()\`

같은 AreaObject 수준 API를 사용할 수 있고, 실제 상태 책임은 해당 component가 가집니다.

Health/Skill/Condition의 상태는 각 Component가 소유하고, 외부에서는 AreaObject 수준 API를 통해 접근합니다.

---

## 6. Player Action State로 입력 가능 범위를 묶어 관리

Combat 중 행동 제한을 입력 함수마다 서로 다른 bool 조건으로 관리하지 않습니다.

~~~cpp
enum class EPlayerState : uint8
{
    NORMAL,
    ONLY_ROTATE,
    ACTION,
    CANACTION,
    DIE,
    GLIDING,
};
~~~

각 state는 별도 restriction을 가집니다.

~~~cpp
struct FActionRestrictions
{
    bool bCanLook = true;
    bool bCanMove = true;
    bool bCanRotate = true;
    bool bCanOnlyRotate = false;
    bool bCanAction = true;
};
~~~

예:

~~~text
NORMAL
→ 일반 이동 / Action 가능

ACTION
→ 새 Action 제한

CANACTION
→ 이동은 제한하지만 Combo / Dodge 가능

DIE
→ 이동 / Action 제한

GLIDING
→ 일반 이동 대신 Glider movement 사용
~~~

현재 Player state가 “무엇을 할 수 있는가”를 한 곳에서 결정합니다.

---

## 7. Animation이 Action State의 전환 시점도 결정

Combat Montage의 Notify는 공격 판정뿐 아니라 Player Action State도 전환합니다.

~~~text
ACTION
   ↓
Attack Window
   ↓
CANACTION
   ├─ Combo
   └─ Dodge
   ↓
NORMAL
~~~

C++ Timer와 Animation timing을 따로 맞추지 않고 **공격 motion과 cancel window를 같은 timeline에서 조정**합니다.

자세한 Skill/Notify 구조는 [[05. Combat, Skill & Animation|05_Combat_Skill_Animation]]에서 설명합니다.

---

## 8. Condition은 여러 Character 상태를 Bitmask로 표현

Dead / Invincible / Hidden처럼 동시에 조합될 수 있는 상태를 각각 독립 bool로 늘리지 않습니다.

~~~cpp
enum class EConditionBitsType : uint32
{
    None       = 0,
    Dead       = 1 << 0,
    Invincible = 1 << 1,
    Hidden     = 1 << 2,
};
~~~

예를 들어 Capture 중 Monster를 \`Hidden\` 상태로 두면:

- 공격 대상에서 제외
- Rendering / Collision / AI 비활성

같은 lifecycle과 함께 사용할 수 있습니다.

### Timed Condition 처리

Timed Condition은 Condition type을 key로 사용해 duration을 관리합니다. 같은 type의 재적용은 해당 Condition의 현재 timer를 기준으로 처리합니다.

---

## 9. Movement는 UE 기본 동기화 위에 프로젝트 상태만 추가

기본 위치/속도 이동은 CharacterMovement의 역할을 사용하고, 프로젝트 고유 상태만 별도로 관리합니다.

대표적으로:

- Sprint
- Glider
- Lock-on
- Current Weapon / Weapon Visibility

가 Client presentation에 영향을 줍니다.

위치·속도 동기화는 CharacterMovement를 사용하고, Sprint·Glider·Lock-on·Weapon visibility 같은 프로젝트 상태를 추가로 관리합니다.

자세한 동기화 방식은 [[12. Multiplayer Synchronization|12_Multiplayer_Synchronization]]에서 다룹니다.

---

## 10. PlayerState가 늦게 도착해도 Pawn 기능을 다시 연결

Remote Client에서는 Pawn이 생성되는 시점과 PlayerState가 유효해지는 시점이 같다고 가정하지 않습니다.

\`OnRep_PlayerState()\` 이후:

~~~text
PlayerState 확보
   ↓
PalCaptureComponent.InitializeWithPlayerState
   ↓
PalPartnerSkillComponent.InitializeWithPlayerState
   ↓
Player-owned component와 Pawn 다시 연결
~~~

처럼 PlayerState 기반 기능을 초기화합니다.

PlayerState / Pawn 수명을 나눈 만큼 **둘이 만나는 초기화 경계도 명시적으로 관리**합니다.

---

## 설계 선택과 비용

| 선택 | 얻은 것 | 비용 / 제약 |
|---|---|---|
| **PlayerState / Pawn 분리** | Inventory·Stat·Pal을 World body lifecycle과 분리 | Pawn 생성/복제 시 다시 연결하는 초기화 필요 |
| **ActorComponent 구성** | Health/Skill/Condition 기능 재사용 | component 간 orchestration 지점 필요 |
| **Source 기반 Stat Modifier** | 장비/Buff별 선택적 제거 | Source identity 관리 필요 |
| **Action State + Restriction** | Combat/Input 조건을 한 곳에서 관리 | 상태 종류가 늘면 transition 규칙 관리 필요 |
| **Condition Bitmask** | 조합 가능한 상태를 작은 값으로 표현 | 같은 condition의 source별 독립 duration에는 한계 |

---

## 연관 문서

- [[05. Combat, Skill & Animation|05_Combat_Skill_Animation]] — Action State / Skill Grant / Animation timing
- [[07. Inventory & Crafting|07_Inventory_Crafting]] — Equipment가 Stat/Skill Source가 되는 흐름
- [[08. Pal Capture & Partner Lifecycle|08_Pal_Capture_Partner_Lifecycle]] — PlayerState PalInventory와 Pawn Partner 기능 연결
- [[12. Multiplayer Synchronization|12_Multiplayer_Synchronization]] — Player-owned state의 동기화

---

## 관련 코드

- [SonheimPlayer.cpp](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Player/SonheimPlayer.cpp)
- [SonheimPlayerState.cpp](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Player/SonheimPlayerState.cpp)
- [AreaObject](https://github.com/chungheonLee0325/Sonheim/tree/main/Sonheim/Source/Sonheim/AreaObject)
- [StatBonusComponent.cpp](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Attribute/StatBonusComponent.cpp)
