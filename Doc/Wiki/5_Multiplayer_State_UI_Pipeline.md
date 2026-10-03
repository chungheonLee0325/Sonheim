# 5. Case Study — Multiplayer State & UI Pipeline

> **문제:** 서버에서 진행되는 복잡한 콘텐츠 상태를 UMG 수명과 분리하면서 Listen Server와 Client 모두 동일하게 표현하려면?

Dungeon UI는 Widget이 gameplay state를 직접 읽거나 서버 runtime에 붙지 않습니다.

---

## Pipeline

```text
UDungeonStageRuntimeSubsystem
        ↓
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
UDungeonViewWidget / UMG
```

---

## Replicated Snapshot

Snapshot에는 단순 StageId만이 아니라 현재 화면을 재구성하는 run state가 포함됩니다.

- RunId / DefinitionAssetId
- StageId / Revision / RunStatus
- Objective group progress
- Selected Branch / RunTags
- Stage deadline
- Sealed Barriers
- Rewards
- Defeated / Captured
- Participants / Owner
- Elapsed / Best Record
- Boss health / action / phase / vulnerable / break

UI가 늦게 생성되거나 재생성돼도 현재 상태를 다시 구성할 수 있습니다.

---

## Presenter

`UDungeonStagePresenter`는 replicated state를 Definition / Presentation asset과 조합해 Widget이 소비하기 쉬운 `FDungeonStageViewData`로 변환합니다.

ViewData는 다음을 포함합니다.

- Dungeon route
- 3계층 objective
- Optional objective 상태
- Party member health
- Boss panel
- Result stats / rewards
- World marker
- Minimap rooms
- Elapsed / best time
- Outcome / grade

Widget은 gameplay tag나 transition rule을 해석하지 않습니다.

---

## LocalPlayer Router

`UDungeonUIRouterSubsystem`은 LocalPlayer 단위로 활성 UI를 관리합니다.

- participant에게만 dungeon UI 노출
- run 중 기존 screen hide / restore
- 최신 ViewData 유지
- room title / banner notice routing
- registry asset을 통한 UI class/config 접근

Split-screen까지 적극 지원하는 프로젝트는 아니지만, UI ownership을 PlayerController가 아니라 LocalPlayer 수명에 맞춘 구조입니다.

---

## 서버 시간 기준 UI

Timer와 Boss action progress는 local elapsed accumulation이 아니라 **replicated server time / deadline**을 기준으로 계산합니다.

따라서 client frame 차이나 UI recreation 때문에 timer 기준이 달라지지 않습니다.

---

## Minimap / World Marker도 같은 ViewData를 소비

Minimap은 별도의 dungeon logic을 다시 구현하지 않습니다.

Presenter가:

- map texture
- map bounds
- room rectangles
- current room
- objective destination

을 ViewData로 만들고 Widget은 이를 표현합니다.

World marker 역시 현재 open objective가 가리키는 target을 presentation data와 조합해 표시합니다.

---

## 공용 Notice로 일반화

Dungeon 전용 Toast / Room Title 구현은 `UNoticeSubsystem`으로 이동했습니다.

Notice는:

- Banner / Title slot
- TakeTurns / ReplaceShowing
- producer channel
- soft widget/style config

을 제공하고 다음 기능이 같은 시스템을 사용합니다.

- Dungeon banner
- Dungeon room title
- Level-up
- Capture failure
- Crafting complete
- Island region title

---

## 관련 코드

- [DungeonStagePresenter](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/UI/Dungeon/DungeonStagePresenter.h)
- [DungeonViewData](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/UI/Dungeon/DungeonViewData.h)
- [DungeonUIRouterSubsystem](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/UI/Dungeon/DungeonUIRouterSubsystem.h)
- [NoticeSubsystem](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/UI/Notice/NoticeSubsystem.h)
