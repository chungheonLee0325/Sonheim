# 17. Quest System

퀘스트의 선행 조건·단계·목표·보상은 `DT_Quest`에서 정의하고, 플레이어별 진행 기록은 `UQuestComponent`가 관리합니다. 서버는 처치·포획·제작·지역 이동과 NPC 상호작용을 집계해 **목표 달성, 단계 전환, 완료 보고와 보상 지급**을 처리합니다.

## 실제 퀘스트 화면

| 퀘스트 일지 | NPC 대화 |
|---|---|
| ![퀘스트 일지: 단계·목표·보상](../Media/Wiki/16_UMG_Migration_with_Claude/16_journal_after.jpg) | ![NPC 퀘스트 대화: 수락·완료 보고](../Media/Wiki/16_UMG_Migration_with_Claude/16_dialog_after.jpg) |

일지에서는 진행 중인 퀘스트의 목표·보상·추적 여부를 확인하고, NPC 대화에서 퀘스트를 수락하거나 완료 보고합니다. 화면의 시각적 개편은 [[18. UMG v2 Migration|18_UMG_v2_Migration]]에서 다룹니다.

**퀘스트 수락부터 완료까지 — 플레이 영상**

https://github.com/user-attachments/assets/356d1438-7c3c-417d-8bd4-b8dcb7a3efb6

영상에서는 **NPC 대화 → 퀘스트 수락 → 목표 진행과 HUD 갱신 → 완료 보고 → 보상 수령**의 흐름을 확인할 수 있습니다.

## 목차

