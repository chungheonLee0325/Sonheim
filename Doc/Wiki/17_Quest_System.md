# 17. Quest System

Sonheim 퀘스트는 **DataTable로 단계·목표를 정의하고, 서버가 게임플레이 이벤트와 NPC 상호작용을 집계하여 진행 상태와 보상을 확정하는 콘텐츠 시스템**입니다. 월드 콘텐츠와 UI는 퀘스트 상태를 참조하지만 완료 판정 자체는 `UQuestComponent`가 관리합니다.

## 요약

- `FQuestData → FQuestStep → FQuestObjective`로 선행 조건, 목표 종류, 완료 보고와 보상을 정의합니다.
- `ASonheimPlayerState::ReportDeed`를 통해 처치·포획·제작·지역 도착·던전 클리어를 집계합니다.
- `Possess`는 현재 인벤토리 수량을 재계산하며, `Talk / Deliver`는 `IQuestTarget` 상호작용 경로로 처리합니다.
- `UQuestComponent`가 단계 진행·수락·보고·포기·추적을 서버에서 관리합니다.
- `FQuestList`의 OwnerOnly FastArray 복제와 호스트 SaveGame으로 플레이어별 진행을 유지합니다.

## Runtime Result

| 퀘스트 일지 | NPC 대화 |
|---|---|
| ![퀘스트 일지](../Media/Wiki/16_UMG_Migration_with_Claude/16_journal_after.jpg) | ![NPC 퀘스트 대화](../Media/Wiki/16_UMG_Migration_with_Claude/16_dialog_after.jpg) |

*일지에서 단계·목표·보상·추적 상태를 확인하고 NPC 상호작용을 통해 퀘스트를 수락·보고합니다. 화면 스타일 변경 자체는 [[18. UMG v2 Migration|18_UMG_v2_Migration]]에서 다룹니다.*

## 목차

