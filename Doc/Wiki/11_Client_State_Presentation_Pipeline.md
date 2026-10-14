# 11. Client State Presentation — 게임 데이터에서 UI까지

Sonheim의 화면은 게임플레이 컴포넌트가 관리하는 상태를 읽어 UI에 필요한 값으로 바꿉니다. **단일 컴포넌트의 HP·아이템 갱신**과 **여러 데이터를 함께 보여주는 던전 HUD·결과 화면**은 갱신 방식이 다르므로 화면별로 필요한 구조를 사용했습니다.

## 실제 화면

| 던전 HUD | 던전 결과 |
|---|---|
| ![목표·파티·미니맵·타이머를 표시한 던전 HUD](../Media/Wiki/11_Client_State_Presentation_Pipeline/11_dungeon_hud.png) | ![보상·등급·클리어 기록 결과 화면](../Media/Wiki/11_Client_State_Presentation_Pipeline/11_dungeon_result.png) |

| 데이터·화면 | 구현 방식 |
|---|---|
| 기본 HUD·인벤토리 | 상태를 가진 컴포넌트의 Delegate로 화면 갱신 |
| 도감·퀘스트 | DataTable 정의 + 플레이어 기록 → `MonsterDexView / QuestView` |
| 던전 HUD·결과 | GameState의 현재 진행 상태 + 화면 설정 + 파티 HP → `FDungeonStageViewData` |
| 메뉴·알림 | `UUIStackSubsystem`·`UNoticeSubsystem` 등 LocalPlayer별 정책 |

Widget은 **선택한 목록 항목·필터·스크롤** 같은 화면 상태를 소유할 수 있지만, HP·인벤토리·퀘스트·던전 진행의 실제 원본 상태를 갖고 있지는 않습니다.

## 컴포넌트 상태를 화면에 반영

### 기본 HUD

`UHealthComponent`·`UStaminaComponent`·`ULevelComponent` 등은 각각 게임플레이 상태를 관리합니다. `ASonheimPlayerController`와 HUD 위젯은 변경 Delegate를 연결하고 초기 표시 값도 적용합니다. HP가 바뀔 때마다 모든 위젯이 Tick에서 값을 반복 조회하지 않도록 했습니다.

### 인벤토리·보관함·제작

`UInventoryComponent`가 관리하는 슬롯 데이터에는 `ItemID`·수량·슬롯 순서가 있습니다. `UInventoryWidget`은 이를 `USonheimGameInstance::GetDataItem`의 아이템 정의와 조합해 `USlotWidget`에 표시합니다.

~~~text
InventoryComponent (ItemID · Count · Slot)
                +        FItemData (아이콘 · 이름)
                ↓
           InventoryWidget
                ↓
       SlotWidget · 장비 슬롯
~~~

보관함은 플레이어와 보관함 슬롯을 한 화면에서 보여주면서 기존 Slot 드래그 처리를 재사용합니다. 제작 UI는 레시피 정의와 보유 재료·제작대의 작업 진행도를 결합하고, 레시피를 바꿀 때 재료 Row를 재사용합니다. 각각의 **실제 아이템 이동·제작 요청은 게임플레이 컴포넌트**로 보냅니다.

버리기·폐기 확인은 `UConfirmWidget`이 수량과 확인 입력을 받고, `UInventoryWidget`이 Confirm 콜백을 받아 인벤토리 요청을 수행합니다. 확인 팝업은 아이템 상태를 직접 수정하지 않습니다.

[07. Inventory & Crafting](07_Inventory_Crafting.md)

## 도감·퀘스트의 ViewData

도감과 퀘스트는 **정의된 콘텐츠 전체**와 **플레이어가 진행한 기록**을 결합해야 합니다.

~~~mermaid
flowchart LR
    A["DataTable<br/>몬스터·퀘스트 정의"] --> V["MonsterDexView::Build<br/>QuestView::Build"]
    B["플레이어 Component<br/>발견·단계·진행도"] --> V
    V --> D["페이지·목표·보상<br/>표시 데이터"]
    D --> W["목록 · 상세 · 대화 · HUD"]
