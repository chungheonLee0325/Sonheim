#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"

#include "RewardTypes.generated.h"

USTRUCT(BlueprintType)
struct FRewardItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 ItemID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Count = 1;
};

USTRUCT(BlueprintType)
struct FRewardDef
{
	GENERATED_BODY()

	// EXP reward (0 = none)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Exp = 0;

	// Item rewards (including money-as-item)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FRewardItem> Items;
};

USTRUCT(BlueprintType)
struct FRewardRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 RewardID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FRewardDef Reward;
};

USTRUCT(BlueprintType)
struct FDropRewardEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 RewardID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1"))
	int32 Weight = 100;
};

USTRUCT(BlueprintType)
struct FDropTableRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 DropTableID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1"))
	int32 RollMin = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1"))
	int32 RollMax = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FDropRewardEntry> Entries;
};
