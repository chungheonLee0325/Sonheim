# 17. Quest System

NPC 대화에서 퀘스트를 수락하고, 전투·포획·제작·탐험으로 목표를 진행한 뒤 완료 보고와 보상을 받도록 구현했습니다. 같은 퀘스트 정보를 **NPC 대화, 퀘스트 일지, HUD 추적기**에 표시하고, 건설 퀘스트는 실제 월드 오브젝트 상태도 변경합니다.

## 게임 화면

| 퀘스트 일지 | NPC 대화 |
|---|---|
| ![퀘스트 일지의 단계·목표·보상](../Media/Wiki/16_UMG_Migration_with_Claude/16_journal_after.jpg) | ![NPC 퀘스트 수락·보고 화면](../Media/Wiki/16_UMG_Migration_with_Claude/16_dialog_after.jpg) |

**수락 → 목표 진행 → 완료 보고 → 보상 수령**

https://github.com/user-attachments/assets/356d1438-7c3c-417d-8bd4-b8dcb7a3efb6

## 퀘스트 정의와 진행 기록

`DT_Quest`의 `FQuestData`에는 선행 퀘스트, 수락·보고 NPC, 단계별 목표, 경험치·아이템 보상을 정의합니다. `UQuestSubsystem`이 이 데이터를 조회하고, `ASonheimPlayerState`의 `UQuestComponent`는 플레이어가 실제로 진행한 기록만 보관합니다.

| 정의 데이터 — `FQuestData` | 플레이어 기록 — `FQuestRecord` |
|---|---|
| 제목·설명·`Prerequisites`·`Giver / Receiver` | `QuestID`·`State`·`Step` |
| 단계별 `FQuestStep / FQuestObjective` | 현재 단계의 목표별 `Progress` |
| `Exp / Items` 최종 보상 | HUD 추적 여부 `bTracked` |

~~~cpp
// QuestTypes.h — 주요 필드 발췌
struct FQuestObjective
{
    EQuestGoal Goal = EQuestGoal::Kill;
    int32 Id = 0;
    FName Target;
    int32 Required = 1;
    FText Label;
};

struct FQuestRecord : public FFastArraySerializerItem
{
    int32 QuestID = 0;
    EQuestState State = EQuestState::Active;
    int32 Step = 0;
    TArray<int32> Progress;
    bool bTracked = true;
};
~~~

진행 데이터에 제목·목표 문구·보상을 복사하지 않고 **QuestID와 상태·수량만 기록**합니다. 퀘스트 데이터가 바뀌어도 저장 기록이 화면용 문구를 별도로 보유할 필요가 없고, HUD와 대화창이 같은 정의를 참고할 수 있습니다.

