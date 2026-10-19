# GameUIFramework 설계

Sonheim의 UI를 게임에 묶이지 않는 플러그인 위에 올린다. 지금의 `UUIStackSubsystem`을 키워 UI 매니저로 만들고, 화면을 레이어·기반 클래스·태그 레지스트리로 정리한다. CommonUI와 MVVM 플러그인은 쓰지 않는다.

## 1. 지금 구조와 바꿀 점

| 지금 | 문제 | 바뀐 뒤 |
|---|---|---|
| `UUIStackSubsystem`이 `AddToViewport(ZOrder)`로 화면을 띄움 | 순서가 ini 숫자에 흩어짐 (메뉴 0, 상자 1, 도감 2, 탭바 50, 알림 2050~2150, 던전 1800/1900) | 루트 레이아웃 한 장(`WBP_UIRoot`)의 레이어 슬롯에 넣음 |
| 화면 규칙이 `DefaultGame.ini`의 `FUIScreenRule` 목록 | 위젯과 규칙이 떨어져 있고, 일부 화면은 호출하는 쪽이 클래스를 넘김 (`InventoryWidgetClass` 등) | 규칙은 위젯 클래스 기본값, 클래스는 태그 레지스트리 |
| 화면 이름이 `FName` (`UIScreen::Menu`) | 오타가 런타임 ensure로만 드러남 | `UI.Screen.*` Gameplay Tag |
| 열기가 `LoadSynchronous` | 도감 첫 열기에 종 클래스 동기 로드 약 120 ms | 비동기 로드, 로딩 상태, 미리 로드 |
| 위젯 약 45개가 `UUserWidget` 직속 | 열림/닫힘 시점이 `NativeConstruct`/`NativeDestruct`에 섞임 (유지 화면에서 렌더 타깃 누수가 났던 원인) | 화면 기반 클래스의 `OnOpened`/`OnClosed` |
| 팝업(`UConfirmWidget`)이 인벤토리 안에서 `AddToViewport()` | 입력 모드·뒤로 가기가 스택 밖 | Popup 레이어 + 결과 콜백 |
| 알림이 `AddToPlayerScreen(2100)` | 큐 로직이 게임 모듈 안 | Toast 레이어 큐는 플러그인, 알림 내용은 게임 |

## 2. 레이어와 위젯 종류

두 축은 따로다. 레이어는 **어디에, 어떤 규칙으로** 놓이는가. 위젯 종류는 **매니저와 어떤 수명 약속을** 하는가.

| 레이어 (태그) | 레이어 클래스 | 겹침 | 한 레이어 안 | 입력·뒤로 가기 |
|---|---|---|---|---|
| `UI.Layer.HUD` | `UGameUILayer` | 맨 아래 | 붙인 것 전부 보임 | 안 낌 |
| `UI.Layer.Menu` | `UGameUIStackLayer` | 그 위 | 맨 위 하나만 보임 | 낌 |
| `UI.Layer.Popup` | `UGameUIStackLayer` | 그 위 | 맨 위 하나만 보임 | 낌, Menu보다 먼저 |
| `UI.Layer.Toast` | `UGameUILayer` | 맨 위 | 붙인 것 전부 보임, HitTestInvisible | 안 낌 |

| 위젯 종류 | 누가 여는가 | 언제 닫히는가 | 매니저와의 약속 |
|---|---|---|---|
| HUD (`UUserWidget`) | 게임 쪽이 `AddToLayer` 한 번 | 안 닫힘 | 없음 |
| Screen (`UGameUIScreenWidget`) | 누군가 태그로 `Open` | 닫기·뒤로 가기·`CloseSelf` | `OnOpened`/`OnClosed`, `OnCovered`/`OnUncovered`, 입력 모드, 뒤로 가기, 유지 |
| Popup (`UGameUIPopupWidget`) | Screen과 같음 | Screen과 같음 | Screen 약속 + 결과 콜백 한 번 |
| Toast (`UGameUIToastWidget`) | 아무도 안 엶. 내용을 큐에 넣음 | 시간이 지나면 스스로 | `ShowToast`/`Stop`/`OnFinished` |

Popup은 Screen의 한 종류라 어느 스택 레이어에도 놓일 수 있다. Popup 레이어에 두는 것은 기본값일 뿐이다. Toast는 열고 닫는 화면이 아니라(위젯 하나가 여러 알림을 동시에 쌓기도 함) Screen을 상속하지 않는다.

