# 20. UI Performance Profiling & Optimization

인벤토리·도감·지도처럼 자주 사용하는 UI에서 **평상시 프레임 시간보다 화면을 열거나 아이템을 교환하는 순간의 지연**에 주목했습니다. 동일한 플레이 상황을 반복할 수 있는 측정 장면을 만들고, Unreal Insights와 CSV 이벤트 기록으로 비용이 발생하는 경로를 확인한 뒤 UI 갱신과 화면 수명 관리를 수정했습니다.

주요 결과는 아이템 교환 프레임 **32.6 → 16.5ms**, 인벤토리 열기 **57.5 → 21.7ms**입니다. 모두 1920×1080 환경에서 기록한 *조작 이벤트가 발생한 프레임의 중앙값*이며, 전체 게임의 평균 FPS 개선을 뜻하지는 않습니다.

이 문서는 기존 [18. UMG v2 Migration](18_UMG_v2_Migration.md)에서 만든 UIStack·인벤토리·도감 화면을 대상으로 **측정 → 원인 분석 → 구조 수정 → 재측정**한 내용을 다룹니다.

## 개선 전후

| 조작 | 개선 전 Frame (Game Thread) | 개선 후 Frame (Game Thread) |
|---|---:|---:|
| 아이템 교환 | 32.6ms (19.4ms) | **16.5ms (2.9ms)** |
| 인벤토리 열기 | 57.5ms (44.6ms) | **21.7ms (8.1ms)** |
| 도감 열기 | 25.8ms (12.4ms) | **22.1ms (8.5ms)** |
| 월드맵 열기 | 21.0ms (7.4ms) | **19.3ms (5.6ms)** |
| 보관함 열기 | 30.4ms (16.9ms) | **22.6ms (9.2ms)** |

*PR #6의 이벤트별 측정 중앙값입니다. Game Thread는 괄호 안에 구분했습니다. 실행 환경 및 장면의 영향을 받는 수치이므로 다른 프로젝트의 고정 성능값으로 일반화하지 않습니다.*

조작에 따라 병목이 달랐습니다. 아이템 교환은 전체 슬롯의 반복 갱신과 툴팁 처리, 화면 열기는 위젯 생성·슬롯 구성 비용이 중요한 개선 대상이었습니다. 도감은 화면 열기 비용과 별도로 **3D 프리뷰의 매 프레임 SceneCapture 렌더링 비용**도 조사했습니다.

## 실제 조작 프레임을 측정하기

평균 프레임 시간만 보면 1회성으로 발생하는 화면 열기·드래그·교환의 지연을 놓치기 쉽습니다. 그래서 `UPerfScenarioSubsystem : UGameInstanceSubsystem`에 개발 빌드용 시나리오를 구현했습니다.

- 플레이어 위치와 시선을 일정하게 맞춰 배경 렌더링 차이를 줄입니다.
- `-PerfScenes`, `-PerfWarmup`, `-PerfSeconds`, `-PerfRun`으로 장면·워밍업·수집 시간을 지정합니다.
- `HUD`, `Inventory`, `Dex`, `Map`, `OpenClose`, `InvSwap`, `DragSwap`, `Chest` 등 실제 UI 작업에 해당하는 장면을 반복합니다.
- CSV 이벤트(`PerfUI/Open Menu`, `PerfUI/Swap` 등)와 CPU Trace Scope로 **이벤트 발생 시점과 해당 프레임**을 찾아 비교합니다.
- CSV 캡처가 끝나면 비동기 파일 쓰기 완료를 확인하고 다음 장면으로 넘어갑니다.

~~~text
-PerfScenes=OpenClose,InvSwap,DragSwap,Chest
-PerfWarmup=3 -PerfSeconds=10 -PerfLabel=after -PerfRun=1
~~~

`OpenClose`는 인벤토리·도감·지도를 차례로 여닫고, `InvSwap`은 인벤토리 컴포넌트에 교환 요청을 전달합니다. `DragSwap`은 `FSlateApplication`에 Press → Move → Drop 입력을 전달해 **실제 UMG 드래그 경로**를 통과합니다. 두 교환 장면은 측정하는 코드 경로가 서로 다르므로 구분했습니다. 드래그 장면에서는 반복 측정상 큰 프레임 지연이 관찰되지 않았습니다.

