# 08. Pal Capture & Partner Lifecycle

**야생 몬스터를 포획한 뒤 같은 `ABaseMonster` Actor를 보유·소환·전투에 재사용**합니다. 포획의 성공 여부는 서버가 결정하고, `UPalInventoryComponent`는 보유 목록과 선택 상태를 관리합니다. 소환 이후에는 `PartnerOwner`에 따라 기존 몬스터의 AI와 피아식별 규칙이 바뀝니다.

## 포획 시연

https://github.com/user-attachments/assets/57246d79-bd3b-473f-85fc-762670023729

PalSphere를 던지고 포획 확률·단계별 Reveal·성공/실패 결과가 표시되는 실제 게임 화면입니다.

## 핵심 구현

| 단계 | 담당 | 실제 처리 |
|---|---|---|
| **조준·투척** | `UPalCaptureComponent` · `APalSphere` | 포획률 UI·궤적 미리보기·투사체 충돌 |
| **성공 판정** | `UPalCaptureComponent` | 서버 포획률·랜덤 판정·Reveal 데이터 생성 |
| **보유 등록** | `ABaseMonster` · `UPalInventoryComponent` | `PartnerOwner` 설정·`OwnedPals` 등록 |
| **보관·선택** | `UPalInventoryComponent` (PlayerState) | OwnerOnly 목록·슬롯·선택 UI 갱신 |
| **소환·회수** | `UPalPartnerSkillComponent` (Pawn) | 소환 Montage 완료 후 Monster 활성/비활성 전환 |
| **Partner 행동** | `ABaseMonster` · Pal FSM | 소유자 추종·전투·Partner Skill·피아식별 |

## 목차