~~~

`MonsterDexView::Build()`는 몬스터 ID로 도감 정의·`dt_AreaObject`·기록을 조회해 `FMonsterDexBookView`를 만듭니다. `QuestView::Build()`는 `DT_Quest` 정의와 `FQuestRecord`를 조합해 `FQuestView`를 구성합니다.

예를 들어 퀘스트를 **아직 수락하지 않았다면** 기록이 없어도 NPC 대화에서는 `Offered`로 보여야 합니다. 완료 보고 대기 상태에서는 기존 목표 대신 ‘보고’ 문구를 표시합니다. 이런 판단은 `QuestView`가 수행하고, 각 Widget은 만들어진 목표·상태·보상을 그립니다.

데이터가 변경되면 `OnDexChanged`·`OnQuestsChanged`가 위젯의 `Redraw()`를 호출합니다. 전체 목록은 다시 구성하더라도 현재 선택한 종·퀘스트 ID가 남아 있으면 선택을 유지합니다. 플레이어의 현재 진행 기록과 UI에서 선택한 항목은 별도 상태로 관리됩니다.

[16. MonsterDex](16_MonsterDex_System.md) · [17. Quest](17_Quest_System.md)

## 던전 HUD — 여러 출처의 데이터를 ViewData로 결합

던전 화면은 Stage 진행, 목표, 분기, 보스 행동, 파티 HP, 타이머, 미니맵을 한꺼번에 표시합니다. 이를 각 HUD Widget이 따로 조회하도록 만들지 않고 **`UDungeonStagePresenter`가 `FDungeonStageViewData`를 구성**합니다.

~~~mermaid
flowchart LR
    S["GameState<br/>DungeonStageRuntimeState"] --> P["DungeonStagePresenter"]
    D["DungeonPresentationDataAsset<br/>문구·아이콘·맵 범위"] --> P
    H["참가자 HealthComponent<br/>현재 HP"] --> P
    P --> V["DungeonStageViewData"]
    V --> R["DungeonUIRouterSubsystem"]
    R --> W["HUD · 미니맵 · 결과 Widget"]
~~~

![던전 표시 문구·아이콘·맵 범위를 설정한 DataAsset](../Media/Wiki/11_Client_State_Presentation_Pipeline/11_dungeon_presentation_asset.png)

`GameState`의 스냅샷에는 `StageId`·`RunTags`·목표 수치·`RunStatus`·보스 Phase와 서버 기준 시간이 있습니다. 표시 설정 DataAsset에는 해당 Stage의 제목·문구·아이콘·방 영역이 있습니다. Presenter는 두 값을 결합해 **Widget이 바로 사용할 제목·진행도·목표 Row·아이콘**을 채웁니다.

~~~cpp
// DungeonStagePresenter.cpp — Present() 발췌
View.Status = Latest.RunStatus;
View.Revision = Latest.Revision;
View.CountText = FText::Format(
    Texts.CountFormat, Latest.CurrentCount, Latest.RequiredCount);
View.Progress = Latest.RequiredCount > 0
    ? float(Latest.CurrentCount) / Latest.RequiredCount : 0.f;

if (const FDungeonStagePresentation* Stage =
        FindStage(Texts, Latest.StageId))
{
    View.Title = Stage->Title;
    View.Objective = Stage->Objective;
}
~~~

이렇게 하면 Stage ID나 현재 진행 수치를 읽어 표시 문구를 선택하는 처리를 HUD·Result 위젯마다 반복하지 않습니다. **던전의 실제 Stage 전환은 Runtime이, 표현 데이터 조합은 Presenter가, 위젯 생성·교체는 Router가** 담당합니다.

### 스테이지 진행·선택 목표·보상 표시

`UDungeonStagePresenter::Present()`는 화면 설정의 `Objectives`를 순회하면서 현재 스냅샷에 따라 `Open / Done / Missed` 상태와 목표 문구를 채웁니다. 메인 목표와 선택 목표는 별도 목록으로 구성합니다.