```
APlayerController (로컬)
└─ GameViewport
   └─ WBP_UIRoot  (UGameUIRootWidget, 뷰포트에 한 번)
      ├─ UGameUILayer       Tag=UI.Layer.HUD     상태창, 퀘스트 추적, 미니맵, 던전 HUD
      ├─ UGameUIStackLayer  Tag=UI.Layer.Menu    인벤토리, 도감, 지도, 퀘스트, 상자, 제작대
      ├─ WBP_MenuTabBar     (게임 쪽 요소)       매니저 이벤트를 듣고 스스로 보이고 숨음
      ├─ UGameUIStackLayer  Tag=UI.Layer.Popup   수량 확인 …
      ├─ UGameUILayer       Tag=UI.Layer.Toast   알림 슬롯 (Feed/Title/Headline)
      └─ LoadingIndicator   (BindWidgetOptional) 비동기 열기 대기 중에만 보임
```

- 레이어는 플러그인의 패널 위젯(`UOverlay` 파생)이다. WBP 디자이너 팔레트에서 루트에 끌어다 놓고 디테일 패널에서 `Tag`를 고른다. 루트는 `NativeOnInitialized`에서 위젯 트리를 돌며 `UGameUILayer`들을 태그로 등록한다. 플러그인은 레이어 개수와 이름을 모른다.
- 레이어의 동작은 레이어 클래스가 정한다. 쌓을지, 입력에 낄지를 bool로 두지 않는다. 다른 동작(차례로 하나씩 보여 주는 큐 레이어 등)이 필요하면 클래스를 하나 더 만든다.
- 겹침 순서 = 루트 안 자식 순서. 입력 우선순위 = 같은 순서의 역순. Z-order 숫자는 없어진다.
- 스택 레이어 안에서는 맨 위 화면만 보인다. 그 아래 화면은 스택에 남은 채 `Collapsed`가 되고 `OnCovered`를 받는다. 위 화면이 닫히면 다시 보이고 `OnUncovered`를 받는다. 가려진 화면은 닫힌 것이 아니라서 `OnClosed`는 부르지 않는다.
- 열려 있는 화면을 다시 열면 스택 맨 위로 올린다. 떼었다가 다시 붙인다 (지금과 같음).
- 입력 모드·마우스·게임 입력 차단은 **입력에 끼는 레이어 중 가장 위 레이어의 맨 위 화면**이 정한다. 그런 화면이 없으면 GameOnly.
- 뒤로 가기(Esc / 게임패드 B)도 같은 화면 하나에만 간다.
- 루트는 `AddToPlayerScreen`으로 붙어 분할 화면에서도 플레이어마다 제 화면을 가진다. HUD, 메뉴, 팝업, 알림이 모두 루트 안에 있어 Z-order 숫자를 쓰지 않는다.
- 루트는 로컬 플레이어마다 하나. 맵이 바뀌어 루트가 사라지면 다음 요청 때 다시 만든다. 이전 컨트롤러의 유지 화면은 버린다 (지금 `Kept` 검사와 같음).
- 화면이 놓일 레이어는 화면 클래스 기본값 `Layer`. 여는 쪽이 레이어를 덮어쓰는 인자는 같은 화면을 두 레이어에 띄울 일이 생기면 그때 더한다.
- 화면 태그(`UI.Screen.*`)와 레이어 태그(`UI.Layer.*`)는 따로 둔다. 화면 태그는 이름, 레이어는 그 화면의 속성이다. `UI.Screen.Menu.Inventory`처럼 레이어를 이름에 넣으면 화면을 다른 레이어로 옮길 때 태그가 바뀌고, 그 태그를 이름 문자열로 저장한 에셋(레지스트리, 탭바)이 조용히 빈 태그가 된다.

## 3. 플러그인과 모듈 경계

