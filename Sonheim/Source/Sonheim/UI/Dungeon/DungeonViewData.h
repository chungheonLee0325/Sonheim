#pragma once
#include "CoreMinimal.h"
#include "Sonheim/GameManager/Dungeon/DungeonStageRuntimeTypes.h"
#include "DungeonViewData.generated.h"
class UTexture2D;
USTRUCT(BlueprintType)
struct FDungeonRewardViewData
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) FText Label;
	/** The item's icon; none for the line that says the run gave nothing. */
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UTexture2D> Icon;
};
USTRUCT(BlueprintType)
struct FDungeonStageViewData
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) FText Title;
	UPROPERTY(BlueprintReadOnly) FText Objective;
	UPROPERTY(BlueprintReadOnly) FText CountText;
	UPROPERTY(BlueprintReadOnly) FText BranchText;
	/** The dungeon's name, and the step of the way through the current stage is, with how far along that is. */
	UPROPERTY(BlueprintReadOnly) FText DungeonTitle;
	UPROPERTY(BlueprintReadOnly) FText StepText;
	UPROPERTY(BlueprintReadOnly) float StepProgress = 0.f;
	/** The boss's name and health while it lives; 0 health hides the boss bar. */
	UPROPERTY(BlueprintReadOnly) FText BossName;
	UPROPERTY(BlueprintReadOnly) float BossHealth = 0.f;
	/** One line per item the run gave, only on a finished run. */
	UPROPERTY(BlueprintReadOnly) FText RewardText;
	/** The same rewards with their icons; a run that gave nothing has one line that says so. */
	UPROPERTY(BlueprintReadOnly) TArray<FDungeonRewardViewData> Rewards;
	/** Time taken and monsters defeated, only on a finished run. */
	UPROPERTY(BlueprintReadOnly) FText SummaryText;
	/** Set only on the result of a run that set the best time. */
	UPROPERTY(BlueprintReadOnly) FText NewBestText;
	/** Server time the stage runs out at, or 0 when it has no limit. The widget counts down from it, in TimeFormat ({0} is m:ss). */
	UPROPERTY(BlueprintReadOnly) double DeadlineServerTime = 0;
	UPROPERTY(BlueprintReadOnly) FText TimeFormat;
	UPROPERTY(BlueprintReadOnly) float Progress = 0.f;
	UPROPERTY(BlueprintReadOnly) EDungeonRunStatus Status = EDungeonRunStatus::Idle;
	/** The local player takes part in the run. The run's screens stay hidden from players who do not. */
	UPROPERTY(BlueprintReadOnly) bool bParticipant = true;
	UPROPERTY(BlueprintReadOnly) int32 Revision = 0;
};