스테이지를 떠날 때 미완료 선택 목표를 바로 지우지 않고, `ResolveOptional()`이 완료·놓침을 잠시 보여준 뒤 제거합니다. 진행 경로(`Steps`)는 현재 Stage와 `SelectedBranchId`를 함께 사용해 해당 분기 이름·아이콘을 표시합니다.

성공·실패 이후에는 같은 Presenter가 **보상 아이템 목록·처치/포획 통계·경과 시간·선택 분기·클리어 등급과 최고 기록 비교**를 ViewData로 만듭니다. 보상 아이템은 기존 인벤토리 `USlotWidget::AddReadOnly`로 읽기 전용 표시할 수 있습니다.

### 파티 HP는 독립 컴포넌트에서 받기

던전 Snapshot에 참가자의 HP까지 중복 저장하지 않습니다. `BindMembers()`는 각 참가자 Pawn의 `UHealthComponent::OnHealthChanged`를 구독합니다.

~~~cpp
// DungeonStagePresenter.cpp — HP 변경 시 표시 갱신
void UDungeonStagePresenter::HandleMemberHealth(
    float CurrentHP, float Delta, float MaxHP)
{
    Present();
}
~~~

참가자 목록이나 Pawn이 변경되면 기존 Health Delegate를 해제하고 새 컴포넌트에 연결합니다. **진행 목표가 변경되지 않아도 파티 HP만 따로 갱신**할 수 있습니다.

## 던전 미니맵과 목표 마커

![원격 플레이어·현재 방·목표 마커가 표시된 미니맵](../Media/Wiki/11_Client_State_Presentation_Pipeline/11_minimap_multiplayer.png)