```
Sonheim/Plugins/GameUIFramework/
  GameUIFramework.uplugin
  Source/GameUIFramework/          (Runtime 모듈 하나)
    GameUIFramework.Build.cs        Core, CoreUObject, Engine, UMG, Slate, SlateCore, GameplayTags, DeveloperSettings
    Public/
      GameUIManagerSubsystem.h      ULocalPlayerSubsystem. 스택, 레이어 배치, 입력 상태, 뒤로 가기, 비동기 열기, 유지
      GameUIRootWidget.h            위젯 트리의 레이어를 태그로 등록 + 로딩 표시 BindWidget
      GameUILayer.h                 UGameUILayer(그냥 붙임), UGameUIStackLayer(스택, 맨 위만 보임, 입력에 낌)
      GameUIScreenWidget.h          화면 기반 + 클래스 기본값 (레이어, 입력, 유지, 뒤로)
      GameUIPopupWidget.h           팝업 기반 (결과 콜백)
      GameUIToastWidget.h           토스트 위젯 기반
      GameUIToastSubsystem.h        슬롯별 큐 (TakeTurns / ReplaceShowing / Stack), 채널 회수
      GameUIScreenRegistry.h        UDataAsset: 태그 → TSoftClassPtr<UGameUIScreenWidget>, 미리 로드 표시
      GameUISettings.h              UDeveloperSettings: 루트 클래스, 레지스트리, 토스트 슬롯
```

플러그인은 Sonheim 모듈을 모른다. 게임 쪽은 아래 이음매로만 끼어든다.

| 이음매 | 플러그인이 내놓는 것 | Sonheim이 하는 일 |
|---|---|---|
| 입력 상태 | `FOnGameUIInputStateChanged OnInputStateChanged(bool bBlockGameplay)` | `UInputHintSubsystem`이 받아 `IMC_Menu`를 켜고 끔 (지금 `SetMenuInput` 직접 호출을 대체) |
| 맨 위 화면 변경 | `FOnGameUITopScreenChanged OnTopScreenChanged(FGameplayTag)` | 메뉴 탭바가 받아, Menu 레이어의 맨 위 화면이 탭이면 보이고 불을 켬 (위에 팝업이 떠도 그대로) |
| 뒤로 가기 입력 | `bool HandleBack()` | `IA_UI_Close`(Esc, 게임패드 B)에서 호출 |
| 화면 내용 | 기반 클래스의 가상 함수 | 인벤토리·도감·지도·퀘스트·상자·제작대·팝업 위젯이 상속 |
| 토스트 내용 | `FInstancedStruct` 페이로드 + `UGameUIToastWidget` | `FNoticeData`를 싣고, `UNoticeWidget`이 상속해 풀어 그림 |
| 미리 로드할 에셋 | `UGameUIScreenWidget::GatherPreloadAssets` | 도감이 종의 `DisplayClass` 경로를 돌려줌 |

게임에 남는 것: 각 화면 위젯, View 구조체(`MonsterDexView`, `QuestView`, 새로 `FWorldMapView`), 알림 내용(`FNoticeData`, `UNoticeStyle`), 입력 힌트, 메뉴 탭바, 성능 장면, `UI.Screen.*` 태그 정의, `WBP_UIRoot`(플러그인 루트 클래스를 상속한 게임 에셋).

## 4. 기반 클래스 계약

### UGameUIScreenWidget (Abstract, UUserWidget)

클래스 기본값 (WBP Class Defaults에서 편집):

```cpp
UPROPERTY(EditDefaultsOnly, Category="Screen") FGameplayTag Layer;  // meta=(Categories="UI.Layer"), 프로젝트 설정의 DefaultScreenLayer로 채움
UPROPERTY(EditDefaultsOnly, Category="Screen") EGameUIInputMode InputMode = EGameUIInputMode::GameAndUI; // GameOnly, GameAndUI, UIOnly
UPROPERTY(EditDefaultsOnly, Category="Screen") bool bShowMouse = true;
UPROPERTY(EditDefaultsOnly, Category="Screen") bool bBlockGameInput = true;
UPROPERTY(EditDefaultsOnly, Category="Screen") bool bKeepWhenClosed = false;
UPROPERTY(EditDefaultsOnly, Category="Screen") EGameUIBack Back = EGameUIBack::Close;      // Close, Ignore, Custom
```

매니저가 부르는 것:

