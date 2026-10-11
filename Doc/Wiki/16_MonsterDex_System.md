# 16. MonsterDex System

Sonheim 도감은 **서버가 처치·포획 기록과 보상 수령 상태를 관리하고, 소유 클라이언트에 필요한 기록만 복제하는 수집형 진행 시스템**입니다. 전투·포획 코드에서 도감 UI를 직접 호출하지 않고, `PlayerState`의 공통 행동 이벤트로 연결했습니다.

## 요약

- `DT_MonsterDex`는 종별 설명·표시 순서·보상 단계·3D 표시 클래스를 정의합니다.
- `UMonsterDexComponent`가 종별 `KillCount`/`CaptureCount` 및 `ClaimedTiers`를 서버에서 변경합니다.
- `FMonsterDexList : FFastArraySerializer`를 `COND_OwnerOnly`로 복제하고 `OnDexChanged`로 UI에 알립니다.
- `UMonsterDexSubsystem`이 호스트의 `USaveGame` 슬롯에 플레이어별 기록을 저장합니다.
- `AMonsterDexStage`는 월드 몬스터와 분리된 로컬 3D 프리뷰를 `SceneCaptureComponent2D`로 렌더링합니다.

## Runtime Result

![도감 최종 UI](../Media/Wiki/16_UMG_Migration_with_Claude/16_dex_after.jpg)

*도감의 목록·종별 진행·보상 단계·3D 프리뷰. 시각적 재스타일링 과정은 [[18. UMG v2 Migration|18_UMG_v2_Migration]]에 분리했습니다.*

## 목차