- [포획부터 Partner까지](#포획부터-partner까지)
- [투척과 포획 조건](#투척과-포획-조건)
- [서버 판정과 Reveal](#서버-판정과-reveal)
- [보유 목록과 선택 슬롯](#보유-목록과-선택-슬롯)
- [소환·회수와 Monster Actor 재사용](#소환회수와-monster-actor-재사용)
- [Partner AI와 피아식별](#partner-ai와-피아식별)
- [보스 포획 확장](#보스-포획-확장)
- [설계 선택과 비용](#설계-선택과-비용)
- [관련 코드 및 문서](#관련-코드-및-문서)

---

## 포획부터 Partner까지

~~~mermaid
flowchart TB
    WILD["야생 ABaseMonster"] --> SPHERE["PalSphere 충돌<br/>CanCapture"]
    SPHERE --> DEACT["DeactivateMonster<br/>포획 중 격리"]
    DEACT --> ROLL["서버 포획률·성공 판정"]
    ROLL --> REVEAL["Multicast Reveal<br/>단계별 연출"]
    REVEAL --> OUTCOME{"서버 최종 적용"}
    OUTCOME -->|"성공"| OWNER["SetPartnerOwner<br/>OwnedPals.AddPal"]
    OUTCOME -->|"실패"| RESTORE["ActivateMonster<br/>Aggro 복구"]
    OWNER --> STORED["보관: 비활성 Actor"]
    STORED --> SELECT["Pal 선택"]
    SELECT --> SUMMON["소환 Montage 완료"]
    SUMMON --> ACTIVE["ActivateMonster<br/>Partner FSM"]
    ACTIVE --> RECALL["회수·교체"]
    RECALL --> STORED
~~~

`PlayerState`에 둔 `UPalInventoryComponent`는 **어떤 Pal을 소유하고 선택했는지**를 관리합니다. Pawn의 `UPalCaptureComponent`와 `UPalPartnerSkillComponent`는 **투척·연출·소환·스킬 입력의 실행 과정**을 맡습니다.

두 컴포넌트는 `ASonheimPlayer::InitializeByPlayerState`에서 소유 목록에 연결됩니다. 전체 PlayerState/Pawn 책임은 [[04. Player & Character Systems|04_Player_Character_Systems]]에서 설명합니다.

---

## 투척과 포획 조건

### PalSphere 조준·투척

`UPalCaptureComponent`는 포획 준비 상태를 `bIsThrowingPalSphere`로 복제하고, 로컬에서는 포물선 궤적과 조준 UI를 갱신합니다.

| 흐름 | 주요 코드 |
|---|---|
| 준비 | `StartThrowPalSphere → Server_StartThrowPalSphere` |
| 던지기 | `ThrowPalSphere → Server_ThrowPalSphere` |
| 투사체 생성 | PalSphere 스킬 → `APalSphere` |
| 충돌 | `APalSphere::OnBeginOverlap` → `CheckPalCatch` |
| 포획 처리 | `UPalCaptureComponent::AttemptCapture` |

궤적 미리보기는 준비 중 로컬 입력을 돕고, 실제 포획 대상은 **서버 투사체 충돌에서 확인**합니다. `APalSphere::OnBeginOverlap`은 대상의 `CanCapture`를 검사하고 권한 측에서 `CheckPalCatch`를 호출합니다.

### 몬스터 포획 조건

~~~cpp
// BaseMonster.cpp
bool ABaseMonster::CanCapture() const
{
    return !IsDie()
        && !PartnerOwner
        && !HasCondition(EConditionBitsType::Hidden);
}
~~~

죽지 않았고, 소유자가 없으며, `Hidden` 상태가 아닌 몬스터만 일반 포획 대상입니다. `UPalCaptureComponent::AttemptCapture`는 추가로 몬스터 정의의 `bCapturable`을 확인합니다.

### HP·종 데이터 기반 확률

`CalculateCaptureRate`는 `FAreaObjectData`의 `CaptureBase`, `CaptureLowHPThreshold`, `CaptureResist`와 현재 HP 비율을 사용합니다.

~~~text
현재 HP 비율
    ↓
낮은 HP 구간: 기본 확률을 1에 가깝게 보정
    ↓
종별 CaptureResist 적용
    ↓
최종 확률 [0, 1]
~~~

실제 구현은 HP 비율이 `CaptureLowHPThreshold` 이하이면 저HP 구간 확률을 1로 두고, 그보다 높으면 `CaptureBase`부터 선형으로 보간합니다. 이후 `1 - CaptureResist`를 곱해 최종 값을 0~1로 제한합니다.

로컬 화면은 같은 계산으로 **예상 포획률**을 표시하지만, 성공 여부를 결정하는 랜덤 판정은 `Server_AttemptCapture`에서 수행합니다.

---

## 서버 판정과 Reveal

### 성공 여부와 화면 연출을 같은 데이터로 전달

서버는 계산된 포획률을 백분율로 반올림하고 `RandRange(1, 100)`으로 성공 여부를 결정합니다. 이어서 **`FPalCaptureRevealParams`를 한 번 생성**해 모든 클라이언트에 전달합니다.

| Reveal 필드 | 역할 |
|---|---|
| `Guess` · `bSuccess` | 화면에 전달하는 확률과 서버 판정 결과 |
| `Segments` · `SegmentTime` | 진행 단계 수와 단계별 시간 |
| `StartDelay` · `InterStageDelay` · `EndDelay` | 연출의 시작·단계 사이·종료 간격 |
| `FailStageOverride` | 실패 시 어느 단계에서 멈추는지 |

성공률 구간에 따라 Reveal 단계 수를 1~4개로 정하고, 실패라면 `PickFailStage`로 정지 위치를 선택합니다. `Multicast_BeginCaptureReveal`을 받은 `APalSphere`는 **서버가 결정한 동일 파라미터**로 진행 위젯을 재생합니다.

~~~mermaid
sequenceDiagram
    participant S as Server
    participant P as APalSphere
    participant M as Monster
    participant I as PalInventory

    P->>S: Sphere 충돌 / AttemptCapture
    S->>M: DeactivateMonster
    S->>S: 성공 판정·Reveal 파라미터 생성
    S-->>P: Multicast_BeginCaptureReveal
    P->>P: Capture Progress 재생
    S->>S: RevealTotal Timer
    alt 성공 + 보유 공간 확인
        S->>M: SetPartnerOwner
        S->>I: AddPal
    else 실패 또는 보유 공간 부족
        S->>M: ActivateMonster
        S->>M: SetAggroTarget
    end
~~~

### 포획 중 Monster 격리와 최종 적용

`AttemptCapture`는 대상에게 `DeactivateMonster`를 호출합니다. 이는 FSM 중지, 실행 중인 스킬 정리, `Hidden` 적용, 렌더링·충돌·HP 위젯·AI Tick 비활성화를 수행합니다.

서버는 Reveal이 진행되는 시간을 계산해 `ApplyCaptureOutcome`을 나중에 호출합니다. 이 시점에 **보유 공간을 다시 확인**합니다.

~~~cpp
// PalCaptureComponent.cpp — 성공 처리의 핵심 부분
if (PalInventory->GetOwnedPalCount() < PalInventory->MaxPalCount)
{
    TargetPal->SetPartnerOwner(OwnerPlayer);
    PalInventory->AddPal(TargetPal);
}
else
{
    bSuccess = false;
    NotifyPartyFull();
}
~~~

성공 시에는 몬스터 소유자와 Pal 목록을 갱신하고, 실패 시에는 몬스터를 다시 활성화해 포획을 시도한 플레이어를 Aggro 대상으로 설정합니다.

**랜덤 결과를 확정하는 시점과 실제 소유권을 적용하는 시점을 분리**한 이유는 Capture Reveal이 끝날 때까지 대상 상태를 유지하고, 그 사이 달라질 수 있는 보유 공간을 적용 직전에 확인하기 위해서입니다.

---

## 보유 목록과 선택 슬롯

### Monster 소유권과 플레이어 보유 목록

포획 성공은 서로 다른 두 상태에 반영됩니다.

| 위치 | 상태 | 용도 |
|---|---|---|
| `ABaseMonster` | `PartnerOwner : ASonheimPlayer*` | AI·피아식별에서 소유자 판단 |
| `UPalInventoryComponent` | `OwnedPals : TArray<ABaseMonster*>` | 플레이어가 보유한 Pal 목록 |
| `UPalInventoryComponent` | `CurrentPalIndex` | UI에서 선택한 보유 Pal |
| `UPalPartnerSkillComponent` | `SummonedPal` | 현재 월드에 활성화된 Partner |

**선택한 Pal과 실제 소환된 Pal은 별개**입니다. 다른 슬롯을 선택해도 `SummonedPal`을 즉시 변경하지 않고, 소환 요청과 Montage 완료 처리가 실제 전환을 수행합니다.

### 작은 배열의 OwnerOnly 복제

~~~cpp
// PalInventoryComponent.cpp
DOREPLIFETIME_CONDITION(
    UPalInventoryComponent, OwnedPals, COND_OwnerOnly);
DOREPLIFETIME_CONDITION(
    UPalInventoryComponent, CurrentPalIndex, COND_OwnerOnly);
~~~

`OwnedPals`는 FastArray가 아닌 **일반 복제 배열**입니다. `OnRep_OwnedPals`는 처음 받은 목록에서는 `OnPalAdded`를 발생시키고, 이후에는 `PrevOwnedPals`와 새 배열을 인덱스별로 비교해 `OnPalRemoved / OnPalAdded`를 전달합니다.

최초 동기화에는 `bInitialSyncInProgress`를 설정해 UI가 초기 목록 등록과 새 포획 연출을 구분할 수 있게 합니다.

### 슬롯 선택의 즉시 피드백

`SwitchPalSlot(Direction)`은 클라이언트에서 예상 인덱스를 계산해 `OnSelectedPalChanged`를 먼저 호출하고, `ServerRPC_SwitchPalSlot`으로 서버 변경을 요청합니다.

~~~text
슬롯 전환 입력
    ↓
로컬 예상 인덱스 → 선택 UI
    ↓
ServerRPC_SwitchPalSlot
    ↓
서버 CurrentPalIndex 변경
    ↓
OnRep_CurrentPalIndex → 최종 UI 반영
~~~

예상 인덱스는 **UI Delegate에만 전달**하고 로컬 `CurrentPalIndex`를 직접 덮어쓰지 않습니다. 최종 보유·선택 상태는 서버에서 복제된 값을 따릅니다.

---

## 소환·회수와 Monster Actor 재사용

### Montage 종료 후 실제 소환

`UPalPartnerSkillComponent::Server_TogglePalSummon`은 선택된 Pal과 현재 `SummonedPal`을 비교해 **소환·교체 또는 회수**를 예약합니다.

~~~text
TogglePalSummon
    ↓ Server_TogglePalSummon
SelectedPal == SummonedPal ?
    ├─ 예: bPendingDismiss = true
    └─ 아니오: PendingPal = SelectedPal
    ↓
MultiCast_PlaySummonAnimation
    ↓
Montage 정상 종료 · 서버
    ↓
ProcessPendingSummon
    ├─ 회수: DeactivateMonster · SummonedPal = null
    └─ 교체: 기존 Pal 비활성화
              → PendingPal.ActivateMonster
              → SummonedPal 갱신
~~~

`MultiCast_PlaySummonAnimation`은 플레이어의 무기·PalSphere Mesh를 바꾸고 Montage를 재생합니다. Montage 종료 Delegate에서는 각 클라이언트의 외형을 복구하고, **서버이며 Montage가 중단되지 않았을 때만** `ProcessPendingSummon`을 호출합니다.

`SummonedPal`은 일반 프로퍼티로 복제되고, `OnRep_SummonedPal`이 UI 등의 `OnPalSummoned / OnPalDismissed` Delegate를 발생시킵니다.

### 기존 Monster Actor 활성화·비활성화

`ABaseMonster::DeactivateMonster`는 몬스터를 Destroy하지 않고 다음 상태로 전환합니다.

| 보관·격리 중 | 다시 소환할 때 |
|---|---|
| FSM 중지·현재 Skill 정리 | `SelectMode`로 FSM 재진입 |
| `Hidden` Condition 추가 | `Hidden` 제거 |
| Actor 숨김·충돌 비활성화 | 렌더링·충돌 활성화 |
| HP 위젯·AI Tick 비활성화 | HP 위젯·AI Tick 활성화 |

`ActivateMonster`는 `PartnerOwner`가 있으면 소유자 옆으로 위치도 옮깁니다.

이 방식은 포획 전 몬스터의 **Actor 참조와 소유권을 유지**하면서, 월드에 참여하는 상태만 전환합니다. 기존 몬스터를 매번 새로 Spawn하는 대신 보유 목록의 같은 `ABaseMonster*`를 활성화합니다.

---

## Partner AI와 피아식별

### 소유자 추종에서 전투로

일반 Pal의 FSM은 `PartnerOwner`에 따라 행동 경로를 바꿉니다. `UPartnerPatrolMode::ServerExecute`에서 확인할 수 있는 대표 동작은 다음과 같습니다.

| 조건 | 동작 |
|---|---|
| Aggro Target이 있음 | `SelectMode`로 이동해 전투 상태 판단 |
| 소유자와 거리 2,000 초과 | Nav 위치를 검사한 뒤 소유자 근처로 순간이동 |
| 소유자와 거리 1,000 초과 | `PatrolToPlayer`로 추종 |
| 가까운 거리에서 대기 | 소유자 주변 Nav 지점으로 순찰 |
| Partner Skill 호출 중 | `MoveToPlayer` 등 Partner Skill 상태로 전환 |

`UPalPartnerSkillComponent::UpdateNearestEnemy`는 서버에서 `SummonedPal`이 있을 때 몬스터를 조회하고, 소유자 주변 800 유닛 이내의 적을 찾아 Aggro Target으로 설정합니다.

**선택한 상대와 실제 공격 방식은 몬스터의 기존 FSM·Skill 시스템**을 사용합니다. 포획 성공 시 새 Partner 전용 캐릭터를 Spawn하는 구조가 아닙니다.

### Partner Skill — 소환과 다른 상태

`TogglePartnerSkill`은 소환된 Pal에 `PartnerSkillStart / PartnerSkillEnd`를 호출하고, `SetPartnerSkillTrigger`는 버튼 입력을 `PartnerSkillTrigger`에 전달합니다. 일부 Pal은 `UPartnerSkillMode`에서 플레이어의 `PartnerWeapon` Socket에 부착되며, `bIsAttach`의 복제로 클라이언트 부착 상태를 맞춥니다.

`UPalPartnerSkillComponent`는 `SummonedPal`과 `bUsingPartnerSkill`을 각각 복제합니다. **소환된 상태와 Partner Skill을 사용 중인 상태는 같은 값이 아닙니다.**

### PartnerOwner를 이용한 공격 가능 대상 구분

`ABaseMonster::CanAttack`은 `PartnerOwner` 유무에 따라 공격 가능한 대상을 바꿉니다.

| 공격 주체 | 대상 | 처리 |
|---|---|---|
| 야생 몬스터 | 플레이어 | 허용 |
| 야생 몬스터 | 소유자가 있는 Pal | 허용 |
| 야생 몬스터 | 다른 야생 몬스터 | 제한 |
| Partner Pal | 야생 몬스터 | 허용 |
| Partner Pal | 플레이어·소유자가 있는 Pal | 제한 |

포획 후 `PartnerOwner`가 설정되면 **동일한 Monster 클래스에서 AI 경로와 피아식별 규칙이 함께 바뀝니다.**

---

## 보스 포획 확장

`ABossMonster`도 `ABaseMonster`를 상속하며 포획 가능 조건만 보스 상태에 맞게 확장합니다.

~~~cpp
// BossMonster.cpp
bool ABossMonster::CanCapture() const
{
    return Super::CanCapture() && Status.IsVulnerable();
}
~~~

현재 `FBossStatus::IsVulnerable`은 `Stage == EBossStage::Resting`일 때 참입니다. 따라서 보스는 **Resting 상태의 포획 가능 시간**에만 PalSphere의 `CanCapture` 검사를 통과합니다. 실제 포획률에는 몬스터 정의의 `bCapturable` 조건도 적용됩니다.

성공 판정·Reveal·보유 목록 등록은 일반 Pal과 같은 `UPalCaptureComponent` 경로를 사용합니다. 보스의 Resting 전환과 패턴·전투 상태는 [[10. Boss Encounter Runtime|10_Boss_Encounter_Runtime]]에서 다룹니다.

---

## 설계 선택과 비용

| 선택 | 구현상 이점 | 관리할 부분 |
|---|---|---|
| **서버 포획 판정·공통 Reveal 데이터** | 모든 클라이언트에 동일한 성공/실패 연출 | 연출 시간과 최종 상태 적용 시점 유지 |
| **적용 직전 보유 수 재확인** | Reveal 중 보유 공간 변경 반영 | 실패 복구·알림 처리 |
| **PlayerState의 보유 목록과 Pawn의 실행 분리** | 슬롯 UI·포획·소환의 역할 분리 | `InitializeByPlayerState` 초기화 경계 |
| **일반 배열 + OwnerOnly** | 작은 Pal 목록을 단순하게 복제 | `OnRep`에서 이전 목록과 인덱스 비교 |
| **Actor 활성/비활성 재사용** | 동일 Monster 참조·스탯·소유권 유지 | FSM·충돌·표시·AI 상태 일괄 전환 |
| **PartnerOwner 기반 AI/IFF** | 기존 몬스터 FSM·Skill 재사용 | 소유 여부에 따른 대상 판단 |
| **서버 Partner 대상 조회** | 소환된 Pal의 Aggro Target 선정 | Tick마다 Actor 검색·정렬 비용 |

## 관련 코드 및 문서

- [PalCaptureComponent.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/AreaObject/Player/Utility/PalCaptureComponent.cpp) — 조준·포획률·Reveal·최종 보유 처리
- [PalSphere.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/Element/Derived/Parabola/PalSphere.cpp) — 투사체 충돌·Reveal Widget
- [PalInventoryComponent.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/AreaObject/Player/Utility/PalInventoryComponent.cpp) — 보유·선택·OwnerOnly 복제
- [PalPartnerSkillComponent.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/AreaObject/Player/Utility/PalPartnerSkillComponent.cpp) — 소환·회수·Partner Skill
- [BaseMonster.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/AreaObject/Monster/BaseMonster.cpp) — 포획 조건·소유권·활성 전환·피아식별
- [PartnerPatrolMode.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/AreaObject/Monster/AI/Derived/AiMonster/BasePartnerAiState/PartnerPatrolMode.cpp) — 소유자 추종·순찰·순간이동
- [PartnerSkillMode.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/AreaObject/Monster/AI/Derived/AiMonster/BasePartnerAiState/PartnerSkillMode.cpp) — Partner Skill 부착
- [BossMonster.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/Sonheim/Source/Sonheim/AreaObject/Monster/Boss/BossMonster.cpp) — Resting 포획 조건
- [[04. Player & Character Systems|04_Player_Character_Systems]] — PlayerState와 Pawn 기능 연결
- [[05. Combat, Skill & Animation|05_Combat_Skill_Animation]] — Monster·Partner 전투 경로
- [[10. Boss Encounter Runtime|10_Boss_Encounter_Runtime]] — 보스의 포획 가능 단계
- [[12. Multiplayer Synchronization|12_Multiplayer_Synchronization]] — Pal 선택 복제·Reveal와 상태 분리
- [[16. MonsterDex System|16_MonsterDex_System]] — 포획 이후 도감 기록·등록 알림