측정 시나리오의 목적은 모든 플레이 상황을 완벽하게 재현하는 것이 아니라 **동일 조작을 반복해 개선 전후를 비교하고, Insights에서 원인을 추적할 수 있게 하는 것**입니다.

[측정 장면 구현 — PerfScenarioSubsystem.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-perf/Sonheim/Source/Sonheim/Utilities/PerfScenarioSubsystem.cpp)

## 인벤토리: 데이터 갱신과 슬롯 렌더링 분리

### 원인 — 하나의 아이템만 바뀌어도 슬롯 전체 갱신

기존 `UInventoryWidget::UpdateInventoryFromData()`는 인벤토리 변경 Delegate를 받으면 **모든 슬롯을 먼저 비우고**, 각 슬롯에 아이템 정보를 다시 설정했습니다. 교환으로 실제 데이터가 바뀐 슬롯이 두 개여도 같은 비용이 발생했습니다.

슬롯의 `SetItemData()`에서는 아이콘·무게·수량을 다시 지정하고, 툴팁 위젯이 없다면 생성·초기화했습니다. 그 결과 필요하지 않은 위젯 속성 변경과 Slate Invalidation이 발생할 수 있었습니다.

### 수정 — 슬롯은 마지막 표시 상태를 보유

전체 데이터 배열을 전달하는 인터페이스는 유지하되, 실제 화면 변경 여부를 **각 슬롯 위젯이 판단**하도록 바꿨습니다.

~~~cpp
// SlotWidget.cpp — 실제 변경 코드 발췌
void USlotWidget::SetItemData(const FItemData* ItemData, int32 NewQuantity)
{
    if (ItemData == ShownItem && NewQuantity == Quantity) return;
    ClearSlot();
    // 아이콘·수량·무게·등급 표현 갱신
    ShownItem = ItemData;
    Quantity = NewQuantity;
    if (IsHovered()) RefreshToolTip();
}
~~~

`UInventoryWidget::UpdateInventoryFromData()`는 모든 슬롯을 순회하지만, `USlotWidget`의 **아이템 데이터 포인터와 수량**이 같으면 Brush·텍스트·Visibility를 다시 건드리지 않습니다. 빈 칸 역시 이미 빈 상태라면 `ClearSlot()`을 생략합니다.

이 방식은 데이터의 변경 인덱스를 따로 받는 델타 이벤트가 아닙니다. **데이터 순회는 유지하고, 실제 UMG 속성 갱신 비용을 줄이는 방식**입니다. 현재 아이템 정의 데이터의 포인터가 안정적으로 유지되는 구조를 전제로 합니다.

### 툴팁은 필요할 때 생성

변경 전에는 슬롯 데이터를 설정하는 과정에서 툴팁을 준비했습니다. 수정 후에는 `NativeOnMouseEnter()`에서 `RefreshToolTip()`을 호출해, 플레이어가 실제로 아이템에 마우스를 올렸을 때 생성·초기화합니다. 호버 중 아이템이 바뀌는 경우도 `SetItemData()`에서 갱신합니다.

아이템 수가 많은 화면을 여는 순간에 **사용하지 않을 툴팁을 미리 만드는 비용**을 피하는 선택입니다.

[InventoryWidget.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-perf/Sonheim/Source/Sonheim/UI/Widget/Player/Inventory/InventoryWidget.cpp) · [SlotWidget.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-perf/Sonheim/Source/Sonheim/UI/Widget/Player/Inventory/SlotWidget.cpp)

## UIStack: 닫힌 화면의 UObject 재사용

### 기존 비용과 선택한 방식

UIStack은 화면을 닫을 때 `RemoveFromParent()`로 Viewport에서 제거했습니다. 이후 메뉴를 다시 열면 `CreateWidget`과 슬롯·탭 구성 비용이 반복됐습니다.

이번 변경에서는 `FUIScreenRule::bKeepWhenClosed`를 추가해 **자주 다시 여는 화면에만 재사용 정책**을 적용했습니다. 현재 설정 대상은 메뉴, 보관함, 도감, 월드맵입니다.

~~~mermaid
flowchart LR
    A["Open<br/>CreateWidget 또는 Kept 조회"] --> B["Viewport 표시<br/>Stack에 등록"]
    B --> C["Close<br/>RemoveFromParent"]
    C --> D{"bKeepWhenClosed"}
    D -->|true| E["Kept에 UObject 유지"]
    D -->|false| F["참조 해제 가능"]
    E -->|재오픈| G["OwningPlayer · Class 검사"]
    G -->|일치| B
    G -->|불일치| A