- [정의 데이터와 Runtime State](#정의-데이터와-runtime-state)
- [목표 집계와 단계 전환](#목표-집계와-단계-전환)
- [서버 요청과 NPC 상호작용](#서버-요청과-npc-상호작용)
- [복제·저장·UI 연결](#복제저장ui-연결)
- [관련 코드](#관련-코드)

---

## 정의 데이터와 Runtime State

### 퀘스트 한 건의 정의

`UQuestSubsystem`은 `DT_Quest`에서 `FQuestData`를 읽습니다. 정의와 진행 기록은 별도로 관리합니다.

~~~text
DT_Quest / FQuestData
  QuestID · Title · Description
  Giver · Receiver · Prerequisites
  Steps[]
    Summary
    Objectives[]
      Goal · Id · Target · Required · Label
  Exp · Items

PlayerState / FQuestRecord
  QuestID · State · Step · Progress[] · bTracked
~~~

- `EQuestGoal`: `Kill`, `Capture`, `Possess`, `Craft`, `Reach`, `ClearDungeon`, `Talk`, `Deliver`.
- `EQuestState`: `Active`, `ReadyToTurnIn`, `Completed`.
- 동일 단계의 목표는 각각 진행량을 갖고 **전부 충족되면** 다음 단계로 넘어갑니다.
- 선행 퀘스트는 `Prerequisites`에 정의하고 `CanAccept`에서 완료 여부를 확인합니다.
- `FQuestRecord::Progress[i]`는 **현재 Step의 `Objectives[i]`와 같은 인덱스**를 사용합니다. Step이 바뀌면 목표 수에 맞춰 배열을 0으로 초기화합니다.

<details>
<summary>DataTable 정의와 플레이어별 Runtime Record 필드 보기</summary>

~~~cpp
// QuestTypes.h — USTRUCT / UPROPERTY 메타데이터 생략
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

struct FQuestRecord : FFastArraySerializerItem
{
    int32 QuestID = 0;
    EQuestState State = EQuestState::Active;
    int32 Step = 0;
    TArray<int32> Progress;
    bool bTracked = true;
};
~~~

</details>


## 목표 집계와 단계 전환

### 서로 다른 원천을 공통 진행 상태에 반영

~~~mermaid
flowchart TB
  D["PlayerDeed<br/>Kill · Capture · Craft · Reach · ClearDungeon"] --> A["HandleDeed → Advance"]
  I["Inventory Changed"] --> B["HandleInventory → Settle"]
  N["IQuestTarget Interact<br/>Talk / Deliver"] --> C["Interact → Settle"]
  A --> R["FQuestRecord<br/>Step / Progress / State"]
  B --> R
  C --> R
  R --> O["Changed<br/>MarkItemDirty + Save"]
  O --> U["Owner Replication<br/>QuestView / Journal / Tracker"]
~~~

`HandleDeed`는 현재 열려 있는 목표와 이벤트의 종류·종 ID·아이템 ID·지역을 비교합니다. `Advance`는 완료 수치가 `Required`를 넘지 않도록 제한하고, 증가가 있을 때만 `Settle`과 `Changed`를 호출합니다.

`Possess`는 누적 획득이 아니라 **현재 인벤토리에 보유한 개수**로 계산하므로, 아이템을 사용하거나 이동하면 남은 목표 진행량도 재평가할 수 있습니다. `Deliver`는 `Interact` 시 서버에서 실제 아이템을 `RemoveItem`한 뒤 진행량에 더합니다. 이 차이는 데이터 정의의 `EQuestGoal`에 따라 `UQuestComponent::HandleInventory`와 `UQuestComponent::Interact`에서 분기합니다.

### Settle의 상태 전환

~~~mermaid
flowchart TD
  A["Active<br/>현재 Step"] --> B{"모든 목표 완료?"}
  B -->|No| A
  B -->|Yes| C{"다음 Step 있음?"}
  C -->|Yes| D["Step++<br/>Progress 재초기화"]
  D --> A
  C -->|No| E{"Receiver 있음?"}
  E -->|Yes| F["ReadyToTurnIn<br/>NPC 보고 대기"]
  E -->|No| G["Finish → Completed<br/>Reward Grant"]
  F --> H["ServerTurnIn + 조건 검사"]
  H --> G
~~~

`Settle`은 모든 `Objectives`를 충족한 경우 Step을 증가시키고 `Progress` 배열을 새 단계 길이에 맞게 초기화합니다. 마지막 Step에서는 `Receiver`가 없으면 즉시 `Finish`, 있으면 `ReadyToTurnIn`으로 전환합니다.

<details>
<summary>Settle의 단계 전환과 완료 분기 코드 보기</summary>

~~~cpp
// QuestComponent.cpp — Settle 내부 분기 발췌
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

</details>

`Finish`는 `Completed`를 먼저 기록하고 `SonheimReward::Grant`를 호출합니다. 동일 진행 기록에 대해 다시 완료 보고하더라도 보상을 중복 지급하지 않도록 한 순서입니다.

## 서버 요청과 NPC 상호작용

`ServerAccept`, `ServerTurnIn`, `ServerAbandon`, `ServerTrack`는 서버 RPC입니다. 수락/보고 시 플레이어가 보내온 QuestID를 그대로 신뢰하지 않고 서버에서 다음을 확인합니다.

| 요청 | 주요 검사 |
|---|---|
| Accept | Quest 정의·선행 조건·미수락·단계 유효성·Giver 이름 및 거리 |
| TurnIn | 진행 기록·ReadyToTurnIn 상태·Receiver 이름 및 거리 |
| Abandon | 완료되지 않은 진행 기록인지 |
| Track | 해당 기록이 존재하고 추적 상태가 달라졌는지 |

`IQuestTarget::GetQuestTargetName`을 구현한 Actor와 플레이어 Pawn 사이의 거리를 `IsAt`에서 검사합니다(기본 `ReachDistance = 600.f`). Quest 정의에서 Giver/Receiver가 `None`이면 이 대상 검사는 생략하도록 설계되어 있습니다.

### 월드에 연결한 콘텐츠

`AQuestGiver`는 기존 `IInteractableInterface` 경로로 NPC 대화를 열고, 서버의 `UQuestComponent::Interact`로 Talk/Deliver 목표를 먼저 반영한 뒤 해당 플레이어의 대화창을 표시합니다. 머리 위 !/? 마커는 로컬 퀘스트 상태를 0.5초 주기로 확인합니다.

`AQuestBuildSite`는 설계 상태의 건설 오브젝트입니다. 상호작용을 통해 Deliver 목표를 처리하고, 서버에서 관련 퀘스트 진행 변경을 관찰하여 `bBuilt`를 갱신·복제합니다. 완성 시 시각 표현과 충돌 설정이 바뀝니다. 퀘스트 데이터를 수정할 때 월드 Actor에 별도의 완료 규칙을 중복 작성하지 않도록 한 경계입니다.

![퀘스트 NPC](../Media/Wiki/16_UMG_Migration_with_Claude/16_islander_after.jpg)

*퀘스트 제공 NPC는 월드 상호작용과 수락·완료 보고 상태에 따라 안내를 갱신합니다.*

## 복제·저장·UI 연결

`FQuestRecord : FFastArraySerializerItem`, `FQuestList : FFastArraySerializer` 구조로 진행 기록을 복제합니다.

~~~cpp
DOREPLIFETIME_CONDITION(UQuestComponent, Quests, COND_OwnerOnly);
~~~

`Changed`는 `Quests.MarkItemDirty`, `OnQuestsChanged`, 월드 진행 구독용 `OnAnyRecordChanged`, `UQuestSubsystem::StorePlayer`을 실행합니다. 클라이언트는 FastArray의 `PostReplicatedAdd / Change / PreReplicatedRemove`에서 화면 갱신 델리게이트를 받습니다.

`UQuestSubsystem`은 플레이어별 기록을 호스트 SaveGame 슬롯 `Quests`에 저장합니다. 저장된 QuestID가 데이터 테이블에서 사라졌거나 유효하지 않은 Step을 가리키면 로드시 제외하고, 보유 아이템 조건을 다시 계산합니다. 중앙 서버의 계정 DB를 이용하는 방식은 아닙니다.

UMG는 `QuestView::Build / Journal / AtTarget`이 만든 화면용 데이터를 사용합니다. `UQuestJournalWidget`은 상태별 필터·추적·포기를 제공하고, `UQuestTrackerWidget`은 추적 중인 미완료 목표를 표시합니다. 아이템 보상은 `USlotWidget::AddReadOnly`를 재사용합니다.

## 관련 코드

- [QuestTypes.h](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/Quest/QuestTypes.h)
- [QuestComponent.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/AreaObject/Player/Utility/QuestComponent.cpp)
- [QuestSubsystem.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/Quest/QuestSubsystem.cpp)
- [QuestGiver.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/GameObject/Quest/QuestGiver.cpp)
- [QuestBuildSite.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/GameObject/Quest/QuestBuildSite.cpp)
- [QuestView.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/UI/Quest/QuestView.cpp)
- [[16. MonsterDex System|16_MonsterDex_System]] — 공통 PlayerDeed를 공유하는 도감
- [[18. UMG v2 Migration|18_UMG_v2_Migration]] — 화면 개편·키 안내·알림
