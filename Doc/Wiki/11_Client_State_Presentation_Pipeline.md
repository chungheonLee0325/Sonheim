# 11. UI Architecture & Client Presentation

Sonheim UI는 **상태를 관리하는 게임플레이 코드와 화면을 표시하는 UMG의 역할을 구분**합니다. HP·인벤토리처럼 독립적인 상태는 변경 이벤트로 화면을 갱신하고, 던전처럼 여러 정보를 함께 표시하는 화면은 현재 상태를 조합해 ViewData를 구성합니다. 화면 입력과 알림 표시는 로컬 플레이어별 Subsystem에서 관리합니다.

## 핵심 구성

| 화면·기능 | 상태와 표시 데이터 | UI 갱신·관리 |
|---|---|---|
| **기본 HUD** | `UHealthComponent` · `UStaminaComponent` · `ULevelComponent` | 변경 Delegate → `UPlayerStatusWidget` |
| **인벤토리·보관함·제작** | 아이템·레시피 정의 + 인벤토리·제작 상태 | 화면별 Widget + `UGameUIManagerSubsystem` |
| **도감·퀘스트** | 정의 데이터 + 플레이어별 복제 기록 | `MonsterDexView` / `QuestView` → UMG |
| **던전 HUD·미니맵·결과** | 복제된 던전 Snapshot + 화면 설정·기존 컴포넌트 | `UDungeonStagePresenter` → ViewData → UI Router |
| **알림** | `FNoticeData` + Slot·Channel | `UNoticeSubsystem` → `UGameUIToastSubsystem` → `UNoticeWidget` |

**정의 데이터**는 콘텐츠 설정, **런타임 상태**는 실행 중 변하는 값, **ViewData**는 위젯에서 사용할 표시용 값입니다. 화면에 따라 단순 Delegate 갱신과 ViewData 조합을 선택해 사용합니다.

## 목차

