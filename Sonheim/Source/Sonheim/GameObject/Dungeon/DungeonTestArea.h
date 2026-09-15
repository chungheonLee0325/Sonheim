#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DataTable.h"
#include "Sonheim/GameObject/InteractableInterface.h"
#include "DungeonTestArea.generated.h"
class UBoxComponent;
class UTextRenderComponent;
class UUserWidget;
class UWidgetComponent;
struct FDungeonStageRuntimeState;

USTRUCT(BlueprintType)
struct FDungeonSpawnPointSet
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName PointSetId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(MakeEditWidget=true)) TArray<FTransform> LocalTransforms;
};
UCLASS()
class SONHEIM_API ADungeonTestArea : public AActor, public IInteractableInterface
{
	GENERATED_BODY()
public:
	ADungeonTestArea();
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") TSoftObjectPtr<UDataTable> Catalog;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FName CatalogRow;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") float InteractionDistance = 250.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") TArray<FDungeonSpawnPointSet> PointSets;
	/** Name shown in the Detect prompt. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FText DisplayName;
	TArray<FTransform> GetSpawnTransforms(FName PointSetId) const;
	virtual bool CanInteract_Implementation() const override { return true; }
	virtual void Interact_Implementation(ASonheimPlayer* Player) override;
	virtual void OnDetected_Implementation(bool bDetected) override;
	virtual FString GetInteractionName_Implementation() const override { return DisplayName.ToString(); }
	virtual EInteractableType GetInteractableType_Implementation() const override { return EInteractableType::Object; }
	virtual float GetHoldDuration_Implementation() const override { return 0.f; }
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
	/** Start or restart while no run is active; hidden while a run loads or runs, when the server refuses a start. */
	void RefreshPrompt();
	void HandleStageState(const FDungeonStageRuntimeState& State);
	UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> InteractionBox;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Label;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UWidgetComponent> DetectWidgetComponent;
	UPROPERTY() TSubclassOf<UUserWidget> DetectWidgetClass;
	FDelegateHandle StageStateHandle;
	bool bDetected = false;
};
