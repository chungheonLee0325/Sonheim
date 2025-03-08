// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Sonheim/ResourceManager/SonheimGameType.h"
#include "BaseSkillRoulette.generated.h"

USTRUCT(BlueprintType)
struct FSkillRouletteEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillRoulette")
	EAiSkillType SkillType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillRoulette")
	int SkillID;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillRoulette")
	float Weight;

	FSkillRouletteEntry()
		: SkillType(EAiSkillType::None), SkillID(0), Weight(1.0f)
	{}

	FSkillRouletteEntry(EAiSkillType InType, int InSkillID, float InWeight)
		: SkillType(InType), SkillID(InSkillID), Weight(InWeight)
	{}

	FORCEINLINE bool operator==(const FSkillRouletteEntry& Other) const
	{
		return SkillID == Other.SkillID;
	}
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SONHEIM_API UBaseSkillRoulette : public UActorComponent
{
	GENERATED_BODY()
public:
	UBaseSkillRoulette();
	
	void InitFromSkillBag(const FSkillBagData* dt_SkillBag);
	
	UFUNCTION()
	int GetRandomSkillID();

	// 쿨타임 관련 함수
	UFUNCTION()
	void RemoveSkillEntryByID(const int skillID);
	UFUNCTION()
	void AddSkillEntryByID(const int skillID);

	// 스킬 확률 조정
	/** 기존 스킬 확률에 곱해줍니다. */
	UFUNCTION()
	void ApplySkillWeight(TArray<FSkillRouletteEntry>& entries, const EAiSkillType& skillType, const float ratio) const;
	/** 기존 스킬 확률에 곱해줍니다. */
	UFUNCTION()
	void ApplySkillWeightByID(TArray<FSkillRouletteEntry>& entries, const int skillID, const float ratio) const;
	
	/** 기존 스킬 확률을 덮어 씌웁니다. */
	UFUNCTION()
	void SetSkillWeight(TArray<FSkillRouletteEntry>& entries, const EAiSkillType& skillType, const float weight) const;
	/** 기존 스킬 확률을 덮어 씌웁니다. */
	UFUNCTION()
	void SetSkillWeightByID(TArray<FSkillRouletteEntry>& entries, const int skillID, const float weight) const;

	/** 특정 타입 스킬만 활성화합니다. */
	void SelectSkillByType(TArray<FSkillRouletteEntry>& entries, const EAiSkillType& skillType) const;
	/** 특정 타입 스킬만 활성화합니다. */
	void SelectSkillByType(TArray<FSkillRouletteEntry>& entries, const EAiSkillType& skillType, const float weight) const;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	TArray<FSkillRouletteEntry> SkillEntries;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	TArray<FSkillRouletteEntry> AvailableSkillEntries;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Range")
	float ShortRange = 400.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Range")
	float MiddleRange = 600.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Range")
	float LongRange = 1000.0f;

protected:
	virtual void BeginPlay() override;

	virtual void AdjustSkillWeightsByCombatContext(TArray<FSkillRouletteEntry>& LocalEntries); 

	UPROPERTY()
	class ABaseMonster* Owner;
	
	UPROPERTY()
	mutable EAiSkillType PrevSkillType = EAiSkillType::None;
	
	int PrevSkill_ID = 0;
};
