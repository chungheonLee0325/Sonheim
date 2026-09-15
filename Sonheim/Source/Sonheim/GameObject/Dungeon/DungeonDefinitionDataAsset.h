#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "DungeonStageDefinition.h"
#include "DungeonDefinitionDataAsset.generated.h"
class UDungeonPresentationDataAsset;

USTRUCT(BlueprintType)
struct FDungeonCatalogRow : public FTableRowBase
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName DungeonId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FPrimaryAssetId DefinitionAssetId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1")) int32 RecommendedLevel = 1;
};

UCLASS(BlueprintType)
class SONHEIM_API UDungeonDefinitionDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon", AssetRegistrySearchable) FName DefinitionId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FName StartStageId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") TSoftObjectPtr<UDungeonPresentationDataAsset> Presentation;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon", meta=(TitleProperty="StageId")) TArray<FDungeonStageDefinition> Stages;
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	const FDungeonStageDefinition* FindStage(FName Id) const;
	// The same structural checks gate editor authoring and runtime entry.
	bool ValidateDefinition(TArray<FString>& Errors, TArray<FString>& Warnings) const;
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