| 함수 | 시점 | 규칙 |
|---|---|---|
| `NativeOnOpened()` / BP `OnOpened` | 레이어에 붙은 직후, 매 열기 | 열기마다 다시 할 일: 구독, View 다시 만들기, 렌더 타깃 잡기 |
| `NativeOnClosed()` / BP `OnClosed` | 레이어에서 떼기 직전, 매 닫기 | 열기에서 잡은 것을 모두 놓음: 구독 해제, 렌더 타깃·브러시·`DynamicEntryBox` 풀 비우기 |
| `NativeOnCovered()` / `NativeOnUncovered()` | 같은 스택 레이어에서 위에 화면이 올라옴 / 내려감 | 가려진 동안 멈출 일(틱, 3D 스테이지 캡처) |
| `NativeOnBack()` | `Back == Custom`일 때 뒤로 가기 | `true`를 돌려주면 처리됨, `false`면 아무 일도 없음 |
| `GatherPreloadAssets(TArray<FSoftObjectPath>&) const` | 클래스가 로드된 뒤 CDO에서, 첫 인스턴스를 만들기 전 | 열기 전에 함께 비동기 로드할 경로 |

- 한 번만 할 설정(버튼 바인딩)은 지금처럼 `NativeOnInitialized`.
- `NativeConstruct`/`NativeDestruct`는 유지 화면에서도 열고 닫을 때마다 돈다 (Slate를 다시 짓기 때문). 그래서 열림/닫힘 일은 `OnOpened`/`OnClosed`로 옮기고, `NativeConstruct`에는 남기지 않는다.
- 화면이 스스로 닫을 때는 `CloseSelf()` (지금 `UUIStackSubsystem::CloseWidget(this)` 대체).

### UGameUIPopupWidget (Abstract, UGameUIScreenWidget)

```cpp
DECLARE_DELEGATE_TwoParams(FOnGameUIPopupResult, UGameUIPopupWidget& /*Popup*/, bool /*bConfirmed*/);
FOnGameUIPopupResult OnResult;          // 여는 쪽이 설정, 한 번만 불림
void Finish(bool bConfirmed);           // OnResult 부르고 CloseSelf
```

- 기본값: 레이어는 설정의 `DefaultPopupLayer`(`GetDefaultLayer`), `Back = Custom`(뒤로 = `Finish(false)`). 입력 모드는 Screen 기본값(`GameAndUI`) 그대로: `UIOnly`면 Esc·게임패드 B가 Enhanced Input에 닿지 않는다.
- 다른 이유로 닫히면(맵 이동, 부모 화면 닫힘) `OnClosed`에서 결과를 안 받은 상태면 `Finish(false)`와 같은 취소를 보낸다. 결과 콜백은 정확히 한 번.
- 결과 값은 파생 클래스가 가진다. 예: `UConfirmWidget::GetCount()`. 콜백이 팝업 참조를 받으니 꺼내 쓴다.
- 팝업 뒤를 막는 반투명 배경은 각 팝업 WBP의 전체 화면 Border. 코드는 없다.

### HUD

HUD는 기반 클래스 없이 `UUserWidget` 그대로. 매니저의 `AddToLayer(HUD 레이어 태그, Widget)`로 HUD 레이어에 붙인다. 스택·입력 규칙 없음.

### UGameUIToastWidget (Abstract, UUserWidget)

```cpp
virtual void ShowToast(FName Channel, const FInstancedStruct& Payload);
virtual void Stop();
virtual void StopChannel(FName Channel) {}
FSimpleDelegate OnFinished;
```

지금 `UNoticeWidget`의 `ShowFrom`/`Stop`/`StopChannel`/`OnFinished`와 같은 모양이다. `UNoticeWidget`이 이것을 상속하고 `Payload.Get<FNoticeData>()`를 그린다.

## 5. 레지스트리와 설정

