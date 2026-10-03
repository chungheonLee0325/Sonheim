# 5. Multiplayer State & UI Pipeline

> **Gameplay code가 Widget을 직접 조작하지 않으면서, 네트워크로 도착하는 상태를 UI lifecycle에 맞게 안정적으로 표현하는 것이 목표입니다.**

Sonheim의 UI는 한 가지 패턴으로만 구성되지 않습니다.  
Health처럼 독립 값의 변경은 **Delegate**, Dungeon처럼 여러 값이 하나의 콘텐츠 상태를 만드는 경우는 **Replicated Snapshot + Presenter**, 순간적인 메시지는 **Notice**로 분리합니다.

---

## 1. 일반 HUD: Delegate 기반 Publish / Subscribe

Health, Inventory, Equipment 등은 이미 소유 object가 명확하고 “값이 바뀌었다”는 사실이 중요합니다.

예를 들어 Health:

```text
Server HP 변경
   ↓ Replication
UHealthComponent::OnRep_HP
   ↓
OnHealthChanged.Broadcast
   ↓
HUD Widget Update
```

Widget이 `UHealthComponent` 내부 계산을 알 필요는 없고, gameplay component도 Widget class를 알 필요가 없습니다.

### PlayerController는 Binding Mediator

Player HUD는 `ASonheimPlayerController`가 Widget lifecycle과 event binding을 조율합니다.

이 역할을 한 곳에 두어:

- Widget 생성
- Pawn / PlayerState reference 연결
- Delegate bind
- UI teardown

이 여러 위치에 흩어지지 않도록 했습니다.

---

## 2. Listen Server / Remote Client 초기화 순서 문제

멀티플레이에서 Pawn, Controller, PlayerState가 Client에서 유효해지는 순서는 항상 같지 않습니다.

특히:

- Host: `PossessedBy`는 server path
- Remote Client: Controller / PlayerState는 replication으로 도착

따라서 HUD 초기화를 BeginPlay 한 지점에만 두면 race가 생길 수 있습니다.

Remote Client에서는:

```text
OnRep_Controller ─┐
                  ├─ TryInitHUD_OnClient()
OnRep_PlayerState ┘
```

처럼 두 replication callback이 동일한 gate를 호출하고, 필요한 reference가 모두 준비됐을 때만 한 번 초기화합니다.

Host는 `PossessedBy → Client RPC` 경로를 사용합니다.

핵심은 **어느 callback이 먼저 오느냐에 의존하지 않는 idempotent initialization**입니다.

---

## 3. Dungeon UI: Snapshot 기반 Presentation

Dungeon은 HP 하나처럼 개별 event만 구독하기에는 상태의 결합도가 높습니다.

한 화면이 동시에 사용합니다.

- Stage / Route
- Objective progress
- Branch
- Deadline
- Party
- Boss health/action/phase/break
- Reward / Result
- Best time / Grade
- Marker / Minimap

이런 경우 “무슨 event가 방금 발생했는가”보다 **현재 Run 전체가 어떤 상태인가**가 중요합니다.

```text
UDungeonStageRuntimeSubsystem
        ↓ publish
FDungeonStageRuntimeState
        ↓ Replication
ASonheimGameState
        ↓
UDungeonStagePresenter
        ↓
FDungeonStageViewData
        ↓
UDungeonUIRouterSubsystem
        ↓
UDungeonViewWidget
```

---

## 4. Replicated Snapshot

`FDungeonStageRuntimeState`는 UI 전용 구조체가 아니라 Client가 알아야 할 authoritative run snapshot입니다.

대표 항목:

- RunId / DefinitionAssetId
- StageId / Revision / RunStatus
- Objective group progress
- SelectedBranchId / RunTags
- Deadline
- SealedBarriers
- Rewards
- Defeated / Captured
- Participants / Owner
- Elapsed / Start Best
- Boss status

### Revision

같은 Stage 안에서도 state가 바뀔 수 있기 때문에 StageId만으로 변경 여부를 판단하지 않고 Revision을 함께 사용합니다.

---

## 5. Presenter: Runtime State를 UI 언어로 변환

`UDungeonStagePresenter`는 Snapshot을 그대로 Widget에 넘기지 않습니다.

Definition / Presentation asset과 조합하여 `FDungeonStageViewData`를 만듭니다.

예:

