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
	UPROPERTY(BlueprintReadOnly) float Progress = 0.f;
	UPROPERTY(BlueprintReadOnly) EDungeonRunStatus Status = EDungeonRunStatus::Idle;
	UPROPERTY(BlueprintReadOnly) int32 Revision = 0;
};