```cpp
USTRUCT() struct FGameUIScreenEntry
{
    UPROPERTY(EditAnywhere, meta=(Categories="UI.Screen")) FGameplayTag Tag;
    UPROPERTY(EditAnywhere) TSoftClassPtr<UGameUIScreenWidget> Class;
    UPROPERTY(EditAnywhere) bool bPreload = false;   // 로컬 플레이어가 생기면 비동기로 로드해 둠
};
UCLASS() class UGameUIScreenRegistry : public UDataAsset { TArray<FGameUIScreenEntry> Screens; };

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Game UI")) class UGameUISettings : public UDeveloperSettings
{
    UPROPERTY(Config, EditAnywhere) TSoftClassPtr<UGameUIRootWidget> RootClass;
    UPROPERTY(Config, EditAnywhere) TSoftObjectPtr<UGameUIScreenRegistry> Registry;
    UPROPERTY(Config, EditAnywhere) FGameplayTag DefaultScreenLayer;   // UI.Layer.Menu
    UPROPERTY(Config, EditAnywhere) FGameplayTag DefaultPopupLayer;    // UI.Layer.Popup
    UPROPERTY(Config, EditAnywhere) FGameplayTag ToastLayer;           // UI.Layer.Toast
    UPROPERTY(Config, EditAnywhere) TArray<FGameUIToastSlot> ToastSlots;   // Tag(UI.Toast.*), WidgetClass, Order, MaxWaiting. 배열 순서 = 겹침 순서
};
```

- 프로젝트 설정 > Game > Game UI에서 편집, `DefaultGame.ini`의 `[/Script/GameUIFramework.GameUISettings]`에 저장.
- `[/Script/Sonheim.UIStackSubsystem]`의 `Screens`, `TabBarClass`, `TabBarZOrder`와 `[/Script/Sonheim.NoticeSubsystem]`의 슬롯 설정은 없어진다.
- 레지스트리 에셋: `/Game/UI/DA_UIScreens`.
- 태그: 코드가 쓰는 태그라 `Sonheim/UI/System/UITags.cpp`에 네이티브 태그(`UE_DEFINE_GAMEPLAY_TAG_COMMENT`)로 정의한다. 에디터 태그 목록에도 그대로 나온다.

```
UI.Screen.Inventory     (지금 Menu: 인벤토리 + 플레이어 스탯을 한 WBP로 묶음)
UI.Screen.MonsterDex
UI.Screen.WorldMap
UI.Screen.QuestJournal
UI.Screen.QuestDialog
UI.Screen.Container
UI.Screen.Crafting
UI.Popup.ItemQuantity
UI.Layer.HUD / UI.Layer.Menu / UI.Layer.Popup / UI.Layer.Toast
UI.Toast.Feed / UI.Toast.Title / UI.Toast.Headline
```

- `UI.Dungeon.HUD`/`UI.Dungeon.Result`는 던전 레지스트리 그대로 둔다 (5단계에서 HUD 레이어로만 옮김).
- 메뉴 탭 순서는 `WBP_MenuTabBar`의 `Tabs`에 남고, `FMenuTab::Screen`이 `FName`에서 `FGameplayTag`로 바뀐다. `TopTab`/`StepTab`은 매니저에서 빠져 탭바(게임 쪽)로 간다.

## 6. 여는 흐름 (비동기)

```cpp
// 매니저 API
void Open(FGameplayTag Screen, TFunction<void(UGameUIScreenWidget&)> OnReady = {});   // 이미 열려 있으면 맨 위로 + OnReady
template <class T> void Open(FGameplayTag Screen, TFunction<void(T&)> OnReady);
void Close(FGameplayTag Screen);   // 로딩 중이면 요청 취소
void Toggle(FGameplayTag Screen);
bool CloseTop();
bool HandleBack();
bool IsOpen(FGameplayTag) const;   bool IsPending(FGameplayTag) const;
UGameUIScreenWidget* Find(FGameplayTag) const;
bool IsGameplayBlocked() const;    // 로딩 중인 화면도 차단으로 침 (열기를 누른 순간부터 이동이 멈춤)
void AddToLayer(FGameplayTag Layer, UUserWidget*);
```

```
Open(Tag, OnReady)
 ├─ 열려 있음 ─────────────► 맨 위로, ApplyInputState, OnReady(위젯)  [같은 프레임]
 ├─ 같은 Tag 로딩 중 ───────► OnReady만 대기 목록에 더함
 ├─ 유지 목록에 있음 ───────► 레이어에 다시 붙임 → OnOpened → OnReady  [같은 프레임]
 └─ 새로 만듦
     ├─ 클래스 + 미리 로드 에셋이 이미 메모리에 있음 ─► 바로 생성  [같은 프레임]
     └─ 아님 ─► Pending에 기록, LoadingIndicator 보임, IsGameplayBlocked = true
                 RequestAsyncLoad(클래스)
                  → CDO.GatherPreloadAssets → RequestAsyncLoad(에셋들)
                  → (그 사이 Close(Tag)나 맵 이동이면 버림)
                  → CreateWidget → 레이어에 붙임 → OnOpened → 대기 중 OnReady 모두
                  → LoadingIndicator 숨김, ApplyInputState
```

