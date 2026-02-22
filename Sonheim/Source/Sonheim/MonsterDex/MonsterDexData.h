#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "UObject/SoftObjectPtr.h"
#include "Sonheim/Rewards/RewardTypes.h"

#include "MonsterDexData.generated.h"

USTRUCT(BlueprintType)
struct FMonsterDexRewardRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 RewardID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FRewardDef Reward;
};

USTRUCT(BlueprintType)
struct FMonsterDexRewardTier
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 RequiredKillCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 RequiredCaptureCount = 0;

	// Optional reward table reference (0 = use inline reward)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 RewardTableID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FRewardDef Reward;

	// Auto-claim on reaching the requirement (server)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bAutoClaim = false;
};

USTRUCT(BlueprintType)
struct FMonsterDexData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 MonsterID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName VariantId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bHiddenUntilDiscovered = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FMonsterDexRewardTier> RewardTiers;
};
