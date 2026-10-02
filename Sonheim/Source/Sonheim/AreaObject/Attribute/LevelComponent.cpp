// LevelComponent.cpp
#include "LevelComponent.h"

#include "StatBonusComponent.h"
#include "Net/UnrealNetwork.h"
#include "Sonheim/AreaObject/Base/AreaObject.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerController.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerState.h"
#include "Sonheim/GameManager/SonheimGameInstance.h"
#include "Sonheim/Utilities/StringTableIds.h"

ULevelComponent::ULevelComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    
    // 컴포넌트 리플리케이션 활성화
    SetIsReplicatedByDefault(true);

    // 기본값 설정
    CurrentLevel = 1;
    CurrentExp = 0;
    ExpToNextLevel = 0;
    MaxLevel = 50;
    AvailableStatPoints = 0;
    StatPointsPerLevel = 3;
    ClientPreviousLevel = 1;

    LevelUpNotice.Category = LOCTABLE(SONHEIM_ST_NOTICE, "LevelUp.Category");
    LevelUpNotice.Title = LOCTABLE(SONHEIM_ST_NOTICE, "LevelUp.Title");
    LevelUpNotice.Detail = LOCTABLE(SONHEIM_ST_NOTICE, "LevelUp.Detail");
    LevelUpNotice.Symbol = INVTEXT("★");
    LevelUpNotice.Icon = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/CuratedDungeon/UI/Icons/T_Icon_Star.T_Icon_Star")));
    LevelUpNotice.Tone = ENoticeTone::Highlight;
}

void ULevelComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // 주요 변수들 리플리케이션
    DOREPLIFETIME(ULevelComponent, CurrentLevel);
    DOREPLIFETIME(ULevelComponent, CurrentExp);
    DOREPLIFETIME(ULevelComponent, ExpToNextLevel);
    DOREPLIFETIME(ULevelComponent, AvailableStatPoints);
}

void ULevelComponent::InitLevel(AAreaObject* Parent)
{
    m_Owner = Parent;
    
    // 서버에서만 초기화
    if (GetOwnerRole() == ROLE_Authority)
    {
        ExpToNextLevel = GetExpForLevel(CurrentLevel);
    }
}

int32 ULevelComponent::RewardHuntExp() const
{
    if (!dt_Level)
    {
        return 0;
    }
    if (auto levelData = dt_Level->Find(CurrentLevel))
    {
        return levelData->HuntExp;
    }
    
    return 0;
}

void ULevelComponent::BeginPlay()
{
    Super::BeginPlay();

    // 레벨 데이터 테이블 로드
    if (!dt_Level)
    {
        if (USonheimGameInstance* gameInstance = Cast<USonheimGameInstance>(GetOwner()->GetGameInstance()))
        {
            dt_Level = gameInstance->GetDataLevel();
            if (!dt_Level || dt_Level->IsEmpty())
            {
                UE_LOG(LogTemp, Error, TEXT("LevelComponent: LevelDataTable is not set!"));
            }
        }
    }

    // 클라이언트용 초기값 설정
    ClientPreviousLevel = CurrentLevel;
}

void ULevelComponent::AddExp(int32 ExpAmount)
{
    // 서버에서만 처리
    if (GetOwnerRole() != ROLE_Authority)
        return;

    // 최대 레벨 체크
    if (CurrentLevel >= MaxLevel)
    {
        CurrentExp = 0;
        ExpToNextLevel = 0;
        OnExperienceChanged.Broadcast(CurrentExp, ExpToNextLevel, 0);
        return;
    }

    int32 OldExp = CurrentExp;
    CurrentExp += ExpAmount;

    // 레벨업 체크
    bool bLeveledUp = false;
    while (CurrentExp >= ExpToNextLevel && CurrentLevel < MaxLevel)
    {
        HandleLevelUp();
        bLeveledUp = true;
    }

    // 경험치 변경 이벤트는 서버에서만 발생
    // OnRep 함수가 클라이언트 처리
    OnExperienceChanged.Broadcast(CurrentExp, ExpToNextLevel, ExpAmount);
}

void ULevelComponent::SetLevel(int32 NewLevel)
{
    // 서버에서만 처리
    if (GetOwnerRole() != ROLE_Authority)
        return;

    // 유효성 검사
    if (NewLevel <= 0 || NewLevel > MaxLevel || NewLevel == CurrentLevel)
        return;

    int32 OldLevel = CurrentLevel;
    CurrentLevel = NewLevel;

    // 경험치 초기화
    CurrentExp = 0;
    ExpToNextLevel = GetExpForLevel(CurrentLevel);

    // 레벨 변경에 따른 처리
    bool bLevelUp = NewLevel > OldLevel;
    if (bLevelUp)
    {
        // 레벨업 시 스탯 포인트 계산
        int32 LevelDiff = NewLevel - OldLevel;
        AvailableStatPoints += StatPointsPerLevel * LevelDiff;
    }

    // 스탯 보너스 적용
    ApplyLevelUpBonuses(OldLevel, NewLevel);

    // 서버에서 이벤트 발생
    OnLevelChanged.Broadcast(OldLevel, NewLevel, bLevelUp);
    OnExperienceChanged.Broadcast(CurrentExp, ExpToNextLevel, 0);
}

void ULevelComponent::SetExp(int32 NewExp)
{
    // 서버에서만 처리
    if (GetOwnerRole() != ROLE_Authority)
        return;

    // 유효성 검사
    if (NewExp < 0 || CurrentLevel >= MaxLevel)
        return;

    int32 OldExp = CurrentExp;
    CurrentExp = NewExp;

    // 레벨업 체크
    while (CurrentExp >= ExpToNextLevel && CurrentLevel < MaxLevel)
    {
        HandleLevelUp();
    }

    OnExperienceChanged.Broadcast(CurrentExp, ExpToNextLevel, NewExp - OldExp);
}