![DT_Quest에서 단계별 목표를 작성한 화면](https://github.com/user-attachments/assets/ed187038-120f-441c-a4bd-c733977068ec)

## 게임플레이 동작을 목표 진행으로 연결

`UQuestComponent`는 목표 종류마다 진행도를 계산하는 방식이 다릅니다.

| 목표 | 진행 처리 |
|---|---|
| `Kill / Capture / Craft / ClearDungeon` | 게임플레이 `ReportDeed` 이벤트의 종류·대상 ID·횟수를 비교해 누적 |
| `Reach` | 지역 진입 이벤트의 `Target` 이름이 일치하면 달성 |
| `Possess` | 현재 인벤토리 아이템 수량을 다시 읽어 반영 |
| `Talk` | 대상 `IQuestTarget`과 상호작용했을 때 달성 |
| `Deliver` | 상호작용 시 인벤토리에서 자재를 실제 차감하고 차감한 수량만 반영 |

`HandleDeed`는 처치·포획·제작·지역 진입 등 이벤트를 `Advance`에 전달합니다. `Advance`는 현재 단계의 목표를 순회하면서 일치하는 목표에만 진행도를 더하고, 필요한 수량을 넘지 않도록 제한합니다.

`Possess`는 누적 이벤트가 아니라 **현재 보유 수량**입니다. `HandleInventory`는 아이템 변동 시 `Settle`로 수량을 다시 계산합니다. 반면 `Deliver`는 목표 대상에 상호작용했을 때 `RemoveItem`에 성공한 수량만 누적합니다. 두 목표는 UI에서 둘 다 아이템 목표로 보이지만 게임플레이 처리 기준이 다릅니다.

### 단계 전환과 완료 보고

현재 단계의 모든 `Objectives`가 충족되면 `Settle`이 다음 단계를 시작합니다. 다음 단계가 없다면 보고 대상(`Receiver`) 유무에 따라 **완료 보고 대기**로 이동하거나 즉시 완료·보상 지급을 처리합니다.

~~~mermaid
flowchart LR
    A["현재 Step의 목표 집계"] --> B{"모든 목표 완료"}
    B -->|예| C{"다음 Step"}
    C -->|있음| D["Step 증가<br/>Progress 초기화"]
    C -->|없음| E{"Receiver 지정"}
    E -->|있음| F["ReadyToTurnIn<br/>NPC 보고"]
    E -->|없음| G["Completed<br/>보상 지급"]
    F --> G
~~~

`ServerAccept`·`ServerTurnIn`은 서버에서 선행 조건·기록 상태·대상 `IQuestTarget`의 이름과 거리를 검사합니다. `Finish`는 먼저 `Completed`로 변경한 뒤 `SonheimReward::Grant`를 호출해 보상 중복 지급을 방지합니다.

## 퀘스트 데이터를 화면별로 가공

**게임플레이 기록과 UI 표시 상태는 같지 않습니다.** 예를 들어 NPC에서 아직 수락하지 않은 퀘스트를 보여주려면 `FQuestRecord`가 없어도 화면에는 ‘수락 가능(Offered)’ 상태가 필요합니다.

`QuestView::Build`가 `FQuestData`와 `FQuestRecord`를 결합해 `FQuestView`를 만들고, `State`·`Objectives`·`Rewards`를 화면에서 바로 사용하는 형태로 정리합니다.

~~~cpp
// QuestView.cpp — Build() 일부
const FQuestRecord* Record = Quests.FindRecord(QuestID);

View.bTracked = Record && Record->bTracked;
View.State = !Record ? EQuestViewState::Offered
    : Record->State == EQuestState::Active ? EQuestViewState::Active
    : Record->State == EQuestState::ReadyToTurnIn
        ? EQuestViewState::ReadyToTurnIn : EQuestViewState::Completed;
~~~

완료 보고 대기에서는 일반 목표 대신 **‘NPC에게 보고’** 항목을 표시합니다. 완료 상태의 목표는 필요한 수량만큼 달성된 값으로 표시합니다. 이러한 표현 규칙을 Widget마다 반복하지 않고 `QuestView`에서 처리합니다.

~~~mermaid
flowchart TD
    D["DT_Quest · FQuestData"] --> B["QuestView::Build<br/>FQuestView"]
    R["UQuestComponent · FQuestRecord"] --> B
    B --> J["QuestView::Journal<br/>진행/완료 목록"]
    B --> N["QuestView::AtTarget<br/>NPC별 수락·보고 목록"]
    J --> U["UQuestJournalWidget"]
    J --> H["UQuestTrackerWidget"]
    N --> Q["UQuestDialogWidget"]
~~~

| 화면 | 표시·입력 처리 |
|---|---|
| **퀘스트 일지** | 진행·완료·전체 필터, 퀘스트 선택, 추적 설정·포기 |
| **NPC 대화** | 해당 NPC의 `GetOffers / GetReports`를 화면에 구성, 수락·보고 요청 |
| **HUD 추적기** | `bTracked`가 켜진 미완료 퀘스트만 표시, `MaxShown` 수만큼 노출 |

![HUD 퀘스트 추적기의 실제 표시](https://github.com/user-attachments/assets/9f56d3db-825c-49ee-a96f-e12266c4b387)

### 데이터 변경과 위젯 갱신

`UQuestScreenWidget`은 화면이 열릴 때 `NativeOnOpened()`에서 `OnQuestsChanged`를 구독하고, 닫을 때 `NativeOnClosed()`에서 해제합니다. 퀘스트 진행이 변경되면 `Redraw()`에서 최신 `FQuestView` 목록을 다시 만들고, `DynamicEntryBox`의 목록·목표·보상 항목을 구성합니다.

~~~cpp
// QuestWidget.cpp — UQuestScreenWidget::Redraw() 발췌
Shown = Component ? ListQuests(*Component) : TArray<FQuestView>();
if (!PickedView())
    Picked = Shown.Num() ? Shown[0].QuestID : 0;

QuestList->Reset(true);
for (const FQuestView& View : Shown)
{
    UQuestListEntryWidget* Entry =
        QuestList->CreateEntry<UQuestListEntryWidget>();
    Entry->Show(View, View.QuestID == Picked);
    Entry->OnPicked.BindUObject(this, &UQuestScreenWidget::Pick);
}
~~~

목록을 다시 구성하더라도 **선택한 QuestID가 남아 있으면 선택을 유지**하고, 필터 변경 등으로 사라졌다면 첫 항목을 선택합니다. 데이터 업데이트가 들어올 때마다 상세 패널의 선택 상태가 불필요하게 초기화되지 않도록 한 처리입니다.

HUD 추적기는 `UQuestComponent`가 늦게 준비될 수도 있어 초기 바인딩이 없으면 약 1초 간격으로 다시 연결을 시도합니다. 연결 후에는 `OnQuestsChanged` 이벤트로 갱신합니다. NPC 대화의 수락·보고 버튼은 `FQuestView.State`에 따라 다른 RPC를 호출하고 화면을 닫으며, 실제 진행 변경 결과는 다시 기록·알림·UI 갱신으로 반영됩니다.

## 퀘스트 진행과 월드 오브젝트

`AQuestBuildSite`는 `Deliver` 목표와 연결된 건설 오브젝트입니다. 상호작용 시 서버의 `UQuestComponent::Interact`가 자재 차감과 개인 목표 진행도를 처리합니다. 건설물은 `OnAnyRecordChanged`를 구독해 전체 플레이어의 관련 기록을 확인합니다.

| 건설 전 | 건설 완료 |
|---|---|
| ![자재 전달 전 건설 현장](https://github.com/user-attachments/assets/b0a42c3e-8ec8-4216-bfd1-ba431b73de0b) | ![자재 전달 후 건설 완료 상태](https://github.com/user-attachments/assets/41566e26-6b56-438c-918c-bb6a838eb639) |

~~~cpp
// QuestBuildSite.cpp — Refresh() 핵심 조건
if (Record && (Record->State != EQuestState::Active
               || Record->Step > BuildStep))
{
    bBuilt = true;
    ApplyLook();
    return;
}
~~~

한 명의 플레이어라도 해당 단계를 완료했다면 서버에서 `bBuilt`를 변경합니다. 건설물은 `OnRep_Built`에서 외형·충돌을 갱신하므로, **개인 퀘스트 진행과 모두가 보는 월드 상태를 별도로 관리**합니다.

## 진행 기록 복제와 저장

`UQuestComponent`의 `FQuestRecord`는 `FastArray`로 관리하고 소유 클라이언트에 복제합니다. 항목이 추가·수정·삭제되면 복제 콜백이 `OnQuestsChanged`를 발생시키고 위젯이 다시 그립니다.

~~~cpp
// QuestComponent.cpp — 복제·변경 알림
DOREPLIFETIME_CONDITION(UQuestComponent, Quests, COND_OwnerOnly);

void UQuestComponent::Changed(FQuestRecord& Record)
{
    Quests.MarkItemDirty(Record);
    OnQuestsChanged.Broadcast();
    OnAnyRecordChanged.Broadcast();
    if (UQuestSubsystem* Book =
        GetWorld()->GetGameInstance()->GetSubsystem<UQuestSubsystem>())
        Book->StorePlayer(Owner()->GetSaveKey(), Quests.Items);
}
~~~

서버의 `Changed`는 저장과 서버 내부 알림도 함께 처리합니다. 퀘스트 포기처럼 배열을 제거할 때는 `MarkArrayDirty`를 사용합니다. 저장 기록 로드 시에는 현재 정의에 없는 퀘스트·유효하지 않은 단계 기록을 제외하고, `Possess` 목표는 현재 아이템 보유량으로 다시 계산합니다.

## 관련 코드

- [QuestTypes.h](../../Sonheim/Source/Sonheim/Quest/QuestTypes.h) · [QuestSubsystem.cpp](../../Sonheim/Source/Sonheim/Quest/QuestSubsystem.cpp) — 정의·저장
- [QuestComponent.cpp](../../Sonheim/Source/Sonheim/AreaObject/Player/Utility/QuestComponent.cpp) — 목표 집계·단계·수락·완료
- [QuestView.cpp](../../Sonheim/Source/Sonheim/UI/Quest/QuestView.cpp) · [QuestWidget.cpp](../../Sonheim/Source/Sonheim/UI/Quest/QuestWidget.cpp) — 표시 데이터·일지·NPC 대화·HUD
- [QuestBuildSite.cpp](../../Sonheim/Source/Sonheim/GameObject/Quest/QuestBuildSite.cpp) · [QuestGiver.cpp](../../Sonheim/Source/Sonheim/GameObject/Quest/QuestGiver.cpp) — 상호작용·공유 월드 상태
- [16. MonsterDex System](16_MonsterDex_System.md) · [18. UMG v2 Migration](18_UMG_v2_Migration.md)