~~~

화면을 **Collapsed로 계속 Viewport에 남기는 방식은 아닙니다.** 닫을 때 Viewport에서 떼어 `NativeDestruct()`가 실행되도록 하고, UIStack의 `Kept` 배열에서 UObject만 유지합니다. 다시 열 때 OwningPlayer와 Widget Class를 검사해 재사용 가능하면 `AddToViewport()`로 복귀시킵니다. 이때 `NativeConstruct()`는 다시 실행되므로 바인딩과 화면 재진입 상태를 고려해야 합니다.

### 한 번 만드는 상태와 열 때마다 갱신할 상태 구분

- **재사용하는 것:** 인벤토리 슬롯 그리드, 탭 엔트리 풀, 월드맵의 지역 라벨과 탐험 안개 객체.
- **재오픈 시 갱신하는 것:** 현재 플레이어 데이터, 지도 중심·표시 상태, 보관함 내용 및 Delegate 연결.
- **닫을 때 해제하는 것:** 도감 3D 프리뷰 Stage와 오래된 Render Target을 가리키는 Brush 참조.

예를 들어 월드맵은 최초 `NativeConstruct()`에서만 지역·라벨을 만들고, 다시 열 때에는 현재 플레이어의 지도·퀘스트 상태를 연결하고 `Redraw()`합니다. 탭 바도 화면이 닫혔다고 UObject를 지우지 않고, 재오픈 시 다시 표시합니다.

재사용은 **생성 비용 대신 메모리를 점유**합니다. PR #6의 측정에서는 탭 화면의 위젯 객체 메모리가 약 **1.8MB**, 보관함 화면은 약 **0.94MB** 증가했습니다. 도감 프리뷰를 닫은 상태에서 추가로 유지되는 GPU Render Target은 측정되지 않았습니다.

[UIStackSubsystem.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-perf/Sonheim/Source/Sonheim/UI/System/UIStackSubsystem.cpp) · [DefaultGame.ini](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-perf/Sonheim/Config/DefaultGame.ini) · [WorldMapWidget.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-perf/Sonheim/Source/Sonheim/UI/Map/WorldMapWidget.cpp)

## 보관함: 재사용 화면의 Delegate와 표시 데이터 관리

보관함은 UIStack에서 화면을 재사용하도록 변경하면서, **언제 데이터를 구독하고 해제하는지**를 함께 수정했습니다.

기존에는 `PlayerController`가 보관함 UI의 인벤토리 Delegate 바인딩과 해제를 직접 관리했습니다. 하지만 Tab 입력 등으로 UIStack이 먼저 화면을 닫는 경로가 있어, 종료 시점과 구독 해제 시점이 일치하지 않을 수 있었습니다.

수정 후에는 `UContainerInteractionWidget`이 `OpenContainer()`에서 플레이어 인벤토리 Delegate를 연결하고 `NativeDestruct()`에서 해제합니다. 화면을 다시 열 때 중복 등록하지 않도록 `AddUniqueDynamic`을 사용했습니다.

~~~text
보관함 열기
  → UIStack 화면 표시/재사용
  → ContainerInteractionWidget::OpenContainer
  → InventoryComponent Delegate 연결 + 현재 보관함 표시

보관함 닫기 (닫기 버튼 또는 Tab)
  → UIStack에서 화면 제거
  → NativeDestruct
  → 보관함 종료 처리 + InventoryComponent Delegate 해제
~~~

`UContainerWidget`은 슬롯 그리드 크기가 같으면 다시 만들지 않고, 새로운 `ContainerComponent`를 연결하면 **현재 보관함 내용을 즉시 갱신**합니다. 서로 다른 보관함을 연속해서 열어도 이전 보관함의 슬롯 표현이 남지 않게 하기 위한 처리입니다.

같은 취지로 인벤토리·플레이어 스탯 등 반복적으로 연결될 수 있는 이벤트도 `AddUniqueDynamic`을 사용합니다. 여기서 중요한 점은 단순히 중복 등록을 막는 것뿐 아니라, **화면 수명에 맞는 등록·해제 책임을 UI가 가지도록 옮긴 것**입니다.

