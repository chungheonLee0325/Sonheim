#pragma once
#include "CoreMinimal.h"
#include "Sonheim/GameManager/Dungeon/DungeonStageRuntimeTypes.h"
#include "DungeonViewData.generated.h"
USTRUCT(BlueprintType)
struct FDungeonStageViewData
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) FText Title;
	UPROPERTY(BlueprintReadOnly) FText Objective;
	UPROPERTY(BlueprintReadOnly) FText CountText;
	UPROPERTY(BlueprintReadOnly) FText BranchText;
	/** One line per item the run gave, only on a finished run. */
	UPROPERTY(BlueprintReadOnly) FText RewardText;
	/** Time taken and monsters defeated, only on a finished run. */
	UPROPERTY(BlueprintReadOnly) FText SummaryText;
	/** Server time the stage runs out at, or 0 when it has no limit. The widget counts down from it. */
	UPROPERTY(BlueprintReadOnly) double DeadlineServerTime = 0;
	UPROPERTY(BlueprintReadOnly) float Progress = 0.f;
	UPROPERTY(BlueprintReadOnly) EDungeonRunStatus Status = EDungeonRunStatus::Idle;
	/** The local player takes part in the run. The run's screens stay hidden from players who do not. */
	UPROPERTY(BlueprintReadOnly) bool bParticipant = true;
	UPROPERTY(BlueprintReadOnly) int32 Revision = 0;
};
