# 11. Player & Character Systems

기존 AreaObject / Attribute / Player Control / Stat 문서를 **캐릭터 상태와 수명** 관점으로 통합합니다.

---

## AAreaObject

Player와 Monster가 공유하는 Character base로서 공통 component와 damage/death lifecycle의 진입점을 제공합니다.

공통 기능을 모두 `AAreaObject` 내부에 직접 구현하기보다 component로 조합합니다.

---

## Attribute Components

- `UHealthComponent`
- `UStaminaComponent`
- `ULevelComponent`
- `UConditionComponent`
- `UStatBonusComponent`

각 component는 자신의 상태와 delegate를 소유합니다.

예를 들어 Health 변화는 UI, AI, Dungeon Runtime이 같은 delegate를 구독할 수 있지만 Health component가 그 소비자를 직접 알 필요는 없습니다.

---

## Player Framework

Player는 Pawn / PlayerController / PlayerState의 UE 표준 수명과 네트워크 역할을 사용합니다.

- **Pawn**: 월드의 실제 조작 대상
- **PlayerController**: connection / input / request
- **PlayerState**: player identity와 지속 상태

Pawn respawn과 player data 수명을 분리할 수 있습니다.

---

## Character Control

`CharacterMovementComponent`를 기반으로 이동을 네트워크 동기화하고:

- Aim
- Sprint
- Dodge
- Glider
- Lock-on

등의 player action을 상태와 animation flow에 연결합니다.

---

## Stat / Equipment

장비와 bonus source에 따라 final stat을 계산합니다.

Equipment 변경은 stat만 바꾸는 것이 아니라 Skill Grant 등 다른 subsystem에도 영향을 줄 수 있으므로 source 단위 변경을 추적합니다.

---

## 관련 코드

- [AreaObject](https://github.com/chungheonLee0325/Sonheim/tree/main/Sonheim/Source/Sonheim/AreaObject)
- [Player](https://github.com/chungheonLee0325/Sonheim/tree/main/Sonheim/Source/Sonheim/AreaObject/Player)
- [Attribute](https://github.com/chungheonLee0325/Sonheim/tree/main/Sonheim/Source/Sonheim/AreaObject/Attribute)
