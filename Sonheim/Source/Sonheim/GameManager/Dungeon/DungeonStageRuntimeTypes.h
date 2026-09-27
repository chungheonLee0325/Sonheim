#pragma once
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/PrimaryAssetId.h"
#include "DungeonStageRuntimeTypes.generated.h"
class APlayerState;
UENUM(BlueprintType)
enum class EDungeonRunStatus : uint8 { Idle, Loading, Running, Succeeded, Failed };
/** Why a run failed, so the result screen can say it. None is a failure stage the definition reached by its own rules. */
UENUM(BlueprintType)
enum class EDungeonFailReason : uint8 { None, TimeOut, OwnerDown, OwnerLeft, TargetLost, Error };

USTRUCT(BlueprintType)
struct FDungeonRunReward
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) int32 ItemId = 0;
	UPROPERTY(BlueprintReadOnly) int32 Count = 0;
	bool operator==(const FDungeonRunReward& Other) const { return ItemId == Other.ItemId && Count == Other.Count; }
};

/** What became of one spawned group: how many appeared, and how many of them were defeated or captured. */
USTRUCT(BlueprintType)
struct FDungeonGroupTally
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) FName GroupId;
	UPROPERTY(BlueprintReadOnly) int32 Spawned = 0;
	UPROPERTY(BlueprintReadOnly) int32 Defeated = 0;
	UPROPERTY(BlueprintReadOnly) int32 Captured = 0;
	bool operator==(const FDungeonGroupTally& Other) const
	{
		return GroupId == Other.GroupId && Spawned == Other.Spawned && Defeated == Other.Defeated && Captured == Other.Captured;
	}
};

USTRUCT(BlueprintType)
struct FDungeonStageRuntimeState
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) FGuid RunId;
	UPROPERTY(BlueprintReadOnly) FPrimaryAssetId DefinitionAssetId;
	UPROPERTY(BlueprintReadOnly) FName StageId;
	UPROPERTY(BlueprintReadOnly) int32 Revision = 0;
	UPROPERTY(BlueprintReadOnly) EDungeonRunStatus RunStatus = EDungeonRunStatus::Idle;
	UPROPERTY(BlueprintReadOnly) FName ObjectiveGroupId;
	UPROPERTY(BlueprintReadOnly) int32 CurrentCount = 0;
	UPROPERTY(BlueprintReadOnly) int32 RequiredCount = 0;
	UPROPERTY(BlueprintReadOnly) FName SelectedBranchId;
	/** Run flags the stage rules have set, such as the unlocked shortcut. The world reads them; conditions are still evaluated on the server. */
	UPROPERTY(BlueprintReadOnly) FGameplayTagContainer RunTags;
	UPROPERTY(BlueprintReadOnly) double StageStartedServerTime = 0;
	/** Server time the current stage runs out at. 0 while the stage has no limit. */
	UPROPERTY(BlueprintReadOnly) double StageDeadlineServerTime = 0;
	/** What the run handed over, merged per item in the order the rules granted it. The result screen settles from this. */
	UPROPERTY(BlueprintReadOnly) TArray<FDungeonRunReward> Rewards;
	/** Monsters the run confirmed as defeated, and those captured, across every group. */
	UPROPERTY(BlueprintReadOnly) int32 DefeatedCount = 0;
	UPROPERTY(BlueprintReadOnly) int32 CapturedCount = 0;
	/** Every group the run spawned, in spawn order. The objective list counts from these. */
	UPROPERTY(BlueprintReadOnly) TArray<FDungeonGroupTally> Groups;
	/** Seconds from the start to the terminal stage. It stays 0 while the run is going. */
	UPROPERTY(BlueprintReadOnly) float ElapsedSeconds = 0.f;
	/** How often this dungeon has been finished, and the fastest of those runs. Filled from the saved record when a run ends. */
	UPROPERTY(BlueprintReadOnly) int32 ClearCount = 0;
	UPROPERTY(BlueprintReadOnly) float BestSeconds = 0.f;
	/** Players taking part: the run's screens show only to them, and every reward goes to each of them. */
	UPROPERTY(BlueprintReadOnly) TArray<TObjectPtr<APlayerState>> Participants;
	/** The player who started the run. Prompts tell everyone else that only this player can pull the lever. */
	UPROPERTY(BlueprintReadOnly) TObjectPtr<APlayerState> OwnerPlayer;
	UPROPERTY(BlueprintReadOnly) EDungeonFailReason FailReason = EDungeonFailReason::None;
	/** Health of the boss group's monster, 1 to 0, while it is alive; 0 before it appears. */
	UPROPERTY(BlueprintReadOnly) float BossHealth = 0.f;
	bool SamePresentationState(const FDungeonStageRuntimeState& Other) const
	{
		return RunId == Other.RunId && DefinitionAssetId == Other.DefinitionAssetId && StageId == Other.StageId &&
			RunStatus == Other.RunStatus && ObjectiveGroupId == Other.ObjectiveGroupId && CurrentCount == Other.CurrentCount &&
			RequiredCount == Other.RequiredCount && SelectedBranchId == Other.SelectedBranchId && StageStartedServerTime == Other.StageStartedServerTime && StageDeadlineServerTime == Other.StageDeadlineServerTime &&
			RunTags == Other.RunTags && Rewards == Other.Rewards && DefeatedCount == Other.DefeatedCount && CapturedCount == Other.CapturedCount && Groups == Other.Groups && ElapsedSeconds == Other.ElapsedSeconds &&
			ClearCount == Other.ClearCount && BestSeconds == Other.BestSeconds && Participants == Other.Participants && OwnerPlayer == Other.OwnerPlayer && FailReason == Other.FailReason && BossHealth == Other.BossHealth;
	}
};
