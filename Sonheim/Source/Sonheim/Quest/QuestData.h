#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"

#include "Sonheim/Utilities/ItemChangeReason.h"
#include "Sonheim/Rewards/RewardTypes.h"

#include "QuestData.generated.h"

UENUM(BlueprintType)
enum class EQuestState : uint8
{
	None,
	Active,
	Completed,
	Failed,
};

UENUM(BlueprintType)
enum class EQuestObjectiveType : uint8
{
	Kill,
	PossessItem,
	AcquireItem,
	CraftItem,
	TurnInItem,
};

USTRUCT(BlueprintType)
struct FQuestRewardRow : public FTableRowBase
{
	GENERATED_BODY()

	// E02 동결 계약: RewardID만 고정한다.
	// 추가 필드는 임시이며 E03+에서 변경될 수 있다.

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 RewardID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FRewardDef Reward;
};

USTRUCT(BlueprintType)
struct FQuestObjectiveDef
{
	GENERATED_BODY()

	// Stable key for progress mapping and UI.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName ObjectiveKey = NAME_None;

	// Step gating (0..N). Only objectives of the current step are active.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 StepIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EQuestObjectiveType Type = EQuestObjectiveType::Kill;

	// For Kill (0 = any)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 TargetAreaObjectID = 0;

	// For item objectives
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 ItemID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 RequiredCount = 1;

	// For AcquireItem/CraftItem. Bitmask of EItemChangeReason.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(Bitmask, BitmaskEnum="EItemChangeReason"))
	int32 ReasonMask = 0;

	// Optional UI override
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText TitleOverride;
};

USTRUCT(BlueprintType)
struct FQuestData : public FTableRowBase
{
	GENERATED_BODY()

	// E02 동결 계약:
	// QuestID, Title, Objectives, RewardTableID
	// 추가 필드는 임시이며 E03+에서 변경될 수 있다.

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 QuestID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 StartNpcID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 EndNpcID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bAutoAccept = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bAutoComplete = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<int32> PrereqQuestIDs;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FQuestObjectiveDef> Objectives;

	// Optional reward table reference (0 = use inline rewards)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 RewardTableID = 0;

	// Inline rewards (used when RewardTableID is 0 or missing)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FRewardDef Rewards;
};
