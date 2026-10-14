# 16. MonsterDex System

몬스터를 처음 처치하거나 포획하면 도감에 등록하고, 누적 기록에 따라 보상을 수령할 수 있습니다. 도감 목록과 상세 화면에는 발견 상태, 몬스터 모델, 처치·포획 횟수와 단계별 보상을 표시합니다.

## 게임 화면

![몬스터 목록·상세 3D 프리뷰·처치/포획 기록·보상 영역](../Media/Wiki/16_MonsterDex_System/16_dex_screen_areas.svg)

도감에 정의된 종은 발견 전에도 목록에 나타나지만 이름은 `???`로, 모델은 실루엣으로 표시합니다. 첫 처치·포획 이후 원래 이름과 외형, 속성 정보가 열립니다.

[도감 등록 시연](https://github.com/user-attachments/assets/d1d96b5f-1076-4e29-872d-3399a540acac) · [보상 진행·수령 시연](https://github.com/user-attachments/assets/f498002f-d15e-45ab-ac9b-1380ac0ad82b)

## 도감 데이터와 화면 표시

도감은 **몬스터 정의, 플레이어 기록, UI 표시 데이터**를 구분해 처리합니다.

| 데이터 | 주요 내용 | 사용하는 곳 |
|---|---|---|
| `DT_MonsterDex` | `AreaObjectID`, 도감 번호·이름·설명, `RewardTiers`, `DisplayClass` | 종별 정보와 보상 조건 |
| `dt_AreaObject` | 몬스터 기본 아이콘·속성 | 목록 이미지 대체, 상세 속성 |
| `FMonsterDexEntry` | `AreaObjectID`, 처치·포획 횟수, `ClaimedTiers` | 플레이어별 진행 기록 |
| `FMonsterDexBookView` | 목록 `Pages`, 발견·포획한 종의 수 | 도감 전체 화면 |
| `FMonsterDexPageView / FMonsterDexTierView` | 발견 여부, 표시 이름·모델·진행도·보상 상태 | 목록 Cell·상세·보상 Row |

![DT_MonsterDex의 종별 정의와 보상 단계 설정](../Media/Wiki/16_MonsterDex_System/16_dt_monsterdex_editor.png)

`UMonsterDexSubsystem`은 `DT_MonsterDex`의 항목을 `Number` 순으로 정렬합니다. `MonsterDexView::Build()`는 각 항목의 `AreaObjectID`로 플레이어 기록과 `dt_AreaObject`를 조회해 화면에 사용할 페이지를 구성합니다. 위젯이 도감 정의와 진행 기록을 직접 비교하지 않도록 **발견 여부와 표시 규칙을 View 구성 단계에서 결정**합니다.

~~~cpp
// MonsterDexView.cpp — Build() 발췌
const FMonsterDexEntry* Entry = Dex.FindEntry(Page.AreaObjectID);
View.bDiscovered = Entry != nullptr;
View.bCaptured = Entry && Entry->CaptureCount > 0;
View.Kills = Entry ? Entry->KillCount : 0;
View.Captures = Entry ? Entry->CaptureCount : 0;

if (View.bDiscovered)
{
    View.Name = Page.DisplayName;
    View.Description = Page.Description;
    if (Species) View.Elements = Species->DefenceElementalAttributes.Array();
}
else
{
    View.Name = INVTEXT("???");
    View.Description = LOCTABLE(SONHEIM_ST_DEX, "Page.Unknown.Description");
}
~~~

발견한 종은 **`FMonsterDexEntry`의 존재 여부**로 판단합니다. 포획 표시는 `CaptureCount > 0`으로 따로 계산합니다. `Book.Discovered`와 `Book.Captured`는 각각 발견한 종의 수와 한 번 이상 포획한 종의 수이며, 처치·포획 횟수의 합이 아닙니다.

## 기록 변경을 도감 UI에 반영

전투·포획 시스템에서 `ASonheimPlayerState::ReportDeed`로 이벤트를 전달하면 `UMonsterDexComponent::Record`가 도감에 정의된 종의 처치 또는 포획 횟수를 갱신합니다. 처음 처리하는 종은 `FMonsterDexEntry`를 추가합니다.

~~~mermaid
flowchart LR
    A["전투 · 포획<br/>ReportDeed"] --> B["MonsterDexComponent<br/>Record · Commit"]
    B --> C["OnDexChanged"]
    C --> D["MonsterDexView::Build"]
    D --> E["도감 목록 · 상세 · 보상"]
    B --> F["저장 · FastArray"]
    F -->|"소유 클라이언트 변경 통지"| C
~~~

`Commit()`은 `MarkItemDirty`와 `OnDexChanged`를 실행하고 저장 데이터를 갱신합니다. 기록은 PlayerState에 보관하며 `OwnerOnly FastArray`로 소유 클라이언트에 복제합니다. 복제된 항목의 Add/Change 콜백도 `OnDexChanged`를 발생시킵니다.

**UI에서는 기록 변경 시 페이지 표시 데이터를 다시 계산**합니다. 반면 선택한 종은 기존 `Picked` ID가 여전히 목록에 있을 때 유지합니다.

~~~cpp
// MonsterDexWidget.cpp — Redraw() 발췌
Book = MonsterDexView::Build(*Component);
if (!Book.Pages.ContainsByPredicate(
        [this](const FMonsterDexPageView& P) { return P.AreaObjectID == Picked; }))
{
    Picked = Book.Pages.Num() ? Book.Pages[0].AreaObjectID : 0;
}
~~~

`Redraw()`는 목록 Cell을 다시 구성하고 상세 화면을 갱신합니다. 하지만 사용자가 **다른 종을 선택하는 동작**은 `Pick()`에서 선택 프레임과 상세 화면만 바꿉니다. 목록의 정지 이미지도 도감을 열 때 생성한 `CellPictures`를 계속 사용합니다. 이 때문에 종을 선택할 때마다 전체 목록을 다시 캡처하지 않습니다.

첫 등록 시에는 `Headline`, 새 보상 단계 달성 시에는 `Feed`로 알림을 보냅니다. UI 알림은 기존 `UNoticeSubsystem`의 슬롯에 표시됩니다.

![처음 등록한 몬스터의 Headline과 왼쪽 Feed 알림 슬롯](https://github.com/user-attachments/assets/5f2b00a6-ffcf-46b3-ac6f-627e0a03ea73)

## 3D 프리뷰와 목록 썸네일

도감 화면은 상세용 `Stage`와 목록 썸네일용 `SnapStage`, 두 `AMonsterDexStage`를 생성합니다. 둘 다 로컬 `SceneCaptureComponent2D`의 RenderTarget으로 모델을 UI에 표시하지만 갱신 방식은 다릅니다.

![실시간 3D 프리뷰와 목록 썸네일의 화면 위치](../Media/Wiki/16_MonsterDex_System/16_model_preview_annotated.svg)

| 상세 프리뷰 `Stage` | 목록 썸네일 `SnapStage` |
|---|---|
| 선택한 종의 모델을 실시간 렌더링 | 도감에 정의된 종을 순서대로 정지 캡처 |
| 드래그 회전·일정 시간 뒤 자동 회전 | `SnapSettleFrames`만큼 기다린 뒤 `CaptureScene()` |
| 640×640 RenderTarget(기본 설정) | 종별 256×256 RenderTarget |
| 종 선택 시 `Show(DisplayClass)` | 화면을 여는 동안 한 번 `Snap(Classes, 256)` |

`DisplayClass`의 Character 기본 객체(CDO)에서 Skeletal Mesh·애니메이션 클래스·상대 Transform·오버라이드 머티리얼을 읽어 캡처용 Mesh에 적용합니다. `ShowOnlyComponents`를 사용하므로 배경 월드의 Actor가 프리뷰에 들어오지 않습니다.

~~~cpp
// MonsterDexStage.cpp — Frame()의 카메라 거리 계산
const FBox Box = Asset->GetImportedBounds().GetBox()
    .TransformBy(Mesh->GetComponentTransform());
const FVector Extent = Box.GetExtent();
const float Across = FVector2D(Extent.X, Extent.Y).Size();
const float Half = FMath::Max(10.f, FMath::Max(Extent.Z, Across));
const float Distance = Half /
    FMath::Tan(FMath::DegreesToRadians(FieldOfView * 0.5f)) * Margin;
~~~

종별 모델의 크기가 달라도 프레임 안에 들어오도록 **메시 Imported Bounds와 FOV로 카메라 거리를 계산**합니다. 목록 캡처는 클래스별로 모델을 교체하고 몇 프레임 대기해 애니메이션 포즈를 반영한 뒤 찍습니다.

미발견 상태의 실루엣은 모델을 따로 준비하지 않고 `UMonsterDexCellWidget::Show` / `UMonsterDexWidget::DrawPicked`에서 RenderTarget을 표시하는 이미지의 색을 변경합니다. 도감을 닫으면 두 Stage Actor와 이미지 자료를 정리하고, `OnDexChanged` 구독을 해제합니다.

## 보상 진행도와 수령 처리

`DT_MonsterDex.RewardTiers`에 처치·포획 목표와 아이템·경험치 보상을 설정합니다. `MonsterDexView::TierView`는 기록의 수량과 수령 비트를 읽어 아래 세 상태를 구성합니다.

| 표시 상태 | 계산 기준 |
|---|---|
| `InProgress` | 단계별 필요 처치·포획 횟수 미달성 |
| `Ready` | 목표 달성, 미수령 |
| `Claimed` | `ClaimedTiers`의 해당 단계 비트 설정 |

`UMonsterDexTierWidget`은 `Current / Required` 수치와 ProgressBar를 표시하고, 보상 아이템에는 인벤토리에서도 사용하는 `USlotWidget::AddReadOnly`를 사용합니다. 클릭 시에는 `UMonsterDexWidget::Claim()`이 `ServerClaimReward(AreaObjectID, Tier)`를 요청합니다.

서버는 `CanClaimReward()`로 단계 범위·달성 조건·수령 여부를 다시 확인합니다. 조건을 통과하면 **`ClaimedTiers` 비트를 먼저 설정하고 기록한 뒤** `SonheimReward::Grant`로 지급합니다. 같은 단계로 다시 요청해도 이미 수령한 비트 때문에 지급하지 않습니다.

## 관련 코드

- [MonsterDexView.cpp](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/UI/MonsterDex/MonsterDexView.cpp) · [MonsterDexWidget.cpp](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/UI/MonsterDex/MonsterDexWidget.cpp) — View 데이터·목록/상세/보상 UI
- [MonsterDexStage.cpp](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/UI/MonsterDex/MonsterDexStage.cpp) — 3D 상세·목록 캡처
- [MonsterDexTypes.h](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/MonsterDex/MonsterDexTypes.h) · [MonsterDexSubsystem.cpp](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/MonsterDex/MonsterDexSubsystem.cpp) — 정의·저장
- [MonsterDexComponent.cpp](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/AreaObject/Player/Utility/MonsterDexComponent.cpp) — 기록·복제·보상 처리
- [17. Quest System](17_Quest_System.md) · [18. UMG v2 Migration](18_UMG_v2_Migration.md)