- [요구사항과 상태 소유](#요구사항과-상태-소유)
- [기록과 보상 처리](#기록과-보상-처리)
- [복제와 저장](#복제와-저장)
- [3D 미리보기](#3d-미리보기)
- [설계 판단과 검증](#설계-판단과-검증)
- [관련 코드](#관련-코드)

---

## 요구사항과 상태 소유

월드에서 발생한 처치·포획은 **플레이어별 누적 기록**이고, 보상 수령 여부는 세션이 바뀌어도 유지해야 합니다. 따라서 UI Widget이 아니라 플레이어의 `ASonheimPlayerState`에 붙은 `UMonsterDexComponent`가 Runtime State를 소유합니다.

~~~mermaid
flowchart LR
  A["Monster Kill / Capture"] --> B["ASonheimPlayerState<br/>ReportDeed / OnDeed"]
  B --> C["UMonsterDexComponent<br/>Record"]
  T["DT_MonsterDex<br/>FMonsterDexData"] --> C
  C --> D["FMonsterDexList<br/>MarkItemDirty"]
  D --> E["Owner FastArray Delta"]
  E --> F["OnDexChanged<br/>UMG"]
  C --> S["MonsterDexSubsystem<br/>Host SaveGame"]
~~~

- **Definition:** `FMonsterDexData`는 `AreaObjectID`, 표시 번호/이름/설명, `RewardTiers`, `DisplayClass`를 갖습니다. 행 데이터는 `UMonsterDexSubsystem`에서 정렬·조회합니다.
- **Runtime:** `FMonsterDexEntry`는 `AreaObjectID`, `KillCount`, `CaptureCount`, `ClaimedTiers`만 저장합니다.
- **Producer:** `EPlayerDeed::Kill / Capture` 이벤트만 도감 집계로 들어갑니다. 도감에 정의되지 않은 종은 `FindPage` 검사에서 제외합니다.
- **Presentation:** UMG는 서버의 기록을 읽는 소비자이며, 첫 등록 및 보상 가능 알림은 기록 처리 측에서 Notice 데이터를 생성합니다.

## 기록과 보상 처리

### 종별 카운트 갱신

`UMonsterDexComponent::HandleDeed`가 Kill/Capture를 `Record(AreaObjectID, bCaptured)`에 전달합니다. `Record`는 `HasAuthority()`와 종별 페이지 존재를 확인한 뒤 해당 Entry를 만들거나 찾아 수치를 갱신합니다.

처음 기록한 종에서는 `Headline` 슬롯으로 도감 등록을 알리고, 보상 단계의 충족 여부가 **미충족 → 충족**으로 바뀌면 `Feed` 슬롯으로 수령 가능 상태를 알립니다. 조건 비교는 `FMonsterDexEntry::Meets`에서 `RequiredKills / RequiredCaptures`를 기준으로 합니다.

### 보상 수령은 별도 서버 요청

조건을 만족했다고 즉시 지급하는 구조가 아닙니다. 도감 화면에서 수령을 요청하면 `ServerClaimReward(AreaObjectID, Tier)`가 서버에서 다음을 확인합니다.

~~~cpp
if (!GetOwner()->HasAuthority() || !CanClaimReward(AreaObjectID, Tier))
    return;

Entry->ClaimedTiers |= 1 << Tier;
Commit(*Entry);
SonheimReward::Grant(*Cast<ASonheimPlayerState>(GetOwner()),
                     Reward.Items, Reward.Exp);
~~~

`CanClaimReward`는 종별 정의/진행 기록 존재, 단계 인덱스 범위, 아직 수령하지 않았는지, 요구 횟수를 채웠는지 검증합니다. `ClaimedTiers` 비트를 먼저 설정하므로 **반복 요청으로 같은 단계의 보상을 다시 지급하지 않는 순서**입니다. 지급과 영속 저장을 하나의 트랜잭션으로 묶는 구조까지 구현한 것은 아닙니다.

## 복제와 저장

`FMonsterDexEntry : FFastArraySerializerItem`과 `FMonsterDexList : FFastArraySerializer`를 사용합니다. Entry 수정 시 `Dex.MarkItemDirty`, 저장 데이터 전체를 불러온 시점에는 `MarkArrayDirty`를 호출합니다.

~~~cpp
DOREPLIFETIME_CONDITION(UMonsterDexComponent, Dex, COND_OwnerOnly);
~~~

소유 클라이언트에서는 `PostReplicatedAdd / PostReplicatedChange`가 `OnDexChanged`를 발생시킵니다. 화면은 해당 이벤트를 통해 다시 그리며 Tick 기반 전수 조회에 의존하지 않습니다.

`UMonsterDexSubsystem`이 `DT_MonsterDex`의 정적 정의를 읽고 호스트 측 SaveGame 슬롯 `MonsterDex`에 플레이어별 `FMonsterDexEntry`를 기록합니다. 서버의 `OnSetUniqueId` 경로에서 `LoadSaved`를 실행합니다. 저장 키는 호스트와 원격 플레이어를 구분하지만, 이는 **Listen Server 호스트 저장 방식**이지 중앙 계정 DB가 아닙니다.

현재 저장은 `Commit` 시 `SaveGameToSlot`을 수행합니다. 기록 빈도가 많아지면 호출 비용을 줄이기 위한 배치 저장을 검토할 수 있습니다.

## 3D 미리보기

상단 [Runtime Result](#runtime-result)의 도감 화면 오른쪽 영역에 표시되는 프리뷰는 아래 전용 Stage Actor에서 생성합니다.

`AMonsterDexStage`는 월드 내 몬스터 Actor를 복제·이동하는 대신 **로컬 비복제 Actor**에 표시용 `USkeletalMeshComponent`를 보관합니다.

1. `FMonsterDexData::DisplayClass`의 Character Class Default에서 Skeletal Mesh, 상대 Transform, Anim Class, Override Material을 읽습니다.
2. `Show`에서 RenderTarget을 생성하고 `SceneCaptureComponent2D`가 해당 Mesh만 렌더링하도록 설정합니다.
3. Mesh Imported Bounds로 중심과 화면에 맞는 카메라 거리를 계산해 종별 크기 차이를 흡수합니다.
4. 드래그 회전 뒤 일정 시간이 지나면 자동 회전하고, `Snap` 경로는 별도 RenderTarget에 정지 초상을 캡처할 수 있습니다.

게임플레이 몬스터의 체력·AI·충돌을 건드리지 않으며 도감의 표현 수명은 게임 진행과 분리됩니다. 매 프레임 SceneCapture가 켜져 있는 경로와 프리뷰 Actor 수명은 UI 사용량에 따라 비용을 검토할 지점입니다.

## 설계 판단과 검증

| 선택 | 이유와 경계 |
|---|---|
| PlayerState 컴포넌트 | Pawn 교체와 별개인 플레이어 진행 상태 보관 |
| 공통 PlayerDeed | Kill/Capture 생산자가 도감 구현·UI에 직접 의존하지 않게 분리 |
| OwnerOnly FastArray | 개인 도감 목록의 변경분을 해당 플레이어에게 동기화 |
| Claim 비트셋 | 단계별 중복 수령을 작은 상태로 기록(현 구현은 32개 미만 Tier 인덱스 검사) |
| 별도 SceneCapture Stage | 월드 몬스터와 UI 프리뷰 수명 분리 |
| 호스트 SaveGame | 테스트 세션의 기록 보존; 온라인 서버 영속 저장은 아님 |

PR #3 기록상 `verify_dex` **3/3**을 통과했습니다. 실제 게임 진행으로 도감 등록·알림을 확인한 결과이며, 이번 문서 분리 작업에서 PIE/검증을 새로 실행한 것은 아닙니다.

## 관련 코드

- [PlayerDeed / PlayerState](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/AreaObject/Player/SonheimPlayerState.h)
- [MonsterDexComponent.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/AreaObject/Player/Utility/MonsterDexComponent.cpp)
- [MonsterDexTypes.h](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/MonsterDex/MonsterDexTypes.h)
- [MonsterDexSubsystem.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/MonsterDex/MonsterDexSubsystem.cpp)
- [MonsterDexStage.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/UI/MonsterDex/MonsterDexStage.cpp)
- [[17. Quest System|17_Quest_System]] — 같은 PlayerDeed를 활용한 목표 집계
- [[18. UMG v2 Migration|18_UMG_v2_Migration]] — 도감 화면 개편과 공통 UI
