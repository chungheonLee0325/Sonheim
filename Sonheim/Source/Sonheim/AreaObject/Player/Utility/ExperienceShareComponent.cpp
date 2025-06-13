#include "ExperienceShareComponent.h"
#include "Sonheim/AreaObject/Monster/BaseMonster.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Sonheim/AreaObject/Attribute/LevelComponent.h"
#include "Engine/World.h"
#include "Sonheim/AreaObject/Player/Utility/PalManagementComponent.h"
#include "Sonheim/Utilities/LogMacro.h"

UExperienceShareComponent::UExperienceShareComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UExperienceShareComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// 오너에서 필요한 컴포넌트 찾기
	if (AActor* Owner = GetOwner())
	{
		OwnerLevelComponent = Owner->FindComponentByClass<ULevelComponent>();
		PalManagementComponent = Owner->FindComponentByClass<UPalManagementComponent>();
		
		// 레벨 컴포넌트의 경험치 획득 이벤트에 바인딩
		if (OwnerLevelComponent)
		{
			// OnExpGained 델리게이트가 있다면 바인딩
			// OwnerLevelComponent->OnExpGained.AddDynamic(this, &UExperienceShareComponent::ShareExperience);
		}
	}
}

void UExperienceShareComponent::ShareExperience(int32 Exp)
{
	if (!bIsShareEnabled || !PalManagementComponent || Exp <= 0)
		return;
	
	// 플레이어가 받을 경험치
	int32 PlayerExp = Exp * (1.0f - ShareRatio);
	
	// 팰들이 나눠 받을 경험치
	int32 PalShareExp = Exp * ShareRatio;
	
	// 플레이어에게 경험치 지급
	if (OwnerLevelComponent && PlayerExp > 0)
	{
		OwnerLevelComponent->AddExp(PlayerExp);
	}
	
	// 팰들에게 경험치 분배
	if (PalShareExp > 0)
	{
		DistributeExperience(PalShareExp);
	}
}

void UExperienceShareComponent::DistributeExperience(int32 ExpAmount)
{
	if (!PalManagementComponent)
		return;
	
	TArray<ABaseMonster*> OwnedPals = PalManagementComponent->GetOwnedPals();
	ABaseMonster* SummonedPal = PalManagementComponent->GetSummonedPal();
	
	if (OwnedPals.Num() == 0)
		return;
	
	// 각 팰의 경험치 가중치 계산
	TMap<ABaseMonster*, float> ExpWeights;
	float TotalWeight = 0.0f;
	
	for (ABaseMonster* Pal : OwnedPals)
	{
		if (!IsValid(Pal))
			continue;
		
		float Weight = 1.0f;
		
		// 소환된 팰 보너스
		if (Pal == SummonedPal)
		{
			Weight *= SummonedPalExpMultiplier;
		}
		
		// 거리 수정자
		if (bUseDistanceModifier)
		{
			Weight *= CalculateDistanceModifier(Pal);
		}
		
		// 전투 기여도 수정자
		if (bUseBattleContribution)
		{
			Weight *= CalculateBattleContributionModifier(Pal);
		}
		
		ExpWeights.Add(Pal, Weight);
		TotalWeight += Weight;
	}
	
	// 가중치에 따라 경험치 분배
	if (TotalWeight > 0.0f)
	{
		for (const auto& Pair : ExpWeights)
		{
			ABaseMonster* Pal = Pair.Key;
			float Weight = Pair.Value;
			
			int32 PalExp = FMath::RoundToInt((ExpAmount * Weight) / TotalWeight);
			
			if (PalExp > 0 && Pal->m_LevelComponent)
			{
				Pal->m_LevelComponent->AddExp(PalExp);
				
				// 이벤트 발생
				OnExpShared.Broadcast(Pal, PalExp, Weight / TotalWeight);
				
				#if !UE_BUILD_SHIPPING
				FLog::Log("Pal %s gained %d exp (%.1f%% share)", 
					*Pal->GetName(), PalExp, (Weight / TotalWeight) * 100.0f);
				#endif
			}
		}
	}
}

float UExperienceShareComponent::CalculateDistanceModifier(ABaseMonster* Pal) const
{
	if (!Pal || !GetOwner())
		return 1.0f;
	
	float Distance = FVector::Dist(GetOwner()->GetActorLocation(), Pal->GetActorLocation());
	
	// 최대 거리를 초과하면 경험치를 받지 못함
	if (Distance > MaxShareDistance)
		return 0.0f;
	
	// 거리에 따른 선형 감소
	float DistanceRatio = Distance / MaxShareDistance;
	return FMath::Lerp(1.0f, 0.5f, DistanceRatio);
}

float UExperienceShareComponent::CalculateBattleContributionModifier(ABaseMonster* Pal) const
{
	if (!Pal)
		return 1.0f;
	
	// 전투 기여도 확인
	if (const float* Contribution = BattleContributionMap.Find(Pal))
	{
		// 기여도가 있으면 추가 보너스
		return FMath::Lerp(1.0f, BattleParticipantExpMultiplier, *Contribution);
	}
	
	return 1.0f;
}

void UExperienceShareComponent::ResetBattleContributions()
{
	BattleContributionMap.Empty();
}