- 이미 로드된 클래스는 같은 프레임에 열려서, 열자마자 위젯을 만지는 기존 흐름(퀘스트 대화 `SetTarget`, 상자 `OpenContainer`)도 깨지지 않는다. 그래도 호출 쪽은 모두 `OnReady`로 옮긴다.
- 미리 로드: `bPreload` 화면은 매니저 `Initialize` 뒤 첫 컨트롤러가 붙을 때 비동기 로드하고 핸들을 쥐고 있는다. 도감은 `bPreload = true` + `GatherPreloadAssets`로 종 클래스 전부를 함께 로드해, 첫 열기의 동기 로드 약 120 ms가 사라진다. `MonsterDexWidget.cpp`의 `LoadSynchronous` 두 곳은 `Get()`이 되고, 혹시 비어 있으면 그때만 동기 로드한다.
- 실패(경로 틀림): 에러 로그, Pending 지움, 입력 상태 되돌림.

## 7. 데이터와 뷰 분리

규칙: 위젯은 게임 상태를 직접 읽지 않는다. `XxxView::Build(데이터 소스)`가 View 구조체를 만들고 위젯은 그 구조체만 그린다. View 구조체와 빌더는 게임 모듈(`Sonheim/UI/<화면>/<화면>View.h`).

| 화면 | 지금 | 할 일 |
|---|---|---|
| 도감 | `MonsterDexView` 있음 | 그대로 |
| 퀘스트 | `QuestView` 있음 | 그대로 |
| 지도 | 위젯이 컴포넌트와 액터를 직접 읽음 | `FWorldMapView`(찾은 곳, 탐험한 칸, 퀘스트 위치, 이동 지점)를 `SonheimMap::BuildView`가 만듦 (2단계) |
| 인벤토리 | 컴포넌트 이벤트가 넘기는 `FInventoryItem` 배열을 그림. 슬롯 차이 갱신도 이 배열 기준 | 그대로. 입력이 이미 데이터 배열이라 따로 View를 두지 않음 |
| 수량 팝업 | `ItemID`로 위젯이 아이템 데이터 조회 | 여는 쪽이 `FItemQuantityView`를 넘김 (3단계) |
| 알림 | `FNoticeData`가 이미 View 구조체 | 그대로 |
| HUD | 상태 위젯이 컴포넌트 이벤트에 직접 묶임 | 5단계에서 정리, 범위는 그때 측정 보고 판단 |

## 8. 마이그레이션 단계

각 단계 끝에 빌드, 게임 실행(메뉴·탭 전환·상자·제작대·퀘스트 대화·팝업·알림 손으로 확인), 커밋. 1·2·3단계 뒤 성능 장면 `OpenClose`, `InvSwap`을 feat-ui-perf 기준과 비교.

### 1단계: 플러그인 + 루트 레이아웃 + 기반 클래스 + 레지스트리

- 새로: `Plugins/GameUIFramework` 전부, `UITags.h/.cpp`, `WBP_UIRoot`, `WBP_InventoryScreen`, `DA_UIScreens`.
- `Sonheim.uproject`에 플러그인 추가, `Sonheim.Build.cs`에 `GameUIFramework` 의존.
- `UUIStackSubsystem` 삭제. 호출 쪽을 매니저로 바꿈:
  `SonheimPlayerController.cpp`, `InteractionComponent.cpp`, `QuestGiver.cpp`, `FastTravelPoint.cpp`, `PerfScenarioSubsystem.cpp`, `MonsterDexWidget.cpp`, `WorldMapWidget.cpp`, `QuestWidget.cpp`, `ContainerInteractionWidget.cpp`, `CraftingWidget.cpp`, `InputHintSubsystem`(델리게이트 구독), `MenuTabWidget`(태그 + `TopTab`/`StepTab`).