[ContainerInteractionWidget.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-perf/Sonheim/Source/Sonheim/UI/Widget/GameObject/ContainerInteractionWidget.cpp) · [ContainerWidget.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-perf/Sonheim/Source/Sonheim/UI/Widget/GameObject/ContainerWidget.cpp)

## 도감: SceneCapture 비용과 화면 종료 리소스 분리

도감의 3D 프리뷰는 별도 `AMonsterDexStage`와 `SceneCaptureComponent2D`를 사용합니다. 프리뷰에는 몬스터 메시와 전용 조명을 표시하므로, 메인 월드의 모든 렌더링 효과가 필요하지 않습니다.

`AMonsterDexStage`의 `ShowFlagSettings`에서 기존 대기·안개·구름 등에 더해 Dynamic Shadows, Ambient Occlusion, Nanite Meshes, Lumen GI/Reflections 등 사용하지 않는 패스를 비활성화했습니다. PR의 재측정 결과는 **Frame -0.83ms / GPU -0.62ms**입니다. 이는 위 표의 *도감 열기 이벤트 프레임*과 다른, SceneCapture 관련 프레임 비용 비교입니다.

또한 도감 화면을 닫으면 Stage Actor는 파괴되지만, **재사용되는 위젯의 Image Brush가 이전 Render Target을 참조할 수 있습니다.** 이를 위해 `UMonsterDexWidget::NativeDestruct()`에서 프리뷰 Image와 현재 도감 목록 셀의 Brush 참조를 비우고, Stage 관련 참조도 초기화합니다. 다음 열기에는 새로운 Stage와 프리뷰가 연결됩니다.

### 제외한 최적화

`SceneCapture` 갱신을 30Hz로 제한하는 변경도 비교했으나, 6회씩 반복한 측정에서 Frame **-0.01ms**, GPU **+0.08ms**로 뚜렷한 개선이 확인되지 않아 적용하지 않았습니다. 유의미하지 않은 변경을 유지하기보다 실제 비용이 줄어든 Show Flag 설정을 반영했습니다.

[MonsterDexStage.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-perf/Sonheim/Source/Sonheim/UI/MonsterDex/MonsterDexStage.cpp) · [MonsterDexWidget.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-perf/Sonheim/Source/Sonheim/UI/MonsterDex/MonsterDexWidget.cpp)

## 함께 수정한 UI 이벤트 처리

Feed 알림에서 `RowBox->GetAllEntries()`를 순회하면서 항목을 제거하면 순회 중 원본 배열이 변경될 수 있었습니다. `StopChannel()`과 `NativeTick()`에서 **목록 복사본을 순회하며 제거**하도록 수정했습니다.

~~~cpp
// NoticeWidget.cpp — 실제 변경 코드 발췌
for (UUserWidget* Entry : TArray<UUserWidget*>(RowBox->GetAllEntries()))
{
    if (!Entry->IsVisible()) RowBox->RemoveEntry(Entry);
}
~~~

[NoticeWidget.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-perf/Sonheim/Source/Sonheim/UI/Notice/NoticeWidget.cpp)

## 구현에서 얻은 기준

이번 최적화는 렌더링 옵션만 조정한 작업이 아닙니다. **전체 상태 전달과 화면 속성 갱신을 분리**하고, **Widget UObject의 유지 여부와 실제 화면 표시 기간을 분리**했으며, 화면 재사용으로 생기는 Delegate·Render Target 수명도 함께 처리했습니다.

평균 FPS 외에 실제 조작 프레임을 비교했기 때문에 인벤토리 교환과 화면 재진입의 비용을 별도로 찾을 수 있었습니다. 또한 모든 실험을 채택하지 않고, 반복 측정에서 차이가 확인된 수정만 남겼습니다.

**관련 문서:** [07. Inventory & Crafting](07_Inventory_Crafting.md) · [11. UI Architecture & Client Presentation](11_Client_State_Presentation_Pipeline.md) · [16. MonsterDex System](16_MonsterDex_System.md) · [18. UMG v2 Migration](18_UMG_v2_Migration.md) · [19. World Map & Fast Travel](19_World_Map_System.md)

**개발 변경:** [PR #6 — UI 조작 순간의 끊김 측정·수정과 성능 장면 실행기](https://github.com/chungheonLee0325/Sonheim/pull/6)
