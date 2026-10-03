# 11. Player & Character Systems

> **Player의 World body, 연결, 지속 데이터와 Character 공통 능력치를 UE Gameplay Framework의 수명에 맞춰 분리합니다.**

---

## 1. AAreaObject — Player와 Monster의 공통 Gameplay Facade

`AAreaObject`는 모든 기능을 직접 구현하는 God Class가 아니라, Character 공통 component를 조합하고 외부에 일관된 API를 제공합니다.

대표 component:

- `UHealthComponent`
- `UStaminaComponent`
- `UConditionComponent`
- `ULevelComponent`
- `USonheimSkillComponent`
- Move / Rotate Utility

외부 시스템은 `DecreaseHP`, `CastSkill`, `AddCondition` 같은 AreaObject 수준의 API를 사용하고 실제 state mutation은 해당 component가 담당합니다.

---

## 2. Health — RepNotify + Delegate

Health 값은 replicated state이고 변화는 delegate로 알립니다.

```text
Server ModifyHP
     ↓
Replicated HP
     ↓
OnRep_HP
     ↓
OnHealthChanged
     ├─ HUD
     ├─ Dungeon Presenter
     └─ 기타 Subscriber
```

Health component는 누가 이 값을 화면에 그리는지 알지 않습니다.

---

## 3. Condition — Bitmask 기반 상태 조합

Dead / Invincible / Hidden 등 여러 boolean condition을 각각 bool property로 늘리지 않고 `uint32 ConditionFlags` bitmask로 관리합니다.

- Add → OR
- Remove → AND + NOT
- Query → bit check

상태 종류가 늘어나도 동일한 storage와 API를 사용합니다.

### Timed Condition의 한계

현재 timer map은 Condition type 중심이므로 동일 Condition을 여러 source가 서로 다른 duration으로 중첩하는 요구에는 제약이 있습니다.

Source별 독립 수명이 필요해지면 GUID/token 기반 instance 추적이 더 적합합니다.

---

## 4. Level

`ULevelComponent`는 Level table을 이용해:

- Current Level
- EXP
- Next Level requirement
- Level-up

을 관리하고 변경 이벤트를 외부에 알립니다.

Level-up presentation은 gameplay component 내부에서 Widget을 직접 만들지 않고 Notice 계층으로 전달합니다.

---

## 5. Pawn / Controller / PlayerState

Player data를 한 Actor에 모두 넣지 않습니다.

| 객체 | 책임 |
|---|---|
| `ASonheimPlayer` | 이동, Mesh, Animation, World interaction |
| `ASonheimPlayerController` | Input/Connection, Client RPC, UI bootstrap |
| `ASonheimPlayerState` | Inventory, Stat처럼 Pawn 교체와 분리할 Player data |

Pawn이 죽고 새로 생성될 수 있어도 Player identity에 가까운 데이터는 PlayerState lifecycle에 유지할 수 있습니다.

---

## 6. PlayerState 중심 Stat Bonus

`UStatBonusComponent`는 base stat 자체를 소유하기보다 여러 source의 modifier를 모아 최종값을 계산합니다.

`FStatModifier`:

- StatType
- Value
- Additive / Multiplicative / Override
- SourceID

### Source 추적

Equipment, Item, Buff처럼 modifier를 만든 source를 ID로 추적해 특정 source의 bonus만 제거할 수 있습니다.

### 결과 전파

Stat이 바뀌면 Pawn의 실제 runtime component에 반영합니다.

예:

- Max HP → HealthComponent
- 이동 속도 → CharacterMovement
- 기타 Player runtime stat

PlayerState가 계산의 중심이고 Pawn은 World에서 그 결과를 적용합니다.

---

## 7. Equipment와 Stat / Skill 연결

Inventory에서 장비가 바뀌면 단순히 EquippedSlots만 변경하지 않습니다.

- StatBonus 등록/해제
- Current Weapon 상태
- Weapon Mesh
- Skill Grant

까지 이어집니다.

따라서 Equipment는 Inventory, Stat, Skill 사이의 orchestration point 역할을 합니다.

---

## 8. Action Restriction State

Player action 가능 여부를 입력 함수마다 제각각 bool로 관리하지 않고 `EPlayerState`와 `FActionRestrictions`에 모읍니다.

상태 예:

- NORMAL
- ONLY_ROTATE
- ACTION
- CANACTION
- DIE
- GLIDING

각 상태는:

- Look
- Move
- Rotate
- Action

허용 여부를 가집니다.

Animation Notify가 ACTION → CANACTION → NORMAL을 변경하면서 combat cancel window를 조정합니다.

---

## 9. Aim / Camera 전환은 보간한다

Lock-on/aim 전환에서 Camera Boom, 회전 상태를 즉시 snap시키지 않고 `FInterpTo`, rotation interpolation을 사용합니다.

Gameplay state는 discrete하게 바뀌더라도 camera presentation은 시간에 따라 부드럽게 따라가도록 분리합니다.

---

## 10. Movement State Replication

Sprint / Glider / LockOn처럼 다른 Client의 presentation에도 필요한 custom state는 RepNotify를 사용합니다.

예:

- `bIsSprinting`
- `bIsGliding`
- `bIsLockOn`
- selected weapon / visibility

CharacterMovement가 기본 movement replication을 담당하고, 프로젝트 고유 state만 별도 replicated property로 보완합니다.

---

## 11. HUD Initialization도 Character Lifecycle 문제다

Remote Client에서 Controller와 PlayerState replication 순서는 고정되지 않습니다.

Player는 `OnRep_Controller`, `OnRep_PlayerState`에서 동일한 initialization gate를 호출하고 준비 조건이 맞을 때 HUD setup을 진행합니다.

이 내용은 [[Multiplayer State & UI Pipeline|5_Multiplayer_State_UI_Pipeline]]에서 자세히 설명합니다.

---

## 관련 코드

- [AreaObject](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Base/AreaObject.h)
- [Attributes](https://github.com/chungheonLee0325/Sonheim/tree/main/Sonheim/Source/Sonheim/AreaObject/Attribute)
- [SonheimPlayer](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Player/SonheimPlayer.h)
- [SonheimPlayerState](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Player/SonheimPlayerState.h)
- [StatBonusComponent](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Attribute/StatBonusComponent.h)
