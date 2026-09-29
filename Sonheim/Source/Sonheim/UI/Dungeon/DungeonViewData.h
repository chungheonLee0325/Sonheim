#pragma once
#include "CoreMinimal.h"
#include "Sonheim/GameManager/Dungeon/DungeonStageRuntimeTypes.h"
#include "DungeonViewData.generated.h"
class UTexture2D;
/** Where an objective line stands: the run's last goal, the stage's goal, or a goal the run may skip. */
UENUM(BlueprintType)
enum class EDungeonObjectiveKind : uint8 { Final, Main, Optional };
/** An optional line that its stage left behind is Done or Missed for a few seconds before it leaves. */
UENUM(BlueprintType)
enum class EDungeonObjectiveState : uint8 { Open, Done, Missed };
USTRUCT(BlueprintType)
struct FDungeonObjectiveViewData
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) FText Label;
	/** The kind's tag, such as 추가; empty shows none. */
	UPROPERTY(BlueprintReadOnly) FText KindLabel;
	UPROPERTY(BlueprintReadOnly) EDungeonObjectiveKind Kind = EDungeonObjectiveKind::Main;
	/** Done out of total, or empty while the line has no count. A line its stage left behind says 완료 or 놓침 here. */
	UPROPERTY(BlueprintReadOnly) FText Count;
	UPROPERTY(BlueprintReadOnly) EDungeonObjectiveState State = EDungeonObjectiveState::Open;
	/** How long an optional line stays open, and what taking it means or gives. */
	UPROPERTY(BlueprintReadOnly) FText Window;
	UPROPERTY(BlueprintReadOnly) FText Note;
	/** The goal's icon, drawn at the head of the line while it is open. */
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UTexture2D> Icon;
};
/** One step of the way through: done, where the run is, or still ahead. */
UENUM(BlueprintType)
enum class EDungeonStepState : uint8 { Done, Current, Upcoming };
USTRUCT(BlueprintType)
struct FDungeonStepViewData
{
	GENERATED_BODY()
	/** The step's stage, or its stages side by side while the run has not chosen between them. */
	UPROPERTY(BlueprintReadOnly) FText Label;
	UPROPERTY(BlueprintReadOnly) EDungeonStepState State = EDungeonStepState::Upcoming;
	/** The first step has no arrow in front of it. */
	UPROPERTY(BlueprintReadOnly) bool bFirst = false;
	/** The room's icon before its name. */
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UTexture2D> Icon;
};
USTRUCT(BlueprintType)
struct FDungeonMemberViewData
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) FText Name;
	/** Health from 1 to 0; a member at 0 is down. */
	UPROPERTY(BlueprintReadOnly) float Health = 1.f;
	/** The player who started the run: the run fails when this player goes down or leaves. */
	UPROPERTY(BlueprintReadOnly) bool bOwner = false;
};
USTRUCT(BlueprintType)
struct FDungeonStatViewData
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) FText Label;
	UPROPERTY(BlueprintReadOnly) FText Value;
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UTexture2D> Icon;
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
/** A place an open objective line leads to, marked on the screen with the line's icon and the distance. */
USTRUCT(BlueprintType)
struct FDungeonMarkerViewData
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) FVector Location = FVector::ZeroVector;
	/** The marker leaves while the local player stands in this box: the room it leads to, or the space around a switch. */
	UPROPERTY(BlueprintReadOnly) FBox Arrival = FBox(ForceInit);
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UTexture2D> Icon;
	UPROPERTY(BlueprintReadOnly) EDungeonObjectiveKind Kind = EDungeonObjectiveKind::Main;
};
USTRUCT(BlueprintType)
struct FDungeonStageViewData
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) FText Title;
	UPROPERTY(BlueprintReadOnly) FText Objective;
	UPROPERTY(BlueprintReadOnly) FText CountText;
	UPROPERTY(BlueprintReadOnly) FText BranchText;
	/** The dungeon's name, its goal, and the way through it, with the step of the way the current stage is. */
	UPROPERTY(BlueprintReadOnly) FText DungeonTitle;
	UPROPERTY(BlueprintReadOnly) FText Goal;
	UPROPERTY(BlueprintReadOnly) TArray<FDungeonStepViewData> Steps;
	UPROPERTY(BlueprintReadOnly) FText StepText;
	UPROPERTY(BlueprintReadOnly) float StepProgress = 0.f;
	/** The stage's goals, and apart from them the optional ones, with how many of the stage's goals are done. */
	UPROPERTY(BlueprintReadOnly) TArray<FDungeonObjectiveViewData> Objectives;
	UPROPERTY(BlueprintReadOnly) TArray<FDungeonObjectiveViewData> OptionalObjectives;
	UPROPERTY(BlueprintReadOnly) FText ObjectivesDone;
	/** Where the open lines lead, while the run goes, with the distance in MarkerDistanceFormat ({0} is meters). */
	UPROPERTY(BlueprintReadOnly) TArray<FDungeonMarkerViewData> Markers;
	UPROPERTY(BlueprintReadOnly) FText MarkerDistanceFormat;
	/** The players taking part, while the run goes. */
	UPROPERTY(BlueprintReadOnly) TArray<FDungeonMemberViewData> Members;
	/** The boss's name and health while it lives; 0 health hides the boss bar. */
	UPROPERTY(BlueprintReadOnly) FText BossName;
	UPROPERTY(BlueprintReadOnly) float BossHealth = 0.f;
	/** What the boss does by name, and the server times it runs between, which the widget fills a bar from. */
	UPROPERTY(BlueprintReadOnly) FText BossActionText;
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UTexture2D> BossActionIcon;
	UPROPERTY(BlueprintReadOnly) double BossActionStartServerTime = 0;
	UPROPERTY(BlueprintReadOnly) double BossActionEndServerTime = 0;
	/** The boss's phase from phase 2 on, such as 2단계; empty before. */
	UPROPERTY(BlueprintReadOnly) FText BossPhaseText;
	/** Said while the boss can be captured; empty otherwise. */
	UPROPERTY(BlueprintReadOnly) FText BossHintText;
	/** Damage toward the boss's next knockdown, 0 to 1. */
	UPROPERTY(BlueprintReadOnly) float BossBreak = 0.f;
	/** Over the result's title: the emblem of a won or of a failed run, none while the run goes. */
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UTexture2D> OutcomeEmblem;
	/** One line per item the run gave, only on a finished run. */
	UPROPERTY(BlueprintReadOnly) FText RewardText;
	/** The same rewards with their icons; a run that gave nothing has one line that says so. */
	UPROPERTY(BlueprintReadOnly) TArray<FDungeonRewardViewData> Rewards;
	/** Time taken, monsters defeated, captures and the route, only on a finished run. */
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