- [화면별 데이터 흐름](#화면별-데이터-흐름)
- [기본 HUD와 상호작용 화면](#기본-hud와-상호작용-화면)
- [화면 스택과 입력 정책](#화면-스택과-입력-정책)
- [도감·퀘스트의 화면 데이터](#도감퀘스트의-화면-데이터)
- [알림 슬롯과 표시 정책](#알림-슬롯과-표시-정책)
- [던전 UI — Snapshot에서 ViewData까지](#던전-ui--snapshot에서-viewdata까지)
- [던전 UI의 수명 관리](#던전-ui의-수명-관리)
- [Widget 재사용과 설계 비용](#widget-재사용과-설계-비용)
- [관련 코드 및 문서](#관련-코드-및-문서)

---

## 화면별 데이터 흐름

~~~mermaid
flowchart TB
    COMPONENT["컴포넌트 상태<br/>Health · Inventory · Crafting"] --> CHANGE["상태 변경 Delegate"]
    CHANGE --> BASIC["기본 HUD · 상호작용 화면"]

    RECORD["플레이어별 기록<br/>MonsterDex · Quest"] --> FAST["OwnerOnly FastArray 복제"]
    FAST --> BUILD["MonsterDexView::Build / QuestView::Build"]
    DEF["DataTable 정의"] --> BUILD
    BUILD --> SCREEN["도감 · 퀘스트 UMG"]

    DUNGEON["서버 던전 진행"] --> SNAP["GameState<br/>FDungeonStageRuntimeState"]
    SNAP --> PRESENTER["UDungeonStagePresenter"]
    PRESENT["화면 설정 DataAsset<br/>문구 · 아이콘 · 미니맵"] --> PRESENTER
    PRESENTER --> VIEW["FDungeonStageViewData"]
    VIEW --> ROUTER["UDungeonUIRouterSubsystem"]
    ROUTER --> DUNGEONUI["던전 HUD · 미니맵 · 결과"]

    EVENT["게임플레이 알림"] --> NOTICE["UNoticeSubsystem"]
    NOTICE --> TOAST["Feed · Headline · Title"]
~~~

위젯은 **게임플레이의 원본 상태를 소유하지 않습니다.** 선택 항목이나 스크롤 위치 같은 UI 자체의 상태는 위젯에 둘 수 있지만, HP·인벤토리·퀘스트 기록은 게임플레이 컴포넌트가 관리합니다. 던전 진행 상태는 서버에서 처리한 뒤 `GameState`로 전달하며, 화면을 다시 열 때 필요한 값을 읽어 표시합니다.

---

## 기본 HUD와 상호작용 화면

### C++ Widget과 Widget Blueprint의 역할

Sonheim의 C++ Widget은 데이터 연결·입력 이벤트·버튼 동작을 구현하고, Widget Blueprint는 배치·스타일·애니메이션을 담당합니다. 필요한 하위 위젯은 `UPROPERTY(meta=(BindWidget))`으로 연결합니다.

~~~text
C++ UUserWidget
 ├─ BindWidget : 필요한 하위 Widget 명세
 ├─ Gameplay Component / Delegate 연결
 └─ 클릭·드래그·서버 요청 처리
           ↕
Widget Blueprint
 ├─ Widget Tree · Layout
 ├─ 이미지·색상·스타일
 └─ Animation
~~~

`BindWidget`은 C++ 멤버와 Widget Blueprint의 실제 이름·타입이 맞아야 동작합니다. AgentMcp에서 Widget Tree를 수정할 때도 이 연결을 확인합니다. 에디터를 이용한 UI 개편 과정 자체는 [[18. UMG v2 Migration|18_UMG_v2_Migration]]에서 다룹니다.

### 기본 HUD — 변경 이벤트 구독

HP·스태미나·레벨은 각 컴포넌트가 상태를 관리하고 변경 Delegate를 전달합니다.

~~~text
UHealthComponent::OnHealthChanged ──┐
UStaminaComponent 변경 이벤트       ├─→ UPlayerStatusWidget
ULevelComponent::OnLevelChanged  ──┘
~~~

`ASonheimPlayerController`는 상태 HUD와 `UHealthComponent`의 변경 이벤트를 연결하고 초기 HP 값도 적용합니다. 따라서 HP가 변경될 때마다 위젯이 자체적으로 게임플레이 값을 계속 조회하는 구조가 아닙니다.

### 인벤토리 — 아이템 기록과 정의 결합

`UInventoryComponent`는 슬롯·아이템 ID·수량 같은 플레이어 상태를 관리합니다. `UInventoryWidget`은 이 값을 읽고 `USonheimGameInstance::GetDataItem`에서 `FItemData`를 조회해 아이콘·이름 등을 `USlotWidget`에 적용합니다.

~~~mermaid
flowchart LR
    INV["UInventoryComponent<br/>ItemID · Count · Slot"] --> UI["UInventoryWidget"]
    DATA["FItemData<br/>이름 · 아이콘 · 설정"] --> UI
    UI --> SLOT["USlotWidget<br/>인벤토리·장비 슬롯"]
~~~

아이템 수량과 장착 상태를 위젯에 별도로 원본 상태로 보관하지 않습니다. 슬롯 이동이나 장비 변경은 인벤토리 API로 요청하고, 변경된 결과를 화면에 반영합니다.

### 보관함·제작 — 기존 슬롯 표시 재사용

- **보관함:** `UContainerInteractionWidget`이 플레이어 인벤토리와 보관함 인벤토리를 함께 보여줍니다. 아이템 이동은 각 데이터 관리 주체에 요청하며, 기존 Slot·Drag & Drop 표현을 재사용합니다.
- **제작:** 레시피 DataTable, 플레이어 인벤토리, 제작대의 진행 상태를 조합합니다. 레시피 선택 시 재료 목록을 만들고, 보유 수량과 작업 진행도가 바뀌면 화면을 갱신합니다. 재료 행은 부모 위젯의 풀에서 재사용합니다.

### 확인 팝업 — 게임플레이 요청과 분리

아이템 버리기·폐기에는 `UInventoryWidget`이 `UI.Popup.ItemQuantity` 팝업(`UConfirmWidget`)을 열어 아이템과 수량을 확인합니다.

~~~text
버리기 / 폐기 영역에 드롭
       ↓
UInventoryWidget::HandleTrashDrop
 └─ FItemQuantityView(아이콘·이름·최대 수량·폐기 여부)로 팝업 열기
       ↓
UConfirmWidget (Popup 레이어)
 └─ 수량 선택 → Finish(확인 / 취소)
       ↓ OnResult
UInventoryWidget
       ↓
InventoryComponent의 버리기 / 폐기 요청
~~~

확인 팝업은 인벤토리 상태를 변경하지 않고, 아이템 데이터도 직접 조회하지 않습니다. 인벤토리가 넘긴 표시 데이터를 그리고, 결과를 콜백으로 한 번만 돌려줍니다. 팝업의 수명과 결과 전달 방식은 [21. Game UI Framework](21_UI_Framework.md#팝업-결과를-한-번만-전달하기)에서 설명합니다.

인벤토리·보관함·제작의 게임플레이 처리와 복제 구조는 [[07. Inventory & Crafting|07_Inventory_Crafting]]에서 설명합니다.

---

## 화면 스택과 입력 정책

여러 화면이 각자 `SetInputMode`와 커서 표시를 변경하면, 중첩된 화면을 닫을 때 입력 상태가 잘못 복원될 수 있습니다. 이를 줄이기 위해 화면의 열림·닫힘과 입력 정책은 `GameUIFramework` 플러그인의 `UGameUIManagerSubsystem : ULocalPlayerSubsystem`에서 처리합니다. 화면마다 입력 정책은 Widget Blueprint의 클래스 기본값으로 둡니다.

~~~cpp
// GameUIScreenWidget.h — 클래스 기본값 발췌
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Screen", meta=(Categories="UI.Layer"))
FGameplayTag Layer;
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Screen") EGameUIInputMode InputMode = EGameUIInputMode::GameAndUI;
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Screen") bool bShowMouse = true;
// …
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Screen") bool bBlockGameInput = true;
// …
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Screen") bool bKeepWhenClosed = false;
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Screen") EGameUIBack Back = EGameUIBack::Close;
~~~

화면은 `UI.Screen.*` 태그로 열고, 태그와 Widget Blueprint의 연결은 레지스트리 `DA_UIScreens`에 둡니다.

| 동작 | 실제 처리 |
|---|---|
| `Open` | 화면 생성 또는 이미 열린 화면을 스택 맨 위로 이동. 클래스가 메모리에 없으면 비동기 로드 |
| `Close / CloseTop` | 화면 제거 후 남은 맨 위 화면의 설정 적용 |
| `HandleBack` | 맨 위 화면의 `Back` 설정에 따라 닫기·무시·화면별 처리 |
| `ApplyInputState` | `SetInputMode`·커서 표시 적용, `OnInputStateChanged`·`OnTopScreenChanged` 알림 |
| `IsGameplayBlocked` | 플레이어 이동·공격·상호작용 입력에서 게임플레이 차단 여부 확인 |

인벤토리와 능력치는 `WBP_InventoryScreen` 한 화면 안에 함께 배치합니다. 열린 화면이 없으면 `GameOnly`와 커서 숨김 상태로 복원합니다. 레이어 구성과 화면 수명은 [21. Game UI Framework](21_UI_Framework.md)에서 설명합니다.

**입력 모드와 게임플레이 입력 차단은 별개입니다.** 현재 일반 Screen은 `GameAndUI`를 사용하면서 `IsGameplayBlocked`로 이동·공격 등의 입력을 막습니다. `GameAndUI` 설정 자체가 모든 게임플레이 입력을 자동 차단하지는 않습니다.

### 키보드·게임패드 안내

`UInputHintSubsystem`은 Enhanced Input에 매핑된 키를 조회해 입력 안내를 만듭니다. Slate 입력을 바탕으로 마지막 사용 장치가 키보드·마우스인지 게임패드인지 구분하고, 입력 매핑이 갱신되면 글리프도 갱신합니다.

메뉴 탭의 좌우 이동에 사용하는 입력 안내도 이 경로를 활용합니다. `UInputHintSubsystem`은 UI 매니저의 `OnInputStateChanged`를 받아 메뉴 `Input Mapping Context`를 켜고 끕니다.

---

## 도감·퀘스트의 화면 데이터

도감과 퀘스트는 플레이어별 기록을 관리한다는 점이 공통적입니다. `ASonheimPlayerState::ReportDeed`가 서버에서 `Kill / Capture / Craft / Reach / ClearDungeon` 이벤트를 전달하고, 각 컴포넌트가 필요한 이벤트만 집계합니다.

~~~mermaid
flowchart TB
    DEED["ReportDeed → OnDeed"] --> DEX["UMonsterDexComponent<br/>FMonsterDexEntry"]
    DEED --> QUEST["UQuestComponent<br/>FQuestRecord"]
    DEX --> REPL["소유 클라이언트<br/>FastArray 변경분"]
    QUEST --> REPL
    REPL --> CHANGE["OnDexChanged / OnQuestsChanged"]
    CHANGE --> BUILD["MonsterDexView::Build / QuestView::Build"]
    DT["DT_MonsterDex / DT_Quest"] --> BUILD
    BUILD --> UI["도감 · 일지 · NPC 대화 · HUD 추적기"]
~~~

| 구분 | 도감 | 퀘스트 |
|---|---|---|
| 정의 데이터 | `FMonsterDexData` — 종·보상 조건 | `FQuestData` — 단계·목표·보상 |
| 플레이어 기록 | `FMonsterDexEntry` — 처치·포획·수령 | `FQuestRecord` — 현재 단계·진행도 |
| 화면 데이터 | `FMonsterDexBookView` / `FMonsterDexPageView` | `FQuestView` / `FQuestObjectiveView` |
| 서버 검증 | 보상 조건·중복 수령 | 수락·보고 조건과 NPC 대상·거리 |

**도감·퀘스트의 정의 데이터는 각각의 GameInstanceSubsystem에서 로드하고, 플레이어별 진행 기록은 PlayerState의 Component가 관리**합니다. 클라이언트에는 FastArray의 변경분을 `COND_OwnerOnly`로 복제합니다. 저장이 필요한 기록은 서버가 호스트 SaveGame에 별도로 기록합니다.

도감은 첫 발견 여부를 기록 존재로 판단하고, 퀘스트는 진행 상태를 `QuestView::Build`에서 화면 표시 상태로 바꿉니다. UI는 변경 Delegate를 받은 후 정의 데이터와 현재 기록을 조합해 다시 그립니다.

도감 등록·보상·3D 프리뷰는 [[16. MonsterDex System|16_MonsterDex_System]], 퀘스트 목표 집계·단계 전환·NPC 상호작용은 [[17. Quest System|17_Quest_System]]에서 설명합니다.

---

## 알림 슬롯과 표시 정책

레벨 상승, 첫 포획, 퀘스트 단계 갱신, 제작 완료, 던전 진행 알림은 현재 상태를 대신 저장하지 않는 **일회성 UI 피드백**입니다. `UNoticeSubsystem : ULocalPlayerSubsystem`이 메시지를 받아 UI 루트의 Toast 레이어 슬롯으로 보냅니다.

~~~text
게임플레이 코드 / 클라이언트 이벤트
             ↓
FNoticeData + ENoticeSlot + Channel
             ↓
UNoticeSubsystem  ─ ENoticeSlot을 UI.Toast.* 태그로 바꾸고 FInstancedStruct에 담음
             ↓
UGameUIToastSubsystem  ─ 슬롯별 순서 정책·채널 회수
             ↓
UNoticeWidget / Widget Blueprint (Toast 레이어)
~~~

`FNoticeData`는 제목·분류·설명·아이콘·강조 색상 역할(`Tone`) 등의 정보를 담습니다. 실제 배치와 출력 방식은 슬롯 설정과 Widget Blueprint가 맡습니다.

### 슬롯별 동작

현재 프로젝트 설정 **Game > Game UI**의 `ToastSlots`(`DefaultGame.ini`)에 설정된 정책입니다.

| 슬롯 | 배치·용도 | 표시 방식 |
|---|---|---|
| **Feed** | 화면 왼쪽의 퀘스트·제작·진행 알림 | `Stack` — 새 알림을 즉시 추가 |
| **Headline** | 화면 중앙의 레벨업·첫 발견 등 주요 알림 | `TakeTurns` — 순서대로 표시, 최대 대기 3건 |
| **Title** | 화면 상단의 지역명 | `ReplaceShowing` — 표시 중인 알림 교체 |

`UNoticeSubsystem::Push`는 `Slot`과 `Channel`을 받아 알림을 전달합니다. 슬롯별 Widget 클래스와 표시 순서는 `FGameUIToastSlot`에 정의하고, 슬롯 위젯은 설정 순서대로 Toast 레이어에 붙습니다. `UNoticeStyle`은 각 알림 Widget Blueprint의 기본값입니다.

`Channel`은 특정 기능에서 발생한 알림을 한꺼번에 정리하기 위한 식별자입니다. 예를 들어 던전에서 던전 채널로 `UNoticeSubsystem::Clear`를 호출하면 던전 알림만 정리하고 다른 기능의 알림은 유지합니다.

### 알림 시연 영상

**이전 버전의 던전 알림 큐**

https://github.com/user-attachments/assets/bc9e4305-62f6-43bb-b1fa-2b109a06eb5b

*레버·몬스터 그룹·분기 이벤트의 배너가 차례로 표시되는 개편 전 시연입니다. 현재 `Feed`는 `Stack` 방식이므로, 위 영상은 현행 Feed 배치나 동시 표시 정책의 증거로 사용하지 않습니다.*

현재 Feed·Headline·Title 화면 비교는 [[18. UMG v2 Migration|18_UMG_v2_Migration]]에서 확인할 수 있습니다.

### 일반 알림과 아이템 획득 팝업

아이템 슬롯 갱신과 아이템 획득 알림은 동일한 사건이 아닙니다.

~~~text
Inventory.OnInventoryChanged → 슬롯 화면 갱신

Inventory.OnItemAdded
   → PlayerController Client RPC
   → PlayerStatusWidget.DisplayItemPopup
~~~

`OnItemAdded`를 별도로 사용하므로 슬롯 교체나 장착 해제가 아이템 획득 팝업을 발생시키지 않습니다. 이 기존 HUD 팝업은 `UNoticeSubsystem`의 Feed와 다른 경로입니다.

---

## 던전 UI — Snapshot에서 ViewData까지

던전 HUD는 스테이지·메인/선택 목표·파티·보스·미니맵을 한 화면에 표시하고, 종료 후에는 같은 진행 결과를 바탕으로 보상·등급·기록 화면을 구성합니다.

![던전 HUD: 목표, 타이머, 파티, 미니맵](../Media/Wiki/11_Client_State_Presentation_Pipeline/11_dungeon_hud.png)

*스테이지·목표·파티·마커·미니맵을 함께 보여주는 기존 HUD 화면입니다. v2 화면 비교는 [[18. UMG v2 Migration|18_UMG_v2_Migration]]에서 다룹니다.*

### 서버 스냅샷과 화면 설정 결합

~~~mermaid
flowchart TB
    SERVER["UDungeonStageRuntimeSubsystem<br/>서버 던전 진행"] --> STATE["ASonheimGameState<br/>FDungeonStageRuntimeState"]
    STATE --> PRES["UDungeonStagePresenter"]
    DEF["UDungeonPresentationDataAsset<br/>문구·아이콘·맵 영역"] --> PRES
    HEALTH["각 참가자의 UHealthComponent<br/>현재 HP"] --> PRES
    PRES --> VIEW["FDungeonStageViewData<br/>화면 표시 데이터"]
    VIEW --> ROUTER["UDungeonUIRouterSubsystem<br/>위젯 수명 관리"]
    ROUTER --> WIDGET["HUD · 미니맵 · 결과 UMG"]
~~~

`UDungeonStageRuntimeSubsystem`은 서버에서 단계를 진행하고, `ASonheimGameState`는 `FDungeonStageRuntimeState`를 복제합니다. `UDungeonStagePresenter`는 이 스냅샷과 `UDungeonPresentationDataAsset`을 읽어 `FDungeonStageViewData`를 만듭니다.

| 데이터 출처 | 화면에 사용하는 값 |
|---|---|
| `FDungeonStageRuntimeState` | `StageId`·그룹 진행도·분기·보스 상태·서버 기준 시간 |
| `UDungeonPresentationDataAsset` | 스테이지·목표 문구, 아이콘, 미니맵 텍스처·방 범위 |
| `UHealthComponent` | 던전 참가자의 현재 HP |
| `FDungeonStageViewData` | UI에 직접 전달할 제목·진행도·파티 정보·마커·결과 |

![던전 UI 화면 설정 DataAsset](../Media/Wiki/11_Client_State_Presentation_Pipeline/11_dungeon_presentation_asset.png)

*화면 문구·아이콘·미니맵 설정을 데이터 에셋에서 관리합니다. 스테이지 전환 규칙은 별도의 던전 정의 에셋에 있습니다.*

가령 `StageId`가 `Dungeon.ForgottenRuins.Stage.GuardRoom`이고 진행도가 2/4라면, Presenter가 화면 설정의 스테이지 이름·목표 문구·아이콘을 찾아 진행도와 조합합니다. **Widget은 스테이지 전환 규칙을 판단하지 않고 전달받은 결과를 표시**합니다.

### 오래된 Snapshot 제외

같은 실행에서 이전 상태가 화면을 되돌리지 않도록 `RunId`와 `Revision`을 비교합니다.

~~~cpp
// DungeonStagePresenter.cpp — OnSnapshot
if (Snapshot.RunId == Latest.RunId &&
    Snapshot.Revision <= Latest.Revision)
{
    return;
}
~~~

다른 `RunId`라면 새로운 실행 상태로 받아들이고, 같은 실행에서는 더 높은 `Revision`만 반영합니다.

### 파티 HP는 기존 컴포넌트 활용

던전 진행도는 `GameState` 스냅샷에서 읽지만, 파티 HP는 각 참가자 Pawn의 `UHealthComponent`에서 가져옵니다. `UDungeonStagePresenter::BindMembers`는 `OnHealthChanged`를 구독해 HP 변경 시 ViewData를 다시 만듭니다.

따라서 **던전 화면을 위해 파티 HP를 던전 스냅샷에 별도로 복제하지 않습니다.**

### 미니맵·목표 마커

`UDungeonPresentationDataAsset`에는 지도 텍스처·월드 범위·방 영역과 표시 아이콘을 정의합니다. Presenter는 현재 스테이지와 진행 중인 목표를 바탕으로 방 강조 및 목표 마커를 구성합니다.

![멀티플레이 미니맵: 두 플레이어와 목표 마커](../Media/Wiki/11_Client_State_Presentation_Pipeline/11_minimap_multiplayer.png)

*선택한 스테이지 방 영역·로컬 플레이어 화살표·원격 플레이어 위치·레버 목표 마커가 표시된 화면입니다.*

**멀티플레이 미니맵 영상**

https://github.com/user-attachments/assets/fa7c4755-16dd-47fb-a558-80b74aa536b2

*원격 플레이어의 던전 밖 이동과 복귀, 카메라 회전에 따른 로컬 화살표 방향 갱신을 확인할 수 있습니다.*

현재 플레이어 위치·카메라 방향처럼 계속 바뀌는 값은 미니맵 Widget이 월드의 Pawn·카메라를 읽어 좌표로 변환합니다. **고정 지도·목표 데이터와 실시간 위치 갱신을 구분한 구성**입니다.

### 서버 시간 기준 타이머

스냅샷의 `RunStartedServerTime`, `StageDeadlineServerTime`, `BossActionStartServerTime`, `BossActionEndServerTime`은 동기화된 서버 시간과 비교하는 기준입니다.

남은 시간이나 보스 행동 진행도를 위젯 생성 시점부터 새로 계산하지 않습니다. HUD가 늦게 열리거나 재생성되더라도 **같은 서버 기준 시각을 사용**합니다.

### 보스 상태와 결과 화면

![보스 상태 HUD](../Media/Wiki/10_Boss_Encounter_Runtime/10_boss_status_hud.png)

*보스 HP·행동·페이즈·Break·포획 가능 여부를 스냅샷에서 가져와 표시합니다. 보스 FSM을 Widget이 직접 참조해 판단하지 않습니다.*

성공·실패로 실행이 끝나면 `RunStatus`에 따라 HUD 대신 결과 화면을 선택합니다. 최종 스냅샷의 시간·처치/포획·분기·보상·최고 기록과 정의된 등급 규칙을 조합해 결과 ViewData를 만듭니다.

![던전 결과 화면](../Media/Wiki/11_Client_State_Presentation_Pipeline/11_dungeon_result.png)

*현재 실행의 최종 기록을 바탕으로 결과 화면을 구성합니다. UI를 다시 열 때도 완료된 Snapshot의 값을 사용합니다.*

---

## 던전 UI의 수명 관리

`UDungeonUIRouterSubsystem : ULocalPlayerSubsystem`은 던전 HUD와 결과 위젯을 생성·교체합니다. `UDungeonStagePresenter`가 전달한 **최신 `FDungeonStageViewData`를 Router가 보관**하므로, 위젯이 없어져도 표시할 상태를 다시 구성할 수 있습니다.

### 위젯 제거 후 상태 복구

**던전 HUD 재생성 영상**

https://github.com/user-attachments/assets/6c4f9601-8fbe-4957-87d4-d4ad230f3211

*진행 중인 HUD를 `RemoveFromParent`한 뒤 새 위젯에 같은 실행의 목표·미니맵·타이머 상태를 복원한 시연입니다.*

위젯이 다시 생성되면 Router가 보관한 최신 ViewData를 적용합니다. **진행 중인 던전의 실제 상태는 위젯이 아니라 게임플레이 시스템과 스냅샷에 유지**됩니다.

### 비동기 로딩과 이전 콜백 방지

UI Registry와 HUD/Result Widget 클래스는 `RequestAsyncLoad`로 로드합니다. 로딩 중 화면 연결이 종료되거나 새로 시작되면 기존 완료 콜백이 뒤늦게 도착할 수 있습니다.

`Generation`을 증가시키고 콜백에서 요청 시점과 현재 값을 비교해, 이전 연결에서 시작된 비동기 로딩 결과가 현재 화면에 반영되지 않게 합니다.

~~~cpp
// DungeonUIRouterSubsystem.cpp — 핵심 조건
const int32 Expected = Generation;

// 비동기 로드 완료 콜백
if (Expected != Generation) return;
~~~

### 기존 HUD의 표시 상태 복원

던전에 참가해 진행 중인 동안 `HiddenDuringRun`에 지정된 기존 HUD를 숨깁니다. 숨기기 전 각 위젯의 `ESlateVisibility`를 저장하며, 던전 종료·이탈 시 **원래 값으로 복구**합니다. 진행 중 새로 생성된 대상 위젯도 화면 갱신 시 확인합니다.

일반 인벤토리·도감·퀘스트 메뉴는 `UGameUIManagerSubsystem`이 관리하지만, 던전 HUD/Result는 `UDungeonUIRouterSubsystem`이 고릅니다. 라우터는 고른 위젯을 매니저의 `AddToLayer`로 HUD 레이어에 붙이며, 던전 HUD·결과 화면은 비모달 `GameOnly` 방식이라 일반 메뉴 입력 정책을 덮어쓰지 않습니다.

---

## Widget 재사용과 설계 비용

### 생성 빈도에 따른 재사용

| 대상 | 적용 방식 | 이유 |
|---|---|---|
| `USlotWidget` | 인벤토리·장비·보관함·제작·보상 UI에서 공통 사용 | 아이템의 이름·아이콘·수량 표현 공유 |
| 제작 재료 행 | 부모 위젯의 로컬 풀 | 레시피 교체 때 반복 생성 감소 |
| Floating Damage | Actor 풀 | 전투 중 반복되는 데미지 표시 생성 비용 감소 |

아이템 보상의 읽기 전용 표시에는 `USlotWidget::AddReadOnly`도 사용합니다. 모든 위젯을 같은 전역 풀로 관리하지 않고 **생성 빈도와 부모 화면의 수명에 맞춰** 재사용 방식을 선택했습니다.

### 설계 선택과 비용

| 선택 | 효과 | 추가로 관리할 부분 |
|---|---|---|
| **Delegate 기반 갱신** | 독립 상태 변경을 즉시 UI에 전달 | 바인딩·해제·초기값 적용 |
| **C++ Widget + WBP** | 데이터·동작과 화면 배치 분리 | `BindWidget` 이름·타입 일치 |
| **GameUIManagerSubsystem** | 레이어별 화면 스택, 입력·커서 복원 일원화 | 화면 태그·레지스트리, 열기·닫기 시점의 구독 관리 |
| **NoticeSubsystem + GameUIToastSubsystem** | 알림 슬롯·표시 순서·채널 분리 | 슬롯별 표시 제한·알림 수명 |
| **Presenter + ViewData** | 복합 상태를 화면용 데이터로 조합 | 상태·표시 정의 간 매핑 코드 |
| **DungeonUIRouterSubsystem** | 비동기 로딩·HUD 교체·상태 복구 | 이전 콜백·위젯 생명주기 |

단순 HUD에 별도 Presenter를 만들지 않았고, 던전과 도감·퀘스트도 화면 특성에 맞는 표시 데이터 생성 방식을 사용합니다. 공통 입력·알림은 Subsystem으로 분리하되 **게임플레이 상태의 변경 권한은 원래 시스템에 유지**했습니다.

---

## 관련 코드 및 문서

- [SonheimPlayerController.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/AreaObject/Player/SonheimPlayerController.cpp) — 기본 HUD와 입력 연결
- [InventoryWidget.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/UI/Widget/Player/Inventory/InventoryWidget.cpp) — 슬롯 표시·드래그·확인 팝업
- [ConfirmWidget.h](../../Sonheim/Source/Sonheim/UI/Widget/Player/Inventory/ConfirmWidget.h) — 수량 확인 팝업·결과 반환
- [GameUIManagerSubsystem.cpp](../../Sonheim/Plugins/GameUIFramework/Source/GameUIFramework/Private/GameUIManagerSubsystem.cpp) — 화면 스택·입력 모드
- [InputHintSubsystem.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/UI/Input/InputHintSubsystem.cpp) — 키보드·패드 안내
- [NoticeSubsystem.cpp](../../Sonheim/Source/Sonheim/UI/Notice/NoticeSubsystem.cpp) · [GameUIToastSubsystem.cpp](../../Sonheim/Plugins/GameUIFramework/Source/GameUIFramework/Private/GameUIToastSubsystem.cpp) — 슬롯별 알림 표시
- [MonsterDexView.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/UI/MonsterDex/MonsterDexView.cpp) · [QuestView.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/UI/Quest/QuestView.cpp) — 플레이어 기록을 UI 표시 데이터로 변환
- [DungeonStagePresenter.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/UI/Dungeon/DungeonStagePresenter.cpp) — 던전 ViewData 구성
- [DungeonUIRouterSubsystem.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/UI/Dungeon/DungeonUIRouterSubsystem.cpp) — HUD·결과 UI 수명
- [DungeonUIRegistryDataAsset.h](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/UI/Dungeon/DungeonUIRegistryDataAsset.h) — 던전 UI 레지스트리
- [[02. Gameplay Architecture|02_Gameplay_Architecture]] — 상태 관리와 시스템 수명
- [[03. Data & Content Architecture|03_Data_Content_Architecture]] — 정의 데이터·식별자·에셋
- [[07. Inventory & Crafting|07_Inventory_Crafting]] — 인벤토리·제작 데이터 처리
- [[09. Branching Dungeon Runtime|09_Branching_Dungeon_Runtime]] — 던전 스테이지 실행
- [[12. Multiplayer Synchronization|12_Multiplayer_Synchronization]] — 상태 복제·RPC
- [[16. MonsterDex System|16_MonsterDex_System]] · [[17. Quest System|17_Quest_System]] — 도감·퀘스트
- [[18. UMG v2 Migration|18_UMG_v2_Migration]] — 현재 화면 개편·에디터 검증
