# 5. Multiplayer State & UI Pipeline

> **이 문서가 답하는 질문**
>
> “Server의 Dungeon 상태를 Widget이 직접 읽거나 RPC를 놓치지 않고, Client에서 언제든 현재 화면을 다시 구성하려면?”

Dungeon UI는 단순 Delegate 하나로 설명하기 어렵습니다.

핵심 흐름:

```text
Server Runtime State
       ↓ replication
ASonheimGameState
       ↓
UDungeonStagePresenter
       ↓
FDungeonStageViewData
       ↓
UDungeonUIRouterSubsystem
       ↓
UMG
```

---

# Part 1. Gameplay State와 UI Data는 같은 것이 아니다

## 1.1 Runtime Snapshot

Client가 받는 authoritative state의 대표 필드:

```cpp
USTRUCT(BlueprintType)
struct FDungeonStageRuntimeState
{
    GENERATED_BODY()

    FGuid RunId;
    FGameplayTag StageId;
    int32 Revision = 0;
    EDungeonRunStatus RunStatus;

    FGameplayTag ObjectiveGroupId;
    int32 CurrentCount = 0;
    int32 RequiredCount = 0;

    FGameplayTag SelectedBranchId;
    FGameplayTagContainer RunTags;

    double StageDeadlineServerTime = 0;

    TArray<FDungeonRunReward> Rewards;
    TArray<FDungeonGroupTally> Groups;

    TArray<TObjectPtr<APlayerState>> Participants;

    float BossHealth = 0.f;
    FGameplayTag BossActionId;
    int32 BossPhase = 0;
    bool bBossVulnerable = false;
    float BossBreak = 0.f;
};
```

이 구조는 “화면용 text”가 아니라 **현재 Run이 실제로 어떤 상태인가**를 표현합니다.

---

## 1.2 ViewData

Presenter가 Widget에 넘기는 타입은 다릅니다.

```cpp
USTRUCT(BlueprintType)
struct FDungeonStageViewData
{
    GENERATED_BODY()

    FText DungeonTitle;
    FText Goal;

    TArray<FDungeonStepViewData> Steps;
    TArray<FDungeonObjectiveViewData> Objectives;
    TArray<FDungeonObjectiveViewData> OptionalObjectives;

    TArray<FDungeonMarkerViewData> Markers;

    UTexture2D* MapTexture;
    TArray<FDungeonMapRoomViewData> MapRooms;

    TArray<FDungeonMemberViewData> Members;

    FText BossName;
    float BossHealth = 0.f;
    FText BossActionText;
    FText BossPhaseText;
    FText BossHintText;

    TArray<FDungeonRewardViewData> Rewards;
    TArray<FDungeonStatViewData> Stats;

    double DeadlineServerTime = 0;
    EDungeonRunStatus Status;
    bool bParticipant = true;
    int32 Revision = 0;
};
```

Runtime의 GameplayTag나 ItemId를 Widget이 직접 해석하지 않습니다.

---

# Part 2. Presenter가 하는 일

예를 들어 Runtime에는:

```text
StageId = Dungeon.ForgottenRuins.Stage.GuardRoom
ObjectiveGroupId = Dungeon.ForgottenRuins.Group.Guards
CurrentCount = 2
RequiredCount = 4
```

가 있다고 가정합니다.

Presenter는 Definition / Presentation Asset을 조합해:

```text
Title      = "경비실"
Objective  = "경비병을 처치하세요"
Count      = "2 / 4"
Icon       = Guard Icon
Marker     = Guard Room World Position
```

같은 ViewData를 만듭니다.

즉 Presenter는 **authoritative state를 presentation language로 번역하는 계층**입니다.

---

# Part 3. 왜 Widget이 Snapshot을 직접 읽지 않는가

Widget이 직접:

- GameplayTag를 해석하고
- DataAsset을 찾고
- ItemId를 이름으로 변환하고
- Objective kind를 판단하고
- Boss pattern text를 찾기 시작하면

Gameplay schema와 UMG가 강하게 결합됩니다.

Widget은 최종 ViewData만 소비합니다.

```text
Gameplay changes
→ Presenter 수정

Widget layout changes
→ Widget 수정
```

책임을 분리합니다.

---

# Part 4. LocalPlayer UI Router

`UDungeonUIRouterSubsystem : ULocalPlayerSubsystem`은 **어느 LocalPlayer에게 어떤 화면을 띄울지**를 관리합니다.

