#include "LevelComponent.h"
//#include "StatBonusComponent.h"
#include "Sonheim/AreaObject/Base/AreaObject.h"
#include "Sonheim/GameManager/SonheimGameInstance.h"
#include "Sonheim/Utilities/LogMacro.h"

TMap<int32, FLevelData>* ULevelComponent::dt_Level;

ULevelComponent::ULevelComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// 기본값 설정
	CurrentLevel = 1;
	CurrentExp = 0;
	MaxLevel = 50;
	AvailableStatPoints = 0;
	StatPointsPerLevel = 3;
}

void ULevelComponent::InitLevel(class AAreaObject* Parent)
{
	m_Owner = Parent;
	ExpToNextLevel = GetExpForLevel(CurrentLevel);
}

void ULevelComponent::BeginPlay()
{
	Super::BeginPlay();

	// 레벨 데이터 테이블이 설정되었는지 확인
	if (!dt_Level)
	{
		USonheimGameInstance* gameInstance = Cast<USonheimGameInstance>(GetOwner()->GetGameInstance());
		ULevelComponent::dt_Level = gameInstance->GetDataLevel();
		if (dt_Level->IsEmpty())
		{
			UE_LOG(LogTemp, Error, TEXT("LevelComponent: LevelDataTable is not set!"));
			return;
		}
	}
}

void ULevelComponent::AddExp(int32 ExpAmount)
{
	// 최대 레벨이면 경험치 획득 불가
	if (CurrentLevel >= MaxLevel)
	{
		CurrentExp = 0.0f;
		ExpToNextLevel = 0.0f;
		OnExperienceChanged.Broadcast(CurrentExp, ExpToNextLevel, 0);
		return;
	}

	CurrentExp += ExpAmount;

	// 레벨업 조건 체크
	while (CurrentExp >= ExpToNextLevel && CurrentLevel < MaxLevel)
	{
		HandleLevelUp();
	}

	OnExperienceChanged.Broadcast(CurrentExp, ExpToNextLevel, ExpAmount);
}

void ULevelComponent::SetLevel(int32 NewLevel)
{
	if (NewLevel <= 0 || NewLevel > MaxLevel || NewLevel == CurrentLevel)
	{
		return;
	}

	int32 OldLevel = CurrentLevel;
	CurrentLevel = NewLevel;

	// 경험치 초기화
	CurrentExp = 0.0f;
	ExpToNextLevel = GetExpForLevel(CurrentLevel);

	// 레벨 변경에 따른 스탯 보너스 적용
	ApplyLevelUpBonuses(OldLevel, NewLevel);

	// 레벨업 델리게이트 호출
	bool bLevelUp = NewLevel > OldLevel;
	OnLevelChanged.Broadcast(OldLevel, NewLevel, bLevelUp);
	OnExperienceChanged.Broadcast(CurrentExp, ExpToNextLevel, 0);
}

void ULevelComponent::SetExp(int32 NewExp)
{
	if (NewExp < 0.0f || CurrentLevel >= MaxLevel)
	{
		return;
	}

	CurrentExp = NewExp;

	// 레벨업 조건 체크
	while (CurrentExp >= ExpToNextLevel && CurrentLevel < MaxLevel)
	{
		HandleLevelUp();
	}

	OnExperienceChanged.Broadcast(CurrentExp, ExpToNextLevel, NewExp);
}

void ULevelComponent::UseStatPoints(int32 Points)
{
	if (Points <= 0 || Points > AvailableStatPoints)
	{
		return;
	}

	AvailableStatPoints -= Points;
}

int32 ULevelComponent::GetExpForLevel(int32 Level)
{
	if (!dt_Level)
	{
		UE_LOG(LogTemp, Error, TEXT("LevelComponent: LevelDataTable is not set!"));
		return 0;
	}

	// 레벨 데이터 검색
	FLevelData* LevelData = dt_Level->Find(Level);
	if (!LevelData)
	{
		UE_LOG(LogTemp, Error, TEXT("LevelComponent: Failed to find data for level %d"), Level);
		return 100 * Level; // 폴백 값
	}

	// 경험치 테이블 타입에 따라 적절한 경험치 값 반환
	if (EAreaObjectType::Player == m_Owner->dt_AreaObject->AreaObjectType)
	{
		return LevelData->PlayerExp;
	}
	else // Pal 타입
	{
		return LevelData->PalExp;
	}
}

void ULevelComponent::HandleLevelUp()
{
	int32 OldLevel = CurrentLevel;
	CurrentLevel++;

	// 레벨업 시 경험치 조정
	CurrentExp -= ExpToNextLevel;
	ExpToNextLevel = GetExpForLevel(CurrentLevel);

	// 스탯 포인트 추가
	AvailableStatPoints += StatPointsPerLevel;

	// 레벨업에 따른 스탯 보너스 적용
	ApplyLevelUpBonuses(OldLevel, CurrentLevel);

	// 레벨업 델리게이트 호출
	OnLevelChanged.Broadcast(OldLevel, CurrentLevel, true);
	FLog::Log("Current Level is ", CurrentLevel);
}

void ULevelComponent::ApplyLevelUpBonuses(int32 OldLevel, int32 NewLevel)
{
	if (!m_Owner)
	{
		return;
	}

	// ToDo 스탯 보너스 컴포넌트 가져오기 - 검증 이후 커밋
	/*
	UStatBonusComponent* StatComp = m_Owner->FindComponentByClass<UStatBonusComponent>();
	if (!StatComp)
	{
		return;
	}

	// 레벨 변경에 따른 스탯 보너스 적용
	int32 LevelDiff = NewLevel - OldLevel;
	if (LevelDiff > 0)
	{
		// 레벨업: 스탯 보너스 추가
		float HPBonus = 10.0f * LevelDiff;
		float AttackBonus = 2.0f * LevelDiff;
		float DefenseBonus = 1.0f * LevelDiff;

		StatComp->AddStatBonus(EAreaObjectStatType::HP, HPBonus, EStatModifierType::Additive, -1);
		StatComp->AddStatBonus(EAreaObjectStatType::Attack, AttackBonus, EStatModifierType::Additive, -1);
		StatComp->AddStatBonus(EAreaObjectStatType::Defense, DefenseBonus, EStatModifierType::Additive, -1);
	}
	else if (LevelDiff < 0)
	{
		// 레벨 다운: 스탯 보너스 제거 (디버그 용도)
		StatComp->AddStatBonus(EAreaObjectStatType::HP, 10.0f * LevelDiff, EStatModifierType::Additive, -1);
		StatComp->AddStatBonus(EAreaObjectStatType::Attack, 2.0f * LevelDiff, EStatModifierType::Additive, -1);
		StatComp->AddStatBonus(EAreaObjectStatType::Defense, 1.0f * LevelDiff, EStatModifierType::Additive, -1);
	}
	*/
}
