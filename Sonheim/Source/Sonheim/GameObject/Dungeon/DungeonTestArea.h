#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DataTable.h"
#include "Sonheim/GameObject/InteractableInterface.h"
#include "DungeonTestArea.generated.h"
class UBoxComponent;
class UTextRenderComponent;

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
	TArray<FTransform> GetSpawnTransforms(FName PointSetId) const;
	virtual bool CanInteract_Implementation() const override { return true; }
	virtual void Interact_Implementation(ASonheimPlayer* Player) override;
	virtual FString GetInteractionName_Implementation() const override { return TEXT("Start branching dungeon"); }
	virtual EInteractableType GetInteractableType_Implementation() const override { return EInteractableType::Object; }
	virtual float GetHoldDuration_Implementation() const override { return 0.f; }
private:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> InteractionBox;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Label;
};
