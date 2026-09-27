#pragma once
#include "CoreMinimal.h"
#include "Sonheim/GameManager/Dungeon/DungeonStageRuntimeTypes.h"
#include "DungeonViewData.generated.h"
class UTexture2D;
/** Where an objective line stands: the run's last goal, the stage's goal, or a goal the run may skip. */
UENUM(BlueprintType)
enum class EDungeonObjectiveKind : uint8 { Final, Main, Optional };
USTRUCT(BlueprintType)
struct FDungeonObjectiveViewData
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) FText Label;
	/** The kind's tag, such as 최종 or 추가; empty shows none. */
	UPROPERTY(BlueprintReadOnly) FText KindLabel;
	UPROPERTY(BlueprintReadOnly) EDungeonObjectiveKind Kind = EDungeonObjectiveKind::Main;
	/** Done out of total, or empty while the line has no count. */
	UPROPERTY(BlueprintReadOnly) FText Count;
	UPROPERTY(BlueprintReadOnly) bool bDone = false;
};
USTRUCT(BlueprintType)
struct FDungeonStatViewData
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) FText Label;
	UPROPERTY(BlueprintReadOnly) FText Value;
};
USTRUCT(BlueprintType)
struct FDungeonRewardViewData
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) FText Name;
	/** How many, such as ×3; empty on the line that says the run gave nothing. */
	UPROPERTY(BlueprintReadOnly) FText Count;
	/** The item's icon; none on the line that says the run gave nothing. */
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
	/** The run's goals while it goes: the final one, the stage's, and the optional ones, with how many are done. */
	UPROPERTY(BlueprintReadOnly) TArray<FDungeonObjectiveViewData> Objectives;
	UPROPERTY(BlueprintReadOnly) FText ObjectivesDone;
	/** The boss's name and health while it lives; 0 health hides the boss bar. */
	UPROPERTY(BlueprintReadOnly) FText BossName;
	UPROPERTY(BlueprintReadOnly) float BossHealth = 0.f;
	/** One line per item the run gave, only on a finished run. */
	UPROPERTY(BlueprintReadOnly) FText RewardText;
	/** The same rewards with their icons; a run that gave nothing has one line that says so. */
	UPROPERTY(BlueprintReadOnly) TArray<FDungeonRewardViewData> Rewards;
	/** Time taken, monsters defeated and the route, only on a finished run. */
	UPROPERTY(BlueprintReadOnly) TArray<FDungeonStatViewData> Stats;
	/** How often the dungeon was cleared and the best time, only on a finished run of a cleared dungeon. */
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
