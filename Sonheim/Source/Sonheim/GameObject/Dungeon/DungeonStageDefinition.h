#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "DungeonStageDefinition.generated.h"

class UDungeonSpawnRuleDataAsset;

UENUM(BlueprintType)
enum class EDungeonStageEvent : uint8 { StageEntered, WaveCompleted, BossDefeated, ActorInteracted };
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
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FGameplayTag RunTag;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName GroupId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bNegate = false;
};

USTRUCT(BlueprintType)
struct FDungeonStageAction
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) EDungeonStageAction Type = EDungeonStageAction::SpawnGroup;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName GroupId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName PointSetId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UDungeonSpawnRuleDataAsset> SpawnRule;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bBossGroup = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FGameplayTag RunTag;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) EDungeonStageEvent Event = EDungeonStageEvent::StageEntered;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName EventSourceId;
	// GrantReward only: the item row id and how many the run gives to the player who started it.
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 RewardItemId = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 RewardCount = 0;
};

USTRUCT(BlueprintType)
struct FDungeonStageTransition
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName TransitionId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FDungeonStageCondition> Conditions;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName NextStageId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName BranchId;
};

USTRUCT(BlueprintType)
struct FDungeonStageEventRule
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) EDungeonStageEvent Event = EDungeonStageEvent::StageEntered;
	// Exact source match. None is used for the internally generated StageEntered event.
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName SourceId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FDungeonStageAction> Actions;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FDungeonStageTransition> Transitions;
};

USTRUCT(BlueprintType)
struct FDungeonStageDefinition
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName StageId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) EDungeonTerminalOutcome TerminalOutcome = EDungeonTerminalOutcome::None;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FDungeonStageEventRule> EventRules;
};