[멀티플레이 미니맵 시연](https://github.com/user-attachments/assets/fa7c4755-16dd-47fb-a558-80b74aa536b2)

`UDungeonPresentationDataAsset`은 던전 지도 텍스처·월드 범위·Stage별 방 영역을 가집니다. Presenter는 현재 Stage의 `bCurrent` 방 표시와 미완료 목표의 `MarkerTarget`을 구성합니다. Zone·레버는 `SourceId`로 월드 Actor를 조회하고, 찾은 대상은 약한 참조로 캐시합니다.

`FDungeonMarkerViewData`는 목표의 월드 위치·아이콘뿐 아니라 **접근 완료로 처리할 영역(`Arrival`)** 도 담습니다. 미니맵은 이 값을 받아 목표와 플레이어 위치를 표시합니다.

반면 플레이어의 현재 위치와 카메라 방향은 계속 변하므로 **미니맵 Widget이 실시간 Pawn·카메라를 읽어 좌표를 갱신**합니다. 고정 지도의 표시 설정과 이동하는 플레이어 위치를 구분합니다.

## 시간과 Snapshot 갱신

던전 진행 시간은 Widget의 생성 시점부터 재지 않습니다. Snapshot의 `StageDeadlineServerTime`·`RunStartedServerTime`·`BossActionStartServerTime / EndServerTime`과 동기화된 서버 시각을 비교해 남은 시간·진행도를 계산합니다.

`UDungeonStagePresenter::OnSnapshot()`은 같은 실행에서 이전 Revision을 다시 적용하지 않습니다.

~~~cpp
// DungeonStagePresenter.cpp
if (Snapshot.RunId == Latest.RunId &&
    Snapshot.Revision <= Latest.Revision) return;
~~~

새로운 `RunId`라면 새 실행으로 처리합니다. Presenter는 최신 Snapshot을 보관하기 때문에 늦게 로드된 에셋이 도착하거나 Widget이 새로 생성될 때도 **현재 상태로 화면을 다시 구성**할 수 있습니다.

![보스 HP·행동·Break·Phase가 표시된 HUD](../Media/Wiki/10_Boss_Encounter_Runtime/10_boss_status_hud.png)

보스 행동 명칭·아이콘은 표시 DataAsset에서 조회하고, 현재 보스 체력·Break·Phase·포획 가능 여부는 Snapshot의 수치와 플래그를 사용합니다. 위젯이 보스 FSM의 실행 규칙을 직접 판단하지 않습니다.

## HUD·결과 Widget 수명 관리

[던전 HUD 제거·재생성 시연](https://github.com/user-attachments/assets/6c4f9601-8fbe-4957-87d4-d4ad230f3211)

`UDungeonUIRouterSubsystem`은 현재 `FDungeonStageViewData`를 보관합니다. 같은 종류의 HUD가 살아 있으면 새로 만들지 않고 `ApplyViewData()`만 호출합니다. 성공·실패 상태가 되면 `UI.Dungeon.Result`로 교체합니다.

UI Registry와 위젯 클래스는 `RequestAsyncLoad`로 가져옵니다. 던전 종료 등으로 `Detach()`가 실행되면 `Generation`을 증가시키며 기존 요청이 뒤늦게 돌아와도 화면을 다시 만들지 않도록 합니다.

~~~cpp
// DungeonUIRouterSubsystem.cpp — 비동기 완료 콜백
const int32 Expected = Generation;
RegistryLoad = UAssetManager::GetStreamableManager().RequestAsyncLoad(
    RegistryAsset.ToSoftObjectPath(),
    FStreamableDelegate::CreateWeakLambda(this, [this, Expected]()
    {
        if (Expected != Generation) return;
        Registry = RegistryAsset.Get();
        RefreshWidget();
    }));
~~~

던전에서 사용하지 않을 기본 HUD는 `HiddenDuringRun` 목록으로 숨깁니다. 이때 각 위젯의 원래 `ESlateVisibility`를 저장해 던전 종료 시 **기존 표시 상태로 복원**합니다. 던전 HUD·결과는 일반 메뉴의 `UUIStackSubsystem`과 별도의 비모달 화면으로 관리해 메뉴 입력을 덮어쓰지 않습니다.

[중첩 메뉴·알림·입력 안내 구현](18_UMG_v2_Migration.md)

## Widget 공통화

인벤토리·보관함·제작·보상의 아이템 표현은 `USlotWidget`을 공유합니다. 보상에서는 `AddReadOnly`로 기존 아이콘·수량을 표시하되 드래그는 비활성화합니다. 제작 재료 Row는 부모 위젯의 로컬 풀에서 재사용하며, 전투 중 반복되는 Floating Damage는 별도 Actor 풀을 사용합니다.

이처럼 모든 UI를 전역 풀로 처리하지 않고 **재생성 빈도와 Widget 수명**에 맞춰 재사용 여부를 결정했습니다. 화면 레이아웃과 `BindWidget` 연결을 어떻게 수정했는지는 [18. UMG v2](18_UMG_v2_Migration.md)에서 다룹니다.

## 관련 코드

- [SonheimPlayerController.cpp](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Player/SonheimPlayerController.cpp) · [InventoryWidget.cpp](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/UI/Widget/Player/Inventory/InventoryWidget.cpp) — 기본 HUD·인벤토리 입력과 표시
- [MonsterDexView.cpp](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/UI/MonsterDex/MonsterDexView.cpp) · [QuestView.cpp](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/UI/Quest/QuestView.cpp) — 게임 기록에서 화면용 데이터 생성
- [DungeonStagePresenter.cpp](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/UI/Dungeon/DungeonStagePresenter.cpp) · [DungeonViewData.h](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/UI/Dungeon/DungeonViewData.h) — 던전 표시 데이터
- [DungeonUIRouterSubsystem.cpp](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/UI/Dungeon/DungeonUIRouterSubsystem.cpp) — HUD·Result 수명·비동기 로드
- [SlotWidget.cpp](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/UI/Widget/Player/Inventory/SlotWidget.cpp) — 공통 아이템 슬롯
- [12. Multiplayer Synchronization](12_Multiplayer_Synchronization.md) · [16. MonsterDex](16_MonsterDex_System.md) · [17. Quest](17_Quest_System.md) · [18. UMG v2](18_UMG_v2_Migration.md)