- 화면 위젯 부모만 `UGameUIScreenWidget`으로 바꾸고(헤더 한 줄), 지금 ini 규칙(유지: 인벤토리·도감·지도)은 각 C++ 생성자에서 기본값으로 넣는다. WBP에서 덮어쓸 수 있다. 위젯 안 로직은 2단계에서 옮긴다.
- 메뉴가 두 위젯(인벤토리 + 플레이어 스탯)이던 것을 `WBP_InventoryScreen`(`UInventoryScreenWidget`) 한 장으로 묶는다. 매니저는 화면당 위젯 하나만 다룬다. `WBP_UIRoot`, `WBP_InventoryScreen`, `DA_UIScreens`는 에디터에서 만든다.
- `PlayerController`의 `InventoryWidgetClass`, `PlayerStatWidgetClass`, `ContainerInteractionWidgetClass`, `CraftingWidgetClass`가 레지스트리로 감.
- `DefaultGame.ini`에서 `[/Script/Sonheim.UIStackSubsystem]` 삭제, `GameUISettings` 추가.

### 2단계: 탭 화면 (인벤토리, 도감, 지도, 퀘스트) + 상자·제작대

- 대상: `UInventoryScreenWidget`, `UMonsterDexWidget`, `UWorldMapWidget`, `UQuestJournalWidget`, `UQuestDialogWidget`(`UQuestScreenWidget` 공통 부모), `UContainerInteractionWidget`, `UCraftingWidget`.
- 각 위젯의 `NativeConstruct`/`NativeDestruct`에 있는 열림/닫힘 일을 `OnOpened`/`OnClosed`로 옮김. `OpenMenu`의 `AddUniqueDynamic` 묶기는 인벤토리 화면 `OnOpened`/`OnClosed`로.
- 도감 `GatherPreloadAssets` + `bPreload`.
- `FWorldMapView`.

### 3단계: 팝업

- `UConfirmWidget`의 부모를 `UGameUIPopupWidget`으로 (클래스 이름은 WBP가 가리키고 있어 그대로), `UI.Popup.ItemQuantity`. `InventoryWidget::HandleTrashDrop`이 `FItemQuantityView`를 만들어 매니저로 열고 `OnResult`로 받음. 팝업이 아이템 데이터를 직접 읽지 않음.

### 4단계: 알림을 Toast 레이어로

- 큐 로직(`PresentNext`, `Finished`, 순서 세 가지, `MaxWaiting`, 채널 회수)을 `UGameUIToastSubsystem`으로 옮김.
- `UNoticeWidget` 부모를 `UGameUIToastWidget`으로. `Style`은 ini 대신 각 슬롯 WBP의 클래스 기본값.
- `UNoticeSubsystem`은 `Push(ENoticeSlot, Channel, FNoticeData)` → 토스트 서브시스템 호출로 바꾸는 얇은 앞면만 남김 (호출하는 곳이 많아 시그니처 유지). `ENoticeSlot` → `UI.Toast.*` 매핑.
- 손대는 곳: `NoticeSubsystem.h/.cpp`, `NoticeWidget.h/.cpp`, `DefaultGame.ini`.

### 5단계: HUD 정리

- `StatusWidget`, `QuestTracker`, `Minimap`, 던전 HUD/결과(`DungeonUIRouterSubsystem`의 1800/1900)를 `AddToLayer(HUD)`로.
- 던전의 `HiddenDuringRun` 숨김은 그대로 동작하는지 확인.
- 손대는 곳: `SonheimPlayerController.cpp`, `DungeonUIRouterSubsystem.cpp`.

### 범위 밖

로비 UI(`LobbyUI`, `FindRoomUI`, `MakeRoomUI`)는 다른 맵이고 스택을 안 써서 손대지 않는다. 월드 공간 위젯(데미지 숫자, 몬스터 상태, 감지 표시)도 그대로.

## 9. 성능 확인

- 측정 도구: `P4_UIDex_Worktree/Sonheim/Scripts/Perf`의 `measure_ui.py`, `insights.py` (이 브랜치에 커밋하지 않음).
- 기준: feat-ui-perf `14d9ac4c`의 `OpenClose`, `InvSwap`.
- 예상 차이: 화면이 뷰포트 직속이 아니라 Overlay 자식이 되면서 붙이고 뗄 때 상위 Overlay가 레이아웃을 다시 함. 유지 화면은 지금처럼 떼고 다시 붙임. 측정에서 나빠지면 그 화면만 Collapsed로 숨기는 방식을 검토 (결정 5).
