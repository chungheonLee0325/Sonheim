#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Sonheim/GameManager/Dungeon/DungeonStageRuntimeTypes.h"
#include "Sonheim/UI/Dungeon/DungeonViewData.h"
#include "Sonheim/UI/Notice/NoticeWidget.h"
#include "Sonheim/Utilities/StringTableIds.h"
#include "DungeonPresentationDataAsset.generated.h"
/** What completes an objective line, read from the run's snapshot. */
UENUM(BlueprintType)
enum class EDungeonObjectiveGoal : uint8
{
	/** A plain line with no count; it stays open while its stage lasts. */
	None,
	/** Every monster of GroupId defeated or captured. The count shows once the group has appeared. */
	Group,
	/** The run has RunTag, such as the unlocked shortcut. */
	RunTag,
	/** The run is won. */
	Clear,
	/** Target monsters of GroupId captured. */
	Captured
};
USTRUCT(BlueprintType)
struct FDungeonObjectiveLine
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FText Label;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) EDungeonObjectiveKind Kind = EDungeonObjectiveKind::Main;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) EDungeonObjectiveGoal Goal = EDungeonObjectiveGoal::None;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(EditCondition="Goal == EDungeonObjectiveGoal::Group", EditConditionHides, Categories="Dungeon")) FGameplayTag GroupId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(EditCondition="Goal == EDungeonObjectiveGoal::RunTag", EditConditionHides)) FGameplayTag RunTag;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(EditCondition="Goal == EDungeonObjectiveGoal::Captured", EditConditionHides, ClampMin="1")) int32 Target = 1;
	/** How long an optional line stays open, such as 경비실 전투 중, and what taking it means or gives. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FText Window;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FText Note;
	/** Where the line leads: the SourceId of a placed room zone or switch. While the line is open, the screen marks the place. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(Categories="Dungeon")) FGameplayTag MarkerTarget;
};
USTRUCT(BlueprintType)
struct FDungeonStagePresentation
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(Categories="Dungeon")) FGameplayTag StageId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FText Title;
	/** One sentence for screens without an objective list, and the line under the result's title. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FText Objective;
	/** Where the stage stands on the way through, shown under the dungeon's name with StepFormat. 0 leaves the step out. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0")) int32 Step = 0;
	/** The objective list's lines while the run is in this stage, after the run's own lines. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FDungeonObjectiveLine> Objectives;
	/** Before the stage's name on the way through. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UTexture2D> Icon;
	/** The stage's room on the map, in world X and Y: lit while the run is in the stage, with Icon in its middle. Empty leaves it off. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FBox2D MapArea = FBox2D(ForceInit);
};
/** Everything the dungeon's screens say. The widgets only lay it out, so the words change here without a code build. */
UCLASS(BlueprintType)
class SONHEIM_API UDungeonPresentationDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FText DungeonTitle;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") TArray<FDungeonStagePresentation> Stages;
	/** The route each branch takes, shown on the result. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") TMap<FGameplayTag, FText> BranchLabels;
	/** Steps on the way through, which each stage's Step counts against. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon", meta=(ClampMin="1")) int32 StepCount = 4;
	/** Name over the boss's health bar while it lives. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FText BossName = LOCTABLE(SONHEIM_ST_DUNGEON, "Boss.Name");
	/** What the boss does, named over its health bar: its patterns, and Boss.State.* while it sleeps, roars, rests or lies down. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon", meta=(Categories="Boss")) TMap<FGameplayTag, FText> BossActionLabels;
	/** Next to the boss's name from phase 2 on; {0} is the phase. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FText BossPhaseFormat = LOCTABLE(SONHEIM_ST_DUNGEON, "Boss.PhaseFormat");
	/** Under the boss's health bar while it can be captured. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FText BossCaptureHint = LOCTABLE(SONHEIM_ST_DUNGEON, "Boss.CaptureHint");

	/** The run's goal, under the dungeon's name: its final lines' labels. A final line has no count on screen. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Objectives") TArray<FDungeonObjectiveLine> RunObjectives;
	/** The tag in front of each kind of line; empty shows none. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Objectives") TMap<EDungeonObjectiveKind, FText> KindLabels = {
		{EDungeonObjectiveKind::Final, LOCTABLE(SONHEIM_ST_DUNGEON, "Objective.Kind.Final")},
		{EDungeonObjectiveKind::Main, FText::GetEmpty()},
		{EDungeonObjectiveKind::Optional, LOCTABLE(SONHEIM_ST_DUNGEON, "Objective.Kind.Optional")}};
	/** {0} lines done, {1} lines. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Objectives") FText ObjectivesDoneFormat = LOCTABLE(SONHEIM_ST_DUNGEON, "Objective.DoneFormat");
	/** When its stage ends, an optional line shows whether it was taken for this long, then leaves. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Objectives") FText OptionalDoneText = LOCTABLE(SONHEIM_ST_DUNGEON, "Objective.OptionalDone");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Objectives") FText OptionalMissedText = LOCTABLE(SONHEIM_ST_DUNGEON, "Objective.OptionalMissed");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Objectives", meta=(ClampMin="0")) float OptionalResultSeconds = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Texts") FText LoadingText = LOCTABLE(SONHEIM_ST_DUNGEON, "Run.Loading");
	/** {0} the stage's Step, {1} StepCount. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Texts") FText StepFormat = LOCTABLE(SONHEIM_ST_DUNGEON, "Run.StepFormat");
	/** {0} done, {1} to do: the objective lines' counts and the stage's count. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Texts") FText CountFormat = INVTEXT("{0} / {1}");
	/** {0} the time left as m:ss. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Texts") FText TimeFormat = LOCTABLE(SONHEIM_ST_DUNGEON, "Run.TimeLeftFormat");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Texts") FText FailedTitle = LOCTABLE(SONHEIM_ST_DUNGEON, "Result.FailedTitle");
	/** The line under FailedTitle. None is a failure stage the definition reached by its own rules. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Texts") TMap<EDungeonFailReason, FText> FailReasons = {
		{EDungeonFailReason::None, LOCTABLE(SONHEIM_ST_DUNGEON, "Result.FailReason.None")},
		{EDungeonFailReason::TimeOut, LOCTABLE(SONHEIM_ST_DUNGEON, "Result.FailReason.TimeOut")},
		{EDungeonFailReason::OwnerDown, LOCTABLE(SONHEIM_ST_DUNGEON, "Result.FailReason.OwnerDown")},
		{EDungeonFailReason::OwnerLeft, LOCTABLE(SONHEIM_ST_DUNGEON, "Result.FailReason.OwnerLeft")},
		{EDungeonFailReason::TargetLost, LOCTABLE(SONHEIM_ST_DUNGEON, "Result.FailReason.TargetLost")},
		{EDungeonFailReason::Error, LOCTABLE(SONHEIM_ST_DUNGEON, "Result.FailReason.Error")}};
	/** The result's tiles: the time the run took (m:ss), monsters defeated, and the route when a branch was taken. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Texts") FText TimeStatLabel = LOCTABLE(SONHEIM_ST_DUNGEON, "Result.Stat.Time");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Texts") FText KillStatLabel = LOCTABLE(SONHEIM_ST_DUNGEON, "Result.Stat.Kill");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Texts") FText RouteStatLabel = LOCTABLE(SONHEIM_ST_DUNGEON, "Result.Stat.Route");
	/** A tile only when the run captured any monster. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Texts") FText CaptureStatLabel = LOCTABLE(SONHEIM_ST_DUNGEON, "Result.Stat.Capture");
	/** {0} the item's name, {1} how many: a reward line on screens without reward slots. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Texts") FText RewardFormat = INVTEXT("{0} ×{1}");
	/** {0} how many, under a reward slot's name. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Texts") FText RewardCountFormat = INVTEXT("×{0}");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Texts") FText NoRewardText = LOCTABLE(SONHEIM_ST_DUNGEON, "Result.NoReward");
	/** Under the result once the dungeon has been cleared: {0} clears, {1} the best time. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Texts") FText RecordFormat = LOCTABLE(SONHEIM_ST_DUNGEON, "Result.RecordFormat");
	/** A time of a minute or more, {0} minutes and {1} seconds; SecondsFormat below that. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Texts") FText MinutesFormat = LOCTABLE(SONHEIM_ST_DUNGEON, "Time.MinutesFormat");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Texts") FText SecondsFormat = LOCTABLE(SONHEIM_ST_DUNGEON, "Time.SecondsFormat");
	/** On the result of a run that set the best time. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Texts") FText NewBestText = LOCTABLE(SONHEIM_ST_DUNGEON, "Result.NewBest");

	/** At the head of an objective line while it is open, by its goal; the line's widget draws its own marks once it is done or missed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Icons") TMap<EDungeonObjectiveGoal, TSoftObjectPtr<UTexture2D>> GoalIcons;
	/** A step of several stages while the run has not chosen between them; a chosen one shows its stage's icon. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Icons") TSoftObjectPtr<UTexture2D> BranchStepIcon;
	/** Before what the boss does, by the same tags as BossActionLabels. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Icons", meta=(Categories="Boss")) TMap<FGameplayTag, TSoftObjectPtr<UTexture2D>> BossActionIcons;
	/** Before the result's figures. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Icons") TSoftObjectPtr<UTexture2D> TimeStatIcon;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Icons") TSoftObjectPtr<UTexture2D> KillStatIcon;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Icons") TSoftObjectPtr<UTexture2D> CaptureStatIcon;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Icons") TSoftObjectPtr<UTexture2D> RouteStatIcon;
	/** Over the result's title of a won and of a failed run. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Icons") TSoftObjectPtr<UTexture2D> SucceededEmblem;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Icons") TSoftObjectPtr<UTexture2D> FailedEmblem;

	/** How high over the floor of the place a line leads to its marker floats. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Markers", meta=(ClampMin="0")) float MarkerHeight = 180.f;
	/** A room's marker leaves once the player is inside the room; a switch's once the player is this close. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Markers", meta=(ClampMin="0")) float MarkerArriveDistance = 300.f;
	/** Under the marker; {0} is the distance in meters. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Markers") FText MarkerDistanceFormat = INVTEXT("{0} m");

	/** The dungeon's floor plan, white on clear, seen from above with X to the right and Y down, and the world rectangle it covers. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Map") TSoftObjectPtr<UTexture2D> MapTexture;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Map") FBox2D MapBounds = FBox2D(ForceInit);

	/** The dungeon card's pace line: the run's time, then the best time from before the run when there is one ({0} is m:ss). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Record") FText ElapsedFormat = LOCTABLE(SONHEIM_ST_DUNGEON, "Pace.ElapsedFormat");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Record") FText RunBestFormat = LOCTABLE(SONHEIM_ST_DUNGEON, "Pace.BestFormat");
	/** The result's grade tile, and each grade's text; the grades and their rules are the definition's, a grade without text shows its name. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Record") FText GradeStatLabel = LOCTABLE(SONHEIM_ST_DUNGEON, "Result.Stat.Grade");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Record") TSoftObjectPtr<UTexture2D> GradeStatIcon;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Record") TMap<FName, FText> GradeTexts;
	/** Under the record on the result: the finished time against the best from before the run ({0} is m:ss), or a first clear's time. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Record") FText FasterThanBestFormat = LOCTABLE(SONHEIM_ST_DUNGEON, "Result.FasterThanBestFormat");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Record") FText SlowerThanBestFormat = LOCTABLE(SONHEIM_ST_DUNGEON, "Result.SlowerThanBestFormat");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Record") FText SameAsBestText = LOCTABLE(SONHEIM_ST_DUNGEON, "Result.SameAsBest");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Record") FText FirstRecordFormat = LOCTABLE(SONHEIM_ST_DUNGEON, "Result.FirstRecordFormat");

	/** Banner when the run gains a tag its rules set, such as the unlocked shortcut. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Toasts") TMap<FGameplayTag, FNoticeData> TagToasts;
	/** Banner when the run takes a branch. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Toasts") TMap<FGameplayTag, FNoticeData> BranchToasts;
	/** Banner when a group of monsters appears, by group; {0} in Detail is how many. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Toasts") TMap<FGameplayTag, FNoticeData> GroupToasts;
	/** Banner when every monster of a group is defeated or captured, by group. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Toasts") TMap<FGameplayTag, FNoticeData> GroupClearToasts;
};
