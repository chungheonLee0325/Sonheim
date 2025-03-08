// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseSkillRoulette.h"

#include "BaseMonster.h"


// Sets default values for this component's properties
UBaseSkillRoulette::UBaseSkillRoulette()
{
	PrimaryComponentTick.bCanEverTick = false;
}


void UBaseSkillRoulette::BeginPlay()
{
	Super::BeginPlay();
	Owner = Cast<ABaseMonster>(GetOwner());
}

void UBaseSkillRoulette::AdjustSkillWeightsByCombatContext(TArray<FSkillRouletteEntry>& LocalEntries)
{
	float dist = Owner->GetDistToTarget();
	FVector dir = Owner->GetDirToTarget();
	dir.Normalize();
	float forwardDot = FVector::DotProduct(dir, Owner->GetActorForwardVector());
	float rightDot = FVector::DotProduct(dir, Owner->GetActorRightVector());
	
	//직전 스킬 타입이 거리벌리기이면, 원거리 공격 위주로 사용
	if (PrevSkillType == EAiSkillType::Back)
	{
		// LOG_SCREEN("원거리 공격!");
		ApplySkillWeight(LocalEntries, EAiSkillType::Long, 1000.0f);
	}
	//직전 스킬 타입이 원거리 공격이면, 원거리 공격 확률 DOWN
	else if (PrevSkillType == EAiSkillType::Long)
	{
		ApplySkillWeight(LocalEntries, EAiSkillType::Long, 0.25f);
	}
	// 거리가 멀면, 원거리 공격 활성화.
	else if (dist > LongRange)
	{
		// LOG_SCREEN("원거리 공격 활성화");
		ApplySkillWeight(LocalEntries, EAiSkillType::Long, 400.0f);
	}
	// 거리가 짧으면, 위빙 스킬 확률 UP
	else if (dist <= ShortRange)
	{
		ApplySkillWeight(LocalEntries, EAiSkillType::Weaving, 1.5f);
	}
	// 플레이어가 뒤에 있다면, 뒤도는 스킬 확률 UP
	else if (forwardDot < -0.2f)
	{
			SetSkillWeight(LocalEntries, EAiSkillType::Back, 3.0f);
	}
	else if (forwardDot > 0.0f && rightDot >= 0.15f)
	{
		// LOG_PRINT(TEXT("우측 대각선!"));
		if ((PrevSkillType != EAiSkillType::Right) && (PrevSkillType != EAiSkillType::Left))
		{
			SetSkillWeight(LocalEntries, EAiSkillType::Right, 3.0f);
		}
	}
	else if (forwardDot > 0.0f && rightDot <= 0.15f)
	{
		// LOG_PRINT(TEXT("좌측 대각선!"));
		if ((PrevSkillType != EAiSkillType::Right) && (PrevSkillType != EAiSkillType::Left))
		{
			SetSkillWeight(LocalEntries, EAiSkillType::Left, 3.0f);
		}
	}
}


// Called every frame
void UBaseSkillRoulette::InitFromSkillBag(const FSkillBagData* SkillBagData)
{
	SkillEntries.Empty();
	AvailableSkillEntries.Empty();
	if (SkillBagData)
	{
		// SKillBagData는 TypeSkillBag 정보를 가지고 있다.
		// TypeSkillBag은 TMap<EAiSkillType, FSkillBag>
		// EAiSkillType는 Short/Long/Weaveing 등
		// FSkillBag은 TMap<int, float>. 각각 SkillID / Weight;
		for (const auto& TypePair : SkillBagData->TypeSkillBag)
		{
			const FSkillBag& Bag = TypePair.Value;
			for (const auto& SkillPair : Bag.SkillID_Weight)
			{
				int SkillID = SkillPair.Key;
				float Weight = static_cast<float>(SkillPair.Value);
				SkillEntries.Add(FSkillRouletteEntry(TypePair.Key, SkillID, Weight));
				AvailableSkillEntries.Add(FSkillRouletteEntry(TypePair.Key, SkillID, Weight));
			}
		}
	}
}

int UBaseSkillRoulette::GetRandomSkillID()
{
	TArray<FSkillRouletteEntry> localEntries = AvailableSkillEntries;

	AdjustSkillWeightsByCombatContext(localEntries);

	float totalWeight = 0.0f;
	for (const FSkillRouletteEntry& Entry : localEntries)
	{
		totalWeight += Entry.Weight;
	}

	if (totalWeight <= 0.f)
	{
		return 0;
	}

	float RandomValue = FMath::FRandRange(0.f, totalWeight);
	float AccumulatedWeight = 0.0f;
	for (const FSkillRouletteEntry& Entry : localEntries)
	{
		AccumulatedWeight += Entry.Weight;
		if (RandomValue <= AccumulatedWeight)
		{
			// LOG_PRINT(TEXT("스킬 ID: %d"),Entry.SkillID);
			PrevSkillType = Entry.SkillType;

			return Entry.SkillID;
		}
	}

	return 0;
}

void UBaseSkillRoulette::RemoveSkillEntryByID(const int skillID)
{
	AvailableSkillEntries.RemoveAll([=](const FSkillRouletteEntry& Entry)
	{
		return Entry.SkillID == skillID;
	});
}

void UBaseSkillRoulette::AddSkillEntryByID(const int skillID)
{
	const FSkillRouletteEntry* skillEntry = SkillEntries.FindByPredicate([skillID](const FSkillRouletteEntry& Entry)
	{
		return Entry.SkillID == skillID;
	});

	if (skillEntry && !AvailableSkillEntries.Contains(*skillEntry))
	{
		AvailableSkillEntries.Add(*skillEntry);
	}
}

void UBaseSkillRoulette::ApplySkillWeight(TArray<FSkillRouletteEntry>& entries, const EAiSkillType& skillType,
                                          const float ratio) const
{
	for (FSkillRouletteEntry& entry : entries)
	{
		if (entry.SkillType == skillType)
		{
			entry.Weight *= ratio;
		}
	}
}

void UBaseSkillRoulette::ApplySkillWeightByID(TArray<FSkillRouletteEntry>& entries, const int skillID,
                                              const float ratio) const
{
	for (FSkillRouletteEntry& entry : entries)
	{
		if (entry.SkillID == skillID)
		{
			entry.Weight *= ratio;
		}
	}
}

void UBaseSkillRoulette::SetSkillWeight(TArray<FSkillRouletteEntry>& entries, const EAiSkillType& skillType,
                                        const float weight) const
{
	for (FSkillRouletteEntry& entry : entries)
	{
		if (entry.SkillType == skillType)
		{
			entry.Weight = weight;
		}
	}
}

void UBaseSkillRoulette::SetSkillWeightByID(TArray<FSkillRouletteEntry>& entries, const int skillID,
                                            const float weight) const
{
	for (FSkillRouletteEntry& entry : entries)
	{
		if (entry.SkillID == skillID)
		{
			entry.Weight = weight;
		}
	}
}

void UBaseSkillRoulette::SelectSkillByType(TArray<FSkillRouletteEntry>& entries, const EAiSkillType& skillType) const
{
	for (FSkillRouletteEntry& entry : entries)
	{
		if (entry.SkillType != skillType)
		{
			entry.Weight = 0.0f;
		}
	}
}

void UBaseSkillRoulette::SelectSkillByType(TArray<FSkillRouletteEntry>& entries, const EAiSkillType& skillType,
                                           const float weight) const
{
	for (FSkillRouletteEntry& entry : entries)
	{
		if (entry.SkillType == skillType)
		{
			entry.Weight = weight;
		}
		else
		{
			entry.Weight = 1.0f;
		}
	}
}