주요 책임:

- participant 여부
- Dungeon HUD 생성/제거
- Result 화면
- 다른 HUD hide/restore
- Latest ViewData cache
- Notice channel
- async Widget asset loading

Gameplay Runtime은 “이 Client에서 어떤 Widget을 생성해야 하는가”를 알지 않습니다.

---

# Part 5. UI가 늦게 만들어져도 현재 상태를 복구한다

RPC만으로:

```text
"Stage Changed!"
"Boss Phase Changed!"
```

같은 event를 보내면 Widget이 없던 순간의 event는 사라집니다.

Snapshot은 현재 상태 자체를 보관합니다.

그래서 Widget이 다시 생성돼도 Latest State로 화면을 재구성할 수 있습니다.

이 차이는 특히:

- HUD recreation
- late binding
- network relevancy
- local screen transition

에서 중요합니다.

---

# Part 6. Revision

같은 Stage에서도 Objective count, Boss state, Reward 등이 여러 번 바뀝니다.

그래서 “StageId가 같으니 변화 없음”으로 판단할 수 없습니다.

```cpp
int32 Revision = 0;
```

을 사용해 동일 Stage 내부의 presentation update도 구분합니다.

---

# Part 7. Timer는 Client 수신 시각이 아니라 Server 시각을 기준으로 한다

Snapshot은:

```cpp
double RunStartedServerTime;
double StageDeadlineServerTime;
double BossActionStartServerTime;
double BossActionEndServerTime;
```

를 전달합니다.

Client가 packet을 받은 순간부터 30초를 세는 것이 아니라 synchronized server clock을 기준으로 남은 시간을 계산합니다.

이렇게 하면:

- latency
- HUD recreation
- 서로 다른 frame rate

가 있어도 같은 logical timer를 바라봅니다.

---

# Part 8. 모든 UI를 Snapshot으로 만들지는 않는다

Health처럼 이미 Actor Component가 자신의 replicated state와 delegate를 갖는 값은 그대로 사용합니다.

예:

```text
UHealthComponent
  ↓ RepNotify
OnHealthChanged
  ↓
Participant Health UI
```

Dungeon progression처럼 여러 값이 한 묶음으로 현재 콘텐츠 상태를 구성할 때 Snapshot이 유리합니다.

즉:

- 독립 상태 변화 → Delegate
- 복합 콘텐츠 상태 → Snapshot + Presenter

로 구분합니다.

---

# Part 9. Notice는 또 다른 문제다

Room Title, Level Up, Capture 실패는 “현재 상태”라기보다 일시적 메시지입니다.

`UNoticeSubsystem`은:

- Banner / Title slot
- queue
- Replace / TakeTurns
- producer Channel

을 관리합니다.

Transient presentation event를 Runtime Snapshot에 억지로 저장하지 않습니다.

---

# Part 10. UI 초기화 Race

일반 Player HUD에서는 Controller와 PlayerState가 Client에 도착하는 순서가 고정되지 않습니다.

Remote Client:

```text
OnRep_Controller ─┐
                  ├→ TryInitHUD_OnClient
OnRep_PlayerState ┘
```

Host:

```text
PossessedBy (Server)
 → Client RPC
 → HUD Init
```

어느 callback이 먼저 오는지에 의존하지 않고 필요한 reference가 모두 준비됐을 때 한 번만 실행합니다.

---

# 이 문서 다음에 읽기

- Snapshot을 만드는 Server Runtime → [[4. Branching Dungeon Runtime|4_Branching_Dungeon_Runtime]]
- Presentation data의 출처 → [[3. Data & Content Architecture|3_Data_Content_Architecture]]
- Notice와 검증 workflow → [[13. Development Workflow & Verification|13_Development_Workflow_Verification]]

---

## 관련 코드

- [DungeonStageRuntimeTypes.h](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/GameManager/Dungeon/DungeonStageRuntimeTypes.h)
- [DungeonViewData.h](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/UI/Dungeon/DungeonViewData.h)
- [DungeonStagePresenter.cpp](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/UI/Dungeon/DungeonStagePresenter.cpp)
- [DungeonUIRouterSubsystem.h](https://github.com/chungheonLee0325/Sonheim/blob/main/Sonheim/Source/Sonheim/UI/Dungeon/DungeonUIRouterSubsystem.h)
