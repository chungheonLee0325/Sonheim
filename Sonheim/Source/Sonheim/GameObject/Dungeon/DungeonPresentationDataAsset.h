#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DungeonPresentationDataAsset.generated.h"
USTRUCT(BlueprintType)
struct FDungeonStagePresentation
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName StageId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FText Title;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FText Objective;
};
UCLASS(BlueprintType)
class SONHEIM_API UDungeonPresentationDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FText DungeonTitle;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") TArray<FDungeonStagePresentation> Stages;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") TMap<FName, FText> BranchLabels;
};
