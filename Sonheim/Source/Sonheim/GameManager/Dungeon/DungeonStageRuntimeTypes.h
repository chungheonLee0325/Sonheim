#pragma once
#include "CoreMinimal.h"
#include "UObject/PrimaryAssetId.h"
#include "DungeonStageRuntimeTypes.generated.h"
UENUM(BlueprintType)
enum class EDungeonRunStatus : uint8 { Idle, Loading, Running, Succeeded, Failed };

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
	UPROPERTY(BlueprintReadOnly) double StageStartedServerTime = 0;
	bool SamePresentationState(const FDungeonStageRuntimeState& Other) const
	{
		return RunId == Other.RunId && DefinitionAssetId == Other.DefinitionAssetId && StageId == Other.StageId &&
			RunStatus == Other.RunStatus && ObjectiveGroupId == Other.ObjectiveGroupId && CurrentCount == Other.CurrentCount &&
			RequiredCount == Other.RequiredCount && SelectedBranchId == Other.SelectedBranchId && StageStartedServerTime == Other.StageStartedServerTime;
	}
};
