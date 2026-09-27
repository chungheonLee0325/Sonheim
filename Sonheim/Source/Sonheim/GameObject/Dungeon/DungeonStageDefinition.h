#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "DungeonStageDefinition.generated.h"

class UDungeonSpawnRuleDataAsset;

/** Every name a definition gives or uses is a gameplay tag under its dungeon's tag, such as Dungeon.ForgottenRuins.Stage.Combat:
 * stages, groups, branches, point sets, the zones and switches events come from, and the barriers. Each dungeon lists its tags in
 * Config/Tags, so an editor field offers them in a list and the validation refuses a tag of another dungeon.
 *
 * AreaEntered: a player of the run stepped into the trigger zone whose SourceId the rule names.
 * MonsterCaptured: a monster of the group the rule names became a player's partner. A captured monster counts toward its group's
 * completion like a defeated one, so a group can also be completed by capture (WaveCompleted, BossDefeated). */
UENUM(BlueprintType)
enum class EDungeonStageEvent : uint8 { StageEntered, WaveCompleted, BossDefeated, ActorInteracted, StageTimeout, AreaEntered, MonsterCaptured };
UENUM(BlueprintType)
enum class EDungeonStageCondition : uint8 { Always, HasRunTag, SpawnGroupCompleted };
UENUM(BlueprintType)
enum class EDungeonStageAction : uint8 { SpawnGroup, SetRunTag, ClearRunTag, EmitEvent, GrantReward };
UENUM(BlueprintType)
enum class EDungeonTerminalOutcome : uint8 { None, Success, Failure };

USTRUCT(BlueprintType)
struct FDungeonStageCondition
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) EDungeonStageCondition Type = EDungeonStageCondition::Always;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(EditCondition="Type == EDungeonStageCondition::HasRunTag", EditConditionHides, Categories="Dungeon")) FGameplayTag RunTag;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(EditCondition="Type == EDungeonStageCondition::SpawnGroupCompleted", EditConditionHides, Categories="Dungeon")) FGameplayTag GroupId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bNegate = false;
};

USTRUCT(BlueprintType)
struct FDungeonStageAction
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) EDungeonStageAction Type = EDungeonStageAction::SpawnGroup;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(EditCondition="Type == EDungeonStageAction::SpawnGroup", EditConditionHides, Categories="Dungeon")) FGameplayTag GroupId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(EditCondition="Type == EDungeonStageAction::SpawnGroup", EditConditionHides, Categories="Dungeon")) FGameplayTag PointSetId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(EditCondition="Type == EDungeonStageAction::SpawnGroup", EditConditionHides)) TSoftObjectPtr<UDungeonSpawnRuleDataAsset> SpawnRule;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(EditCondition="Type == EDungeonStageAction::SpawnGroup", EditConditionHides)) bool bBossGroup = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(EditCondition="Type == EDungeonStageAction::SetRunTag || Type == EDungeonStageAction::ClearRunTag", EditConditionHides, Categories="Dungeon")) FGameplayTag RunTag;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(EditCondition="Type == EDungeonStageAction::EmitEvent", EditConditionHides)) EDungeonStageEvent Event = EDungeonStageEvent::StageEntered;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(EditCondition="Type == EDungeonStageAction::EmitEvent", EditConditionHides, Categories="Dungeon")) FGameplayTag EventSourceId;
	// GrantReward only: the item row id and how many the run gives to each player taking part. The validation refuses anything else.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(EditCondition="Type == EDungeonStageAction::GrantReward", EditConditionHides, ClampMin="1")) int32 RewardItemId = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(EditCondition="Type == EDungeonStageAction::GrantReward", EditConditionHides, ClampMin="1", ClampMax="99")) int32 RewardCount = 0;
};

USTRUCT(BlueprintType)
struct FDungeonStageTransition
{
	GENERATED_BODY()
	/** Names the transition in the logs and the graph; nothing else refers to it, so it stays a plain name. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName TransitionId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(TitleProperty="Type")) TArray<FDungeonStageCondition> Conditions;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(Categories="Dungeon")) FGameplayTag NextStageId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(Categories="Dungeon")) FGameplayTag BranchId;
};

USTRUCT(BlueprintType)
struct FDungeonStageEventRule
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) EDungeonStageEvent Event = EDungeonStageEvent::StageEntered;
	/** Exact source match: the zone, switch or group the event comes from. Empty for the events a stage raises itself. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(Categories="Dungeon")) FGameplayTag SourceId;
	/** The rule answers only the first time its event comes in a run, such as a bonus for the first capture. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bOnce = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(TitleProperty="Type")) TArray<FDungeonStageAction> Actions;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(TitleProperty="TransitionId")) TArray<FDungeonStageTransition> Transitions;
};

USTRUCT(BlueprintType)
struct FDungeonStageDefinition
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(Categories="Dungeon")) FGameplayTag StageId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) EDungeonTerminalOutcome TerminalOutcome = EDungeonTerminalOutcome::None;
	/** Seconds the stage may last before it raises StageTimeout. 0 is no limit, and a limit needs a rule that answers it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0")) float TimeLimitSeconds = 0.f;
	/** The doorway barriers, by their BarrierId, that stand while the run is in this stage: the way on waits for the stage to end. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(Categories="Dungeon")) TArray<FGameplayTag> SealedBarriers;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(TitleProperty="Event")) TArray<FDungeonStageEventRule> EventRules;
};
