#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Sonheim/GameObject/InteractableInterface.h"
#include "DungeonPortal.generated.h"
class UArrowComponent;
class USoundBase;
struct FDungeonStageRuntimeState;
class UBoxComponent;
class UStaticMeshComponent;
class UUserWidget;
class UWidgetComponent;
/** One end of a pair of portals. F moves the player to the other end: the dungeon lies out of sight, and the pair is the way in and out. */
UCLASS()
class SONHEIM_API ADungeonPortal : public AActor, public IInteractableInterface
{
	GENERATED_BODY()
public:
	ADungeonPortal();
	/** The other end. The player arrives on that portal's Arrival arrow, facing the way it points. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") TObjectPtr<ADungeonPortal> Destination;
	/** Name shown in the Detect prompt. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FText DisplayName;
	/** What F does, shown after the name. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FText ActionText;
	/** The server refuses a player farther than this, as the dungeon entrance does; the interaction trace reaches 500 from the camera. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") float InteractionDistance = 350.f;
	/** The portal leads into the dungeon. Going the other way takes a player out of a run, and ends it for the player who started it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") bool bLeadsIntoDungeon = false;
	/** Opens only once the run is won, as the way home from the treasure room; until then the portal and what hangs off it are
	 * hidden, and F does nothing. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") bool bOpensOnClear = false;
	/** Played to the player who travels, as the screen fades in at the other end. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") TObjectPtr<USoundBase> TravelSound;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") float FadeSeconds = 0.6f;
	FTransform GetArrivalTransform() const;
	bool IsOpen() const { return bOpen; }
	virtual bool CanInteract_Implementation() const override { return Destination != nullptr && bOpen; }
	virtual void Interact_Implementation(ASonheimPlayer* Player) override;
	virtual void OnDetected_Implementation(bool bDetected) override;
	virtual FString GetInteractionName_Implementation() const override { return DisplayName.ToString(); }
	virtual EInteractableType GetInteractableType_Implementation() const override { return EInteractableType::Object; }
	virtual float GetHoldDuration_Implementation() const override { return 0.f; }
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
	void RefreshOpen();
	void HandleStageState(const FDungeonStageRuntimeState& State);
	void HandleGameStateSet(class AGameStateBase* GameState);
	FDelegateHandle StageStateHandle;
	FDelegateHandle WorldHandle;
	bool bOpen = true;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> InteractionBox;
	/** The look of the portal; each placed portal sets its own mesh. */
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Mesh;
	/** Where a player coming from the other end stands, on the floor, and the way they face. */
	UPROPERTY(VisibleAnywhere) TObjectPtr<UArrowComponent> Arrival;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UWidgetComponent> DetectWidgetComponent;
	UPROPERTY() TSubclassOf<UUserWidget> DetectWidgetClass;
};
