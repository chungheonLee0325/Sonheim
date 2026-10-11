# 16. MonsterDex System

몬스터 도감은 `DT_MonsterDex`에 등록된 데이터를 기준으로 목록을 구성합니다. 각 플레이어의 발견 여부, 처치·포획 횟수, 보상 수령 기록을 함께 조회해 **도감 목록·상세 정보·보상 진행 상태**를 표시합니다.

## 도감 화면

![도감 화면의 ①~④ 영역 표시](../Media/Wiki/16_MonsterDex_System/16_dex_screen_areas.svg)

**① 몬스터 목록** — `DT_MonsterDex`의 `Number` 순서로 표시합니다. 미발견 몬스터는 `???`와 실루엣으로 표시하고, 발견·포획·보상 수령 가능 여부를 반영합니다.

**② 몬스터 상세** — 선택한 몬스터의 이름, 설명, 속성과 회전 가능한 3D 모델을 표시합니다.

**③ 처치·포획 기록** — 해당 플레이어가 기록한 처치 횟수(`KillCount`), 포획 횟수(`CaptureCount`)를 표시합니다.

**④ 도감 보상** — 각 보상의 달성 조건, 현재 진행도, 아이템·경험치, 수령 상태를 표시합니다.

화면의 네 영역은 `MonsterDexView::Build`가 만든 `FMonsterDexBookView`를 사용합니다. 목록은 `UMonsterDexCellWidget`, 보상 목록은 `UMonsterDexTierWidget`, 선택된 몬스터의 상세 정보는 `UMonsterDexWidget::DrawPicked`에서 갱신합니다.

## 목차

- [도감 데이터와 플레이어 기록](#도감-데이터와-플레이어-기록)
- [화면에 표시할 데이터 구성](#화면에-표시할-데이터-구성)
- [처치·포획 기록과 동기화](#처치포획-기록과-동기화)
- [도감 보상](#도감-보상)
- [3D 모델과 초상 이미지](#3d-모델과-초상-이미지)
- [관련 코드](#관련-코드)

---

## 도감 데이터와 플레이어 기록

### DT_MonsterDex — 몬스터 도감 정의

`UMonsterDexSubsystem`은 `DT_MonsterDex`의 `FMonsterDexData`를 로드해 `Number` 순으로 정렬합니다. 몬스터 하나가 DataTable의 한 행에 대응하며 `AreaObjectID`로 게임 내 몬스터 데이터와 연결됩니다.

| 필드 | 역할 |
|---|---|
| `AreaObjectID` / `Number` | 몬스터 ID / 도감 표시 순서 |
| `DisplayName` / `Description` | 도감에 표시할 이름과 설명 |
| `RewardTiers` | 처치·포획 조건과 단계별 아이템·경험치 보상 |
| `DisplayClass` | 3D 프리뷰를 생성할 몬스터 Character Class |

실제 `DT_MonsterDex`에는 도로롱, 파이호, 새싹콩, 전기 새싹콩, 수호자 볼트곰이 등록되어 있습니다. 예를 들어 도로롱(`Lamball`, `AreaObjectID=103`, `Number=1`)의 첫 번째 보상은 **포획 1회, 경험치 50, 아이템 500번 3개**로 설정되어 있습니다.

~~~cpp
// MonsterDexTypes.h — UPROPERTY 등 선언 매크로 생략
struct FMonsterDexData : public FTableRowBase
{
    int32 AreaObjectID = 0;
    int32 Number = 0;
    FText DisplayName;
    FText Description;
    TArray<FMonsterDexRewardTier> RewardTiers;
    TSoftClassPtr<ACharacter> DisplayClass;
};

struct FMonsterDexRewardTier
{
    int32 RequiredKills = 0;
    int32 RequiredCaptures = 0;
    int32 Exp = 0;
    TArray<FItemReward> Items;
};
~~~

도감에 사용할 몬스터의 초상 아이콘과 속성 정보는 기존 `dt_AreaObject` 데이터를 활용합니다. `AreaObjectID`로 `FAreaObjectData::AreaObjectIcon`과 `DefenceElementalAttributes`를 조회하기 때문에 두 테이블에 같은 정보를 중복 입력하지 않습니다.

에셋: [DT_MonsterDex.uasset](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Content/_BluePrint/_DataTable/DT_MonsterDex.uasset)

### FMonsterDexEntry — 플레이어별 기록

`ASonheimPlayerState`의 `UMonsterDexComponent`는 플레이어가 발견한 몬스터의 기록을 보관합니다. 아직 처치하거나 포획한 적이 없는 몬스터는 `FMonsterDexEntry`가 생성되지 않습니다.

~~~cpp
// MonsterDexTypes.h — UPROPERTY 등 선언 매크로 생략
struct FMonsterDexEntry : public FFastArraySerializerItem
{
    int32 AreaObjectID = 0;
    int32 KillCount = 0;
    int32 CaptureCount = 0;
    int32 ClaimedTiers = 0;
};
~~~

- `KillCount` / `CaptureCount`: 해당 몬스터의 누적 처치·포획 횟수
- `ClaimedTiers`: 보상 단계별 수령 여부를 저장하는 비트 플래그
- `AreaObjectID`: `DT_MonsterDex`의 몬스터 정보와 플레이어 기록을 연결하는 키

플레이어 기록은 `FMonsterDexList : FFastArraySerializer`에 담아 관리합니다. 몬스터의 설명이나 보상 조건처럼 모든 플레이어에게 공통인 데이터는 여기에 복제하지 않습니다.

## 화면에 표시할 데이터 구성

`MonsterDexView::Build`는 도감 정의와 플레이어 기록, 기존 몬스터 데이터를 조회해 UI용 구조체를 만듭니다.

~~~mermaid
flowchart TB
    DT["DT_MonsterDex<br/>FMonsterDexData"]
    AREA["dt_AreaObject<br/>FAreaObjectData"]
    RECORD["UMonsterDexComponent<br/>FMonsterDexEntry"]
    BUILD["MonsterDexView::Build"]
    BOOK["FMonsterDexBookView<br/>FMonsterDexPageView / FMonsterDexTierView"]
    UI["UMonsterDexWidget<br/>목록 · 상세 · 보상"]
    DT --> BUILD
    AREA --> BUILD
    RECORD --> BUILD
    BUILD --> BOOK --> UI
~~~

`FMonsterDexBookView`에는 전체 페이지 목록(`Pages`), 발견한 몬스터 수(`Discovered`), 한 번 이상 포획한 몬스터 수(`Captured`)가 담깁니다.

`FMonsterDexPageView`는 몬스터별 번호·이름·설명·속성·초상·3D 표시 클래스와 누적 기록을 담고, 보상은 `FMonsterDexTierView` 배열로 구성합니다. UI 위젯이 DataTable과 서버 기록을 각자 조회하지 않도록 화면 표시 데이터 생성 단계를 나눈 것입니다.

### 발견 상태에 따른 화면 표시

도감 목록은 **등록된 모든 몬스터**를 보여주며, `FMonsterDexEntry`의 존재 여부로 발견 상태를 구분합니다.

~~~cpp
// MonsterDexView.cpp — Build 내부
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

| 플레이어 기록 | 목록·상세 화면 |
|---|---|
| `Entry` 없음 | 번호는 표시, 이름은 `???`, 초상·3D 모델은 검은 실루엣, 설명은 미발견 안내 문구 |
| `Entry` 있음 | 이름·설명·속성을 표시하고 몬스터 모델을 원래 색으로 출력 |
| `CaptureCount > 0` | 발견 상태에 더해 목록에 포획 아이콘 표시 |
| 받을 수 있는 보상 있음 | 목록에 보상 수령 가능 표시 |

미발견 몬스터도 목록에서 선택할 수 있고 보상 조건을 확인할 수 있습니다. **도감 등록은 첫 처치 또는 포획 시 이루어지며**, 근처에 접근하는 것만으로는 기록되지 않습니다.

발견 수는 `Entry`가 있는 몬스터의 개수, 포획 수는 `CaptureCount > 0`인 몬스터의 개수입니다. 포획 수는 총 포획 횟수의 합이 아닙니다.

## 처치·포획 기록과 동기화

전투·포획 결과는 서버의 `ASonheimPlayerState::ReportDeed`를 통해 `OnDeed` 델리게이트로 전달됩니다. `UMonsterDexComponent`는 `BeginPlay`에서 이 델리게이트를 구독해 도감 기록을 갱신합니다.

~~~cpp
// MonsterDexComponent.cpp
if (PS && PS->HasAuthority())
    PS->OnDeed.AddUObject(this, &UMonsterDexComponent::HandleDeed);

void UMonsterDexComponent::HandleDeed(
    EPlayerDeed Deed, int32 Id, int32, FName)
{
    if (Deed == EPlayerDeed::Kill || Deed == EPlayerDeed::Capture)
        Record(Id, Deed == EPlayerDeed::Capture);
}
~~~

`Record`는 `UMonsterDexSubsystem::FindPage`로 도감에 등록된 몬스터인지 확인합니다. 처음 기록된 몬스터라면 `FMonsterDexEntry`를 생성하고, 처치 또는 포획 횟수를 1 증가시킵니다. 이벤트의 `Count` 값은 현재 도감 집계에서는 사용하지 않습니다.

기록 변경 뒤에는 다음 순서로 반영됩니다.

~~~mermaid
sequenceDiagram
    participant G as Combat / Capture
    participant P as PlayerState
    participant D as MonsterDexComponent (Server)
    participant C as MonsterDexComponent (Owner)
    participant U as MonsterDexWidget
    G->>P: ReportDeed(Kill / Capture)
    P->>D: OnDeed
    D->>D: Record + MarkItemDirty
    D-->>C: FastArray Delta (OwnerOnly)
    C->>U: OnDexChanged
    U->>U: Redraw → MonsterDexView::Build
~~~

`UMonsterDexComponent::Commit`은 `Dex.MarkItemDirty`로 변경 항목을 표시하고, `OnDexChanged`를 브로드캐스트하며, `UMonsterDexSubsystem::StorePlayer`를 통해 호스트 SaveGame 슬롯(`MonsterDex`)에 기록합니다.

클라이언트는 `FMonsterDexEntry::PostReplicatedAdd / PostReplicatedChange`에서 `OnDexChanged`를 전달받습니다. `UMonsterDexWidget`은 이 델리게이트를 구독해 `Redraw`를 호출합니다. **FastArray는 기록의 변경분만 복제하지만, 현재 `Redraw`는 전체 도감 페이지의 표시 데이터를 다시 생성합니다.**

## 도감 보상

각 몬스터의 `RewardTiers`에는 필요한 처치·포획 횟수와 지급할 아이템·경험치를 정의합니다. 보상 상태는 `FMonsterDexTierView`를 만들 때 현재 기록과 수령 여부를 비교해 결정합니다.

| 상태 | 판정 기준 | 화면 표시 |
|---|---|---|
| 진행 중 (`InProgress`) | 요구한 처치·포획 횟수를 채우지 못함 | 현재 수치/목표 수치, 진행바, 비활성 버튼 |
| 수령 가능 (`Ready`) | 조건 충족, 해당 보상 미수령 | 수령 버튼 활성화 |
| 수령 완료 (`Claimed`) | `ClaimedTiers`에서 해당 단계 비트가 설정됨 | 수령 완료 표시, 버튼 비활성화 |

~~~cpp
// MonsterDexView.cpp — TierView 내부
View.State = Entry && Entry->IsClaimed(Index)
    ? EMonsterDexTierState::Claimed
    : Entry && Entry->Meets(Tier)
        ? EMonsterDexTierState::Ready
        : EMonsterDexTierState::InProgress;
~~~

`FMonsterDexEntry::Meets`는 `RequiredKills`와 `RequiredCaptures`를 모두 비교합니다. 현재 DataTable의 각 보상 단계는 처치 또는 포획 중 한 가지 목표를 사용하며, 진행바도 해당 목표 수치로 표시합니다. 경험치·아이템 보상은 `SonheimReward::Describe`에서 화면용 정보로 변환하고, 아이템은 공용 `USlotWidget::AddReadOnly`로 표시합니다.

### 보상 수령

`UMonsterDexTierWidget`에서 버튼을 누르면 `UMonsterDexWidget::Claim`이 `ServerClaimReward(AreaObjectID, Tier)`를 호출합니다.

서버의 `CanClaimReward`는 몬스터 기록과 보상 단계 존재 여부, 목표 달성, 기존 수령 여부를 확인합니다. 조건을 통과하면 해당 비트를 설정한 뒤 보상을 지급합니다.

~~~cpp
// MonsterDexComponent.cpp — ServerClaimReward_Implementation 내부
if (!GetOwner()->HasAuthority() || !CanClaimReward(AreaObjectID, Tier))
    return;

Entry->ClaimedTiers |= 1 << Tier;
Commit(*Entry);
SonheimReward::Grant(*Cast<ASonheimPlayerState>(GetOwner()),
                     Reward.Items, Reward.Exp);
~~~

`ClaimedTiers`는 비트 플래그이므로 단계 인덱스는 32 미만으로 제한합니다. 수령 기록을 먼저 갱신하기 때문에 같은 단계에 대한 반복 요청은 `CanClaimReward`에서 거부됩니다.

## 3D 모델과 초상 이미지

선택한 몬스터의 3D 모델은 `AMonsterDexStage`에서 렌더링합니다. `FMonsterDexData::DisplayClass`의 Character 기본 Mesh·애니메이션·머티리얼 설정을 가져와 `USkeletalMeshComponent`에 적용하고, `SceneCaptureComponent2D`의 결과를 RenderTarget으로 UI에 표시합니다.

`UMonsterDexWidget`은 도감을 열 때 두 개의 Stage Actor를 생성합니다.

- **`Stage`**: 선택한 몬스터를 실시간 렌더링합니다. 드래그로 회전할 수 있으며 일정 시간 입력이 없으면 자동 회전합니다.
- **`SnapStage`**: 목록에 사용할 몬스터별 정지 초상 이미지를 캡처합니다. `UMonsterDexCellWidget`은 이를 목록 아이콘으로 사용합니다.

`AMonsterDexStage::Frame`은 Skeletal Mesh의 Imported Bounds를 기준으로 카메라 거리와 위치를 계산합니다. 두 Actor는 로컬에서만 생성되며 `bReplicates = false`입니다. 미발견 상태에서는 `UMonsterDexCellWidget::Show`와 `UMonsterDexWidget::DrawPicked`가 이미지 색상에 `SilhouetteColor`를 적용합니다.

## 관련 코드

- [MonsterDexTypes.h](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/MonsterDex/MonsterDexTypes.h) — DataTable 정의·플레이어 기록·FastArray
- [MonsterDexSubsystem.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/MonsterDex/MonsterDexSubsystem.cpp) — 도감 정의 조회·SaveGame
- [MonsterDexComponent.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/AreaObject/Player/Utility/MonsterDexComponent.cpp) — 처치·포획 기록·보상 지급·복제
- [MonsterDexView.h / cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/UI/MonsterDex/MonsterDexView.cpp) — 화면 표시 데이터 생성
- [MonsterDexWidget.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/UI/MonsterDex/MonsterDexWidget.cpp) — 목록·상세·보상 UI
- [MonsterDexStage.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/UI/MonsterDex/MonsterDexStage.cpp) — 3D 모델 프리뷰·목록 초상
- [[17. Quest System|17_Quest_System]] · [[18. UMG v2 Migration|18_UMG_v2_Migration]]
