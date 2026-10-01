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
	/** The dungeon, by its tag. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(Categories="Dungeon")) FGameplayTag DungeonId;
	/** The number the dungeon's records are kept under, in the save and on any server. A tag can be renamed; this number never changes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1")) int32 DungeonNumber = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FPrimaryAssetId DefinitionAssetId;
	/** The level a player needs to start this dungeon. The entrance refuses below it and says so. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1")) int32 RequiredLevel = 1;
};

/** A grade a successful run earns: the first rule, best first, whose clear time and optional objectives the run meets. */
USTRUCT(BlueprintType)
struct FDungeonGradeRule
{
	GENERATED_BODY()
	/** The grade, such as S; the presentation gives its text. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Grade;
	/** The slowest clear that still earns it, in seconds; 0 takes any time. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0")) float MaxClearSeconds = 0.f;
	/** How many of the run's optional objectives it needs done. A route choice, the optional line whose run tag picks a branch (the
	 *  shortcut lever), is not one: the branch it picks already shows in the time. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0")) int32 RequiredOptionalObjectives = 0;
};

UCLASS(BlueprintType)
class SONHEIM_API UDungeonDefinitionDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	/** The dungeon this graph runs. It names the definition's primary asset id, and every tag the graph uses lies under it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon", meta=(Categories="Dungeon")) FGameplayTag DungeonId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon", meta=(Categories="Dungeon")) FGameplayTag StartStageId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") TSoftObjectPtr<UDungeonPresentationDataAsset> Presentation;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon", meta=(TitleProperty="StageId")) TArray<FDungeonStageDefinition> Stages;
	/** The grades of a successful run, best first; the last one takes every clear. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Result", meta=(TitleProperty="Grade")) TArray<FDungeonGradeRule> GradeRules;
	/** The grade the rules give a successful run of this time with this many optional objectives done; none without rules. */
	FName GradeFor(float ClearSeconds, int32 OptionalObjectivesDone) const;
	/** Whether a transition that picks a branch needs RunTag. */
	bool PicksBranch(const FGameplayTag& RunTag) const;
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	const FDungeonStageDefinition* FindStage(const FGameplayTag& Id) const;
	// The same structural checks gate editor authoring and runtime entry.
	bool ValidateDefinition(TArray<FString>& Errors, TArray<FString>& Warnings) const;
	/** The stage graph as Mermaid text, for a document or a review. */
	UFUNCTION(BlueprintCallable, Category="Dungeon Tools")
	FString BuildStageGraph() const;
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
	/** Runs the same checks the save runs, without saving. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Dungeon Tools", meta=(DisplayName="정의 검사"))
	void ValidateNow();
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Dungeon Tools", meta=(DisplayName="스테이지 그래프 복사"))
	void CopyStageGraph();
#endif
};