| Runtime | ViewData |
|---|---|
| GameplayTag StageId | 화면 제목 / Route Step |
| Group progress | Objective label / count |
| Server deadline | Countdown display source |
| Boss PatternId | Boss action text / icon |
| Reward ItemId | 이름 / 수량 / icon |
| World SourceId | Marker location |

Widget은 GameplayTag tree, Stage transition, Item lookup을 직접 해석하지 않습니다.

---

## 6. LocalPlayer UI Router

`UDungeonUIRouterSubsystem : ULocalPlayerSubsystem`은 LocalPlayer 기준으로 UI lifecycle을 관리합니다.

- participant인 경우만 Dungeon HUD 표시
- Run 시작 시 충돌하는 기존 화면 hide
- Run 종료 시 이전 visibility 복원
- 최신 ViewData cache
- HUD / Result route
- Notice channel 정리

UI ownership을 World singleton이 아니라 LocalPlayer에 둬서 “어느 화면에 보여야 하는가”를 gameplay runtime에서 분리했습니다.

---

## 7. 서버 시각을 화면 기준으로 사용

Countdown과 Boss action progress는 Client가 “받은 순간부터 N초”를 세지 않습니다.

Snapshot에 서버 기준 start/deadline을 포함하고 Client는 synchronized server time으로 남은 값을 계산합니다.

이 방식은:

- Network delay
- HUD recreation
- Frame rate 차이

때문에 UI timer의 기준이 달라지는 문제를 줄입니다.

---

## 8. Snapshot 밖의 변화는 Delegate로 보완

모든 화면 값을 Snapshot에 집어넣는 것도 효율적이지 않습니다.

예를 들어 Participant HP처럼 별도 replicated component가 이미 있는 값은 Presenter가 해당 delegate를 따라가며 ViewData를 갱신할 수 있습니다.

즉:

- Dungeon progression → Snapshot
- 독립 actor property → 기존 replication/delegate

로 책임을 중복시키지 않습니다.

---

## 9. Notice: 지속 상태가 아닌 메시지

Room title, Level-up, Capture failure, Crafting complete 같은 메시지는 “현재 게임 상태”가 아니라 **일시적으로 표시할 presentation event**입니다.

`UNoticeSubsystem`은 다음 정책을 공통화합니다.

- Banner / Title slot
- TakeTurns / ReplaceShowing
- producer Channel
- Widget / Style / Z-order config

Channel을 두어 Dungeon이 끝날 때 Dungeon이 만든 대기 메시지만 정리할 수 있습니다.

---

## 10. UI 성능: 사용 패턴에 맞는 Pooling

모든 UI를 하나의 pool에 넣지 않습니다.

### Floating Damage

전투 중 짧은 수명의 damage actor/widget가 반복 생성되므로 전역 pool을 사용합니다.

### Crafting Required Row

Recipe detail 내부의 row는 같은 screen 안에서 반복 재사용되므로 Widget 내부 local pool을 사용합니다.

- 필요한 개수까지 한 번 생성
- 남는 row는 `Collapsed`
- Recipe가 바뀔 때 icon/layout 갱신
- Inventory/수량 변화에는 숫자와 상태만 갱신

Pooling 범위를 실제 사용 수명에 맞춥니다.

---

## Trade-offs

### Presenter/ViewData는 코드량을 늘린다
단순 HUD에는 과한 구조입니다. Dungeon처럼 presentation logic과 상태 조합이 복잡한 화면에만 사용합니다.

### Delegate는 초기 binding이 중요하다
Publisher보다 Widget이 늦게 생성될 수 있으므로 최초 상태를 별도로 읽어 초기화해야 합니다.

### Snapshot은 transient event를 대신하지 않는다
Toast 같은 일회성 presentation까지 Snapshot에 넣으면 stale event 처리 문제가 생기므로 Notice와 분리했습니다.

---

## 관련 코드

- [SonheimPlayerController](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Player/SonheimPlayerController.h)
- [DungeonStagePresenter](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/UI/Dungeon/DungeonStagePresenter.h)
- [DungeonViewData](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/UI/Dungeon/DungeonViewData.h)
- [DungeonUIRouterSubsystem](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/UI/Dungeon/DungeonUIRouterSubsystem.h)
- [NoticeSubsystem](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/UI/Notice/NoticeSubsystem.h)
- [FloatingDamagePool](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/UI/FloatingDamagePool.h)