- [퀘스트 정의와 플레이어 기록](#퀘스트-정의와-플레이어-기록)
- [목표별 진행도 계산과 단계 전환](#목표별-진행도-계산과-단계-전환)
- [NPC 상호작용과 서버 검증](#npc-상호작용과-서버-검증)
- [월드 오브젝트와 퀘스트 연동](#월드-오브젝트와-퀘스트-연동)
- [기록 복제·저장과 UI 갱신](#기록-복제저장과-ui-갱신)
- [실행 검증](#실행-검증)
- [관련 코드](#관련-코드)

---

## 퀘스트 정의와 플레이어 기록

### DT_Quest — 퀘스트 조건과 보상 정의

`UQuestSubsystem`은 `DT_Quest`의 `FQuestData`를 로드합니다. 하나의 퀘스트는 여러 `FQuestStep`으로 구성되고, 각 단계에는 동시에 진행할 수 있는 `FQuestObjective`가 들어갑니다.

~~~cpp
// QuestTypes.h — 선언 매크로 및 일부 필드 생략
struct FQuestObjective
{
    EQuestGoal Goal = EQuestGoal::Kill;
    int32 Id = 0;
    FName Target;
    int32 Required = 1;
    FText Label;
};

struct FQuestStep
{
    FText Summary;
    TArray<FQuestObjective> Objectives;
};

struct FQuestData : public FTableRowBase
{
    int32 QuestID = 0;
    FText Title;
    FText Description;
    FName Giver;
    FName Receiver;
    TArray<int32> Prerequisites;
    TArray<FQuestStep> Steps;
    int32 Exp = 0;
    TArray<FItemReward> Items;
};
~~~

- `Prerequisites`: 수락 전에 완료해야 하는 퀘스트 ID
- `Giver / Receiver`: 수락·완료 보고 대상의 이름. `Receiver`가 `None`이면 마지막 목표 달성 시 바로 완료
- `Steps / Objectives`: 단계별 설명과 목표 종류·대상·필요 횟수
- `Exp / Items`: 최종 완료 보상

![DT_Quest 편집 화면 — 퀘스트 정의와 단계별 목표](https://github.com/user-attachments/assets/ed187038-120f-441c-a4bd-c733977068ec)

*`DT_Quest`에서 퀘스트 정의를 관리하는 Unreal Editor 화면입니다. 단계별 목표와 조건은 `FQuestStep`·`FQuestObjective` 구조에 대응합니다.*

### FQuestRecord — 플레이어별 진행 기록

같은 퀘스트를 수락하더라도 플레이어마다 현재 단계와 목표 진행도는 다릅니다. `ASonheimPlayerState`의 `UQuestComponent`가 다음 기록을 관리합니다.

~~~cpp
// QuestTypes.h — 선언 매크로 생략
struct FQuestRecord : public FFastArraySerializerItem
{
    int32 QuestID = 0;
    EQuestState State = EQuestState::Active;
    int32 Step = 0;
    TArray<int32> Progress;
    bool bTracked = true;
};
~~~

`Step`은 현재 단계 인덱스이며, `Progress[i]`는 해당 단계의 `Objectives[i]` 진행도입니다. 다음 단계로 이동할 때 `Progress` 배열을 새 목표 수에 맞춰 0으로 초기화합니다. `bTracked`는 HUD의 퀘스트 추적 여부입니다.

퀘스트 정의 데이터는 각 플레이어의 기록에 복사하지 않습니다. 진행 기록에는 퀘스트 ID와 현재 상태·진행 수치만 저장하고, 화면을 구성할 때 정의 데이터와 다시 연결합니다.

## 목표별 진행도 계산과 단계 전환

### 목표 종류에 따라 다른 집계 방식

`UQuestComponent`는 목표 종류(`EQuestGoal`)에 맞는 경로로 진행도를 갱신합니다.

| 목표 | 집계 기준 | 처리 경로 |
|---|---|---|
| `Kill` / `Capture` | 지정한 몬스터 처치·포획 횟수 누적 | `ReportDeed → HandleDeed` |
| `Craft` | 지정 아이템 제작 결과 누적 | `ReportDeed → HandleDeed` |
| `Reach` | 지정 지역에 도착하면 목표 수치 충족 | `ReportDeed → HandleDeed` |
| `ClearDungeon` | 지정 던전 클리어 횟수 누적 | `ReportDeed → HandleDeed` |
| `Possess` | **현재 인벤토리에 보유한 아이템 수량** | `HandleInventory → Settle` |
| `Talk` | 지정한 NPC·오브젝트와 상호작용 | `Interact` |
| `Deliver` | 상호작용 시 실제 인벤토리 아이템을 차감한 수량 | `Interact` |

`HandleDeed`는 이벤트 종류와 몬스터·아이템·던전 ID 또는 지역 이름을 비교합니다. `Advance`는 일치하는 목표의 수치를 더하되 `Required`를 넘지 않도록 제한합니다.

`Possess`와 `Deliver`는 모두 아이템과 관련되지만 판정 방식이 다릅니다.

- **보유(`Possess`)**: 누적 획득량이 아니라 현재 인벤토리 수량을 읽습니다. 아이템 수량이 줄면 진행도도 다시 계산할 수 있습니다.
- **전달(`Deliver`)**: 지정 대상과 상호작용할 때 `RemoveItem`으로 실제 아이템을 차감하고, 차감에 성공한 수량만 진행도에 더합니다.

~~~mermaid
flowchart TB
    A["처치·포획·제작·지역 도착·던전 완료"] --> B["HandleDeed → Advance"]
    C["인벤토리 수량 변경"] --> D["HandleInventory → Settle"]
    E["NPC·건설 대상 상호작용"] --> F["Interact (Talk / Deliver)"]
    B --> G["FQuestRecord<br/>Step / Progress / State"]
    D --> G
    F --> G
    G --> H["UQuestComponent::Changed"]
    H --> I["MarkItemDirty → OwnerOnly FastArray"]
    H --> J["StorePlayer → Quests SaveGame"]
    I --> K["소유 클라이언트 OnQuestsChanged → UI"]
~~~

### 모든 목표 달성 후 단계 전환

한 단계의 `Objectives`는 모두 충족해야 다음 단계로 넘어갑니다. `UQuestComponent::Settle`은 현재 단계의 진행도를 확인하고 다음 중 하나를 처리합니다.

~~~mermaid
flowchart TD
    A["Active · 현재 단계"] --> B{"모든 목표를 달성했는가?"}
    B -->|아니요| A
    B -->|예| C{"다음 단계가 있는가?"}
    C -->|예| D["Step 증가·Progress 초기화"]
    D --> A
    C -->|아니요| E{"보고 대상 Receiver가 있는가?"}
    E -->|예| F["ReadyToTurnIn · NPC 보고 대기"]
    E -->|아니요| G["Finish · Completed · 보상 지급"]
    F --> H["ServerTurnIn 검증"]
    H --> G
~~~

~~~cpp
// QuestComponent.cpp — Settle 내부 핵심 분기
if (!bDone) return;

if (Quest.Steps.IsValidIndex(Record.Step + 1))
{
    ++Record.Step;
    Record.Progress.Init(0, Quest.Steps[Record.Step].Objectives.Num());
}
else if (Quest.Receiver.IsNone())
{
    Finish(Record, Quest);
}
else
{
    Record.State = EQuestState::ReadyToTurnIn;
    Record.Progress.Reset();
}
~~~

`Finish`는 `State`를 `Completed`로 변경하고 추적을 해제한 뒤 `SonheimReward::Grant`로 보상을 지급합니다. 이미 완료한 기록은 다시 완료 보고할 수 없도록 서버에서 상태를 검사합니다.

## NPC 상호작용과 서버 검증

`AQuestGiver`는 `IInteractableInterface`로 기존 상호작용 입력을 재사용하고, `IQuestTarget`의 이름으로 퀘스트 대상과 연결합니다. 상호작용 시 서버에서 `UQuestComponent::Interact`로 진행 중인 `Talk / Deliver` 목표를 먼저 처리한 다음, 해당 플레이어에게 퀘스트 대화창을 엽니다.

NPC 머리 위 마커는 플레이어의 퀘스트 기록을 기준으로 표시합니다.

- **`!`**: 새로 수락할 퀘스트가 있음 (`GetOffers`)
- **`?`**: 완료 보고할 퀘스트가 있음 (`GetReports`)

마커는 로컬 플레이어 상태를 0.5초마다 확인하고, 메뉴가 게임플레이 입력을 차단하는 동안 숨깁니다.

![월드에 배치한 퀘스트 NPC](../Media/Wiki/16_UMG_Migration_with_Claude/16_islander_after.jpg)

*NPC의 상호작용 안내와 퀘스트 상태 표시는 플레이어별 기록을 기준으로 결정합니다.*

### 수락·완료 보고 검증

대화창의 수락·보고 버튼은 `ServerAccept` 또는 `ServerTurnIn` RPC를 호출합니다. 서버는 클라이언트가 보낸 퀘스트 ID와 대상 Actor를 사용하기 전에 조건을 검사합니다.

| 서버 요청 | 검사 항목 |
|---|---|
| `ServerAccept` | 퀘스트 정의·선행 퀘스트 완료·미수락·유효한 첫 단계·수락 대상 |
| `ServerTurnIn` | 진행 기록 존재·`ReadyToTurnIn` 상태·완료 보고 대상 |
| `ServerAbandon` | 기록이 존재하고 이미 완료된 퀘스트가 아닌지 |
| `ServerTrack` | 기록 존재·추적 상태 변경 여부 |

`IsAt`은 `IQuestTarget::GetQuestTargetName`이 정의의 `Giver / Receiver`와 일치하는지, 플레이어 Pawn에서 대상까지 거리가 `ReachDistance`(기본 600 Unreal Units) 이내인지 확인합니다. 정의에서 대상 이름이 `None`이면 대상 검사는 생략합니다.

## 월드 오브젝트와 퀘스트 연동

`AQuestBuildSite`는 `Deliver` 목표와 연결된 건설 오브젝트입니다. 플레이어가 상호작용하면 서버에서 건설 여부·플레이어 상태·거리를 확인한 뒤 `UQuestComponent::Interact`로 자재 전달을 처리합니다. 아이템 차감과 목표 진행도 계산은 퀘스트 컴포넌트가 담당합니다.

### 플레이어별 진행 기록에서 공유 월드 상태로

건설 오브젝트는 서버에서 `UQuestComponent::OnAnyRecordChanged`를 구독하고, 시작 시점과 퀘스트 기록 변경 시 `Refresh`를 실행합니다. `GameState::PlayerArray`에 있는 플레이어의 해당 `QuestID` 기록을 검사해 **한 명이라도 `State != Active` 또는 `Step > BuildStep`이면 `bBuilt = true`**로 전환합니다.

~~~mermaid
flowchart TD
    A["플레이어가 건설 대상에 자재 전달"] --> B["서버: UQuestComponent::Interact"]
    B --> C["아이템 차감·FQuestRecord 갱신"]
    C --> D["OnAnyRecordChanged → AQuestBuildSite::Refresh"]
    D --> E{"플레이어 기록 중<br/>건설 완료 조건을 만족하는 기록이 있는가?"}
    E -->|예| F["bBuilt = true"]
    E -->|아니요| G["설계 상태 유지"]
    F --> H["bBuilt 월드 상태 복제"]
    H --> I["OnRep_Built → ApplyLook"]
~~~

`bBuilt`는 `COND_OwnerOnly`인 개인 퀘스트 기록과 달리 **월드에 공유되는 복제 상태**입니다. 서버는 변경 즉시 `ApplyLook`을 실행하고, 클라이언트는 `OnRep_Built`에서 동일한 표시 전환을 적용합니다. 설계 상태에서는 계획 재질과 비활성 충돌을, 완성 상태에서는 메시 재질과 충돌을 사용합니다. 한 번 완성되면 해당 실행 중에는 다시 설계 상태로 돌아가지 않습니다.

`bBuilt` 자체를 별도 SaveGame에 저장하는 코드는 없습니다. 건설 오브젝트는 시작 시점과 플레이어 기록 변경 시 퀘스트 진행 상태를 다시 확인합니다.

| 건설 전 | 건설 완료 |
|---|---|
| ![자재 전달 전 건설 현장](https://github.com/user-attachments/assets/b0a42c3e-8ec8-4216-bfd1-ba431b73de0b) | ![자재 전달 후 건설 완료 상태](https://github.com/user-attachments/assets/41566e26-6b56-438c-918c-bb6a838eb639) |
| 설계 재질, 자재 전달 대상 | 완성된 메시와 충돌 활성화 |

## 기록 복제·저장과 UI 갱신

### 플레이어별 진행 기록의 동기화

퀘스트 진행 기록은 `FQuestRecord : FFastArraySerializerItem`과 `FQuestList : FFastArraySerializer`로 관리하고, 소유 클라이언트에만 복제합니다.

~~~cpp
// QuestComponent.cpp
DOREPLIFETIME_CONDITION(UQuestComponent, Quests, COND_OwnerOnly);
~~~

`Changed`가 수행하는 **복제 변경 표시와 저장은 서로 다른 경로**입니다.

- **복제**: `Quests.MarkItemDirty(Record)`가 변경 항목을 표시합니다. `COND_OwnerOnly` FastArray를 받은 소유 클라이언트에서는 `PostReplicatedAdd / PostReplicatedChange / PreReplicatedRemove`가 `OnQuestsChanged`를 호출해 UI 갱신을 유도합니다.
- **저장**: 서버의 `UQuestSubsystem::StorePlayer`가 해당 플레이어의 퀘스트 기록을 호스트 SaveGame `Quests` 슬롯에 기록합니다.
- **서버 내부 알림**: `Changed`는 `OnQuestsChanged`와 `OnAnyRecordChanged`도 호출합니다. 건설 오브젝트는 후자의 알림으로 완료 상태를 재평가합니다.

기록 단위 갱신에는 `MarkItemDirty`를, 퀘스트 포기나 저장 기록 로드처럼 배열을 제거·교체하는 경로에는 `MarkArrayDirty`를 사용합니다.

저장 기록을 불러올 때는 현재 `DT_Quest`에 없는 퀘스트와 유효하지 않은 진행 단계(진행 중 기록)를 제외하고, `Possess` 목표는 현재 인벤토리 수량으로 다시 계산합니다.

### 퀘스트 화면용 데이터 구성

`QuestView::Build`는 퀘스트 정의와 `FQuestRecord`를 조합해 `FQuestView`를 만듭니다. 각 목표의 현재 수치·필요 수치·완료 여부는 `FQuestObjectiveView`로 전달합니다.

~~~text
DT_Quest (제목·단계·목표·보상)
           +
FQuestRecord (현재 단계·진행도·추적 여부)
           ↓
QuestView::Build → FQuestView
           ↓
퀘스트 일지 · NPC 대화 · HUD 추적기
~~~

`EQuestState`의 `Active / ReadyToTurnIn / Completed`는 서버 진행 기록의 상태입니다. 반면 `EQuestViewState::Offered`는 **아직 수락하지 않았지만 NPC가 제안할 수 있는 퀘스트를 UI에 표시하기 위한 상태**로, 별도 진행 기록을 만들지 않습니다.

- **퀘스트 일지 (`UQuestJournalWidget`)**: 진행 중·완료·전체 필터, 추적 설정과 포기 기능을 제공합니다.
- **NPC 대화 (`UQuestDialogWidget`)**: `QuestView::AtTarget`으로 해당 NPC에서 수락하거나 보고할 퀘스트를 구성합니다.
- **HUD 추적기 (`UQuestTrackerWidget`)**: `bTracked`가 켜진 미완료 퀘스트의 현재 목표를 표시합니다.

![HUD에 표시된 퀘스트 추적 영역과 진행도](https://github.com/user-attachments/assets/9f56d3db-825c-49ee-a96f-e12266c4b387)

*HUD 추적기에 표시되는 퀘스트 제목·목표 진행 상태를 마킹한 실제 게임 화면입니다.*

퀘스트 수락·단계 갱신·보고 가능·완료 알림은 공용 `UNoticeSubsystem`의 `Feed` 슬롯으로 표시합니다. 보상 아이템은 도감과 공통으로 사용하는 `USlotWidget::AddReadOnly`로 표시합니다.

## 실행 검증

[PR #3](https://github.com/chungheonLee0325/Sonheim/pull/3)에 기록된 `verify_quest` 결과는 **8/8 통과**이며, 실제 던전 플레이 1판을 포함합니다. 상단 플레이 영상에서는 NPC 수락부터 목표 진행·완료 보고·보상 수령까지의 게임 흐름을 확인할 수 있습니다.

## 관련 코드

- [QuestTypes.h](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/Quest/QuestTypes.h) — 퀘스트 정의·플레이어 기록·상태
- [QuestSubsystem.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/Quest/QuestSubsystem.cpp) — `DT_Quest` 조회·SaveGame
- [QuestComponent.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/AreaObject/Player/Utility/QuestComponent.cpp) — 목표 집계·단계 전환·서버 검증·복제
- [QuestGiver.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/GameObject/Quest/QuestGiver.cpp) — NPC 상호작용·마커·대화창
- [QuestBuildSite.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/GameObject/Quest/QuestBuildSite.cpp) — 자재 전달과 건설 상태 연동
- [QuestView.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/UI/Quest/QuestView.cpp) — 일지·대화·추적기 표시 데이터
- [QuestWidget.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/UI/Quest/QuestWidget.cpp) — 퀘스트 UI 구성·갱신
- [[16. MonsterDex System|16_MonsterDex_System]] — `ReportDeed` 공유·도감 기록
- [[18. UMG v2 Migration|18_UMG_v2_Migration]] — 퀘스트 화면 및 알림 UI 개편
