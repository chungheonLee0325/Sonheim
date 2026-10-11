#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Sonheim/GameObject/InteractableInterface.h"
#include "Sonheim/Quest/QuestTypes.h"
#include "QuestBuildSite.generated.h"

class UBoxComponent;
class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;
class UUserWidget;
class UWidgetComponent;

/** One piece of what a site builds: a mesh where it stands, relative to the site. */
USTRUCT(BlueprintType)
struct FBuildPiece
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMesh> Mesh;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FTransform Transform;
};

/**
 * A building site that quests name as a target: players hand it materials with F (the quest's Deliver objectives on it), and
 * once some player's quest is past the step that asks for them, the building stands for everyone. Until then its pieces show
 * as a see-through plan, which nothing collides with.
 *
 * Whether it stands is read from the players' quests, so the host's save keeps it standing without a record of its own.
 */
UCLASS()
class SONHEIM_API AQuestBuildSite : public AActor, public IInteractableInterface, public IQuestTarget
{
	GENERATED_BODY()
public:
	AQuestBuildSite();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest") FName QuestTargetName;
	/** The quest whose step BuildStep asks for the materials; the building stands once a player's quest is past that step. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest") int32 QuestID = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest") int32 BuildStep = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest") FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest") FText ActionText;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest") TArray<FBuildPiece> Pieces;
	/** Drawn on every piece while the building is only planned. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest") TObjectPtr<UMaterialInterface> PlanMaterial;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest") float InteractionDistance = 450.f;

	UFUNCTION(BlueprintPure, Category="Quest") bool IsBuilt() const { return bBuilt; }

	virtual FName GetQuestTargetName() const override { return QuestTargetName; }
	virtual bool CanInteract_Implementation() const override { return !bBuilt; }
	virtual void Interact_Implementation(ASonheimPlayer* Player) override;
	virtual void OnDetected_Implementation(bool bDetected) override;
	virtual FString GetInteractionName_Implementation() const override { return DisplayName.ToString(); }
	virtual EInteractableType GetInteractableType_Implementation() const override { return EInteractableType::Object; }
	virtual float GetHoldDuration_Implementation() const override { return 0.f; }
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void MakePieces();
	/** Server: stands the building once any player's quest is past the build step. */
	void Refresh();
	UFUNCTION() void OnRep_Built();
	void ApplyLook();

	UPROPERTY(ReplicatedUsing=OnRep_Built) bool bBuilt = false;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> InteractionBox;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UWidgetComponent> DetectWidgetComponent;
	UPROPERTY() TSubclassOf<UUserWidget> DetectWidgetClass;
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> PieceComponents;
	FDelegateHandle QuestsHandle;
};
