#pragma once
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/PrimaryAssetId.h"
#include "DungeonStageRuntimeTypes.generated.h"
UENUM(BlueprintType)
enum class EDungeonRunStatus : uint8 { Idle, Loading, Running, Succeeded, Failed };

USTRUCT(BlueprintType)
struct FDungeonRunReward
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) int32 ItemId = 0;
	UPROPERTY(BlueprintReadOnly) int32 Count = 0;
	bool operator==(const FDungeonRunReward& Other) const { return ItemId == Other.ItemId && Count == Other.Count; }
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
	/** Monsters the run confirmed as defeated, across every group. */
	UPROPERTY(BlueprintReadOnly) int32 DefeatedCount = 0;
	/** Seconds from the start to the terminal stage. It stays 0 while the run is going. */
	UPROPERTY(BlueprintReadOnly) float ElapsedSeconds = 0.f;
	/** How often this dungeon has been finished, and the fastest of those runs. Filled from the saved record when a run ends. */
	UPROPERTY(BlueprintReadOnly) int32 ClearCount = 0;
	UPROPERTY(BlueprintReadOnly) float BestSeconds = 0.f;
	bool SamePresentationState(const FDungeonStageRuntimeState& Other) const
	{
		return RunId == Other.RunId && DefinitionAssetId == Other.DefinitionAssetId && StageId == Other.StageId &&
			RunStatus == Other.RunStatus && ObjectiveGroupId == Other.ObjectiveGroupId && CurrentCount == Other.CurrentCount &&
			RequiredCount == Other.RequiredCount && SelectedBranchId == Other.SelectedBranchId && StageStartedServerTime == Other.StageStartedServerTime && StageDeadlineServerTime == Other.StageDeadlineServerTime &&
			RunTags == Other.RunTags && Rewards == Other.Rewards && DefeatedCount == Other.DefeatedCount && ElapsedSeconds == Other.ElapsedSeconds &&
			ClearCount == Other.ClearCount && BestSeconds == Other.BestSeconds;
	}
};