void ULevelComponent::UseStatPoints(int32 Points)
{
    // 서버에서만 처리
    if (GetOwnerRole() != ROLE_Authority)
        return;

    // 유효성 검사
    if (Points <= 0 || Points > AvailableStatPoints)
        return;

    AvailableStatPoints -= Points;
}

int32 ULevelComponent::GetExpForLevel(int32 Level) const
{
    if (!dt_Level)
    {
        UE_LOG(LogTemp, Error, TEXT("LevelComponent: LevelDataTable is not set!"));
        return 100 * Level; // 폴백 값
    }

    // 레벨 데이터 검색
    FLevelData* LevelData = dt_Level->Find(Level);
    if (!LevelData)
    {
        UE_LOG(LogTemp, Error, TEXT("LevelComponent: Failed to find data for level %d"), Level);
        return 100 * Level; // 폴백 값
    }

    // 타입에 따른 경험치 반환
    if (m_Owner && m_Owner->dt_AreaObject)
    {
        return (m_Owner->dt_AreaObject->AreaObjectType == EAreaObjectType::Player) 
            ? LevelData->PlayerExp 
            : LevelData->PalExp;
    }

    return LevelData->PlayerExp; // 기본값
}

void ULevelComponent::HandleLevelUp()
{
    int32 OldLevel = CurrentLevel;
    CurrentLevel++;

    // 경험치 조정
    CurrentExp -= ExpToNextLevel;
    ExpToNextLevel = GetExpForLevel(CurrentLevel);

    // 스탯 포인트 추가
    AvailableStatPoints += StatPointsPerLevel;

    // 스탯 보너스 적용
    ApplyLevelUpBonuses(OldLevel, CurrentLevel);

    // 서버 이벤트
    OnLevelChanged.Broadcast(OldLevel, CurrentLevel, true);

    // 레벨이 오른 플레이어에게 배너를 보낸다. 서버가 정하므로 호스트와 클라이언트가 같은 길로 받는다.
    if (const ASonheimPlayer* Player = Cast<ASonheimPlayer>(m_Owner))
    {
        if (ASonheimPlayerController* Controller = Player->GetController<ASonheimPlayerController>())
        {
            FNoticeData Data = LevelUpNotice;
            Data.Title = FText::Format(LevelUpNotice.Title, CurrentLevel);
            Controller->PushNotice(ENoticeSlot::Banner, TEXT("Level"), Data);
        }
    }
}

void ULevelComponent::ApplyLevelUpBonuses(int32 OldLevel, int32 NewLevel)
{
    if (!m_Owner) return;

    UStatBonusComponent* StatComp = nullptr;

    // Owner가 플레이어인지 확인
    if (ASonheimPlayer* Player = Cast<ASonheimPlayer>(m_Owner))
    {
        // 플레이어라면 PlayerState에서 StatBonusComponent를 찾음
        if (ASonheimPlayerState* PS = Player->GetSPlayerState())
        {
            StatComp = PS->FindComponentByClass<UStatBonusComponent>();
        }
    }
    else
    {
        // 플레이어가 아니라면 (몬스터 등) Owner 자신에게서 찾음
        StatComp = m_Owner->FindComponentByClass<UStatBonusComponent>();
    }

    if (StatComp)
    {
        int32 LevelDiff = NewLevel - OldLevel;
        if (LevelDiff != 0)
        {
            float HPBonus = 10.0f * LevelDiff;
            float AttackBonus = 2.0f * LevelDiff;
            float DefenseBonus = 1.0f * LevelDiff;

            StatComp->AddStatBonus(EAreaObjectStatType::HP, HPBonus, EStatModifierType::Additive, -1);
            StatComp->AddStatBonus(EAreaObjectStatType::Attack, AttackBonus, EStatModifierType::Additive, -1);
            StatComp->AddStatBonus(EAreaObjectStatType::Defense, DefenseBonus, EStatModifierType::Additive, -1);
        }
    }

    // HP 회복 (레벨업 시)
    if (NewLevel > OldLevel && m_Owner)
    {
        // 레벨업 시 최대 HP로 회복
        m_Owner->SetHPByRate(1.0f);
    }
}

void ULevelComponent::OnRep_CurrentLevel(int32 OldLevel)
{
    // 클라이언트에서 레벨 변경 감지
    bool bLevelUp = CurrentLevel > ClientPreviousLevel;
    
    // 레벨업 이벤트 발생. 레벨 업 배너는 서버가 HandleLevelUp에서 보낸다.
    OnLevelChanged.Broadcast(ClientPreviousLevel, CurrentLevel, bLevelUp);

    // TODO: 레벨업 파티클 효과, 사운드 재생 등

    ClientPreviousLevel = CurrentLevel;
}

void ULevelComponent::OnRep_CurrentExp(int32 OldExp)
{
    // 클라이언트에서 경험치 변경 감지
    int32 Delta = CurrentExp - OldExp;
    
    // UI 업데이트를 위한 이벤트. 경험치는 상태 위젯의 막대가 보인다.
    OnExperienceChanged.Broadcast(CurrentExp, ExpToNextLevel, Delta);
}

void ULevelComponent::OnRep_AvailableStatPoints()
{
    // 클라이언트에서 스탯 포인트 변경 알림
    // 스탯 포인트를 쓰는 화면이 생기면 여기서 갱신한다.
}