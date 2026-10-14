#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Sonheim/GameObject/InteractableInterface.h"
#include "FastTravelPoint.generated.h"

class ASonheimPlayer;
class UArrowComponent;
class UBoxComponent;
class UMapMarkerComponent;
class UMaterialInstanceDynamic;
class UPointLightComponent;
class UStaticMeshComponent;
class UUserWidget;
class UWidgetComponent;

/**
 * A stone players fast travel to. A player activates it with the interaction key (UWorldMapComponent records it) and from then on
 * can travel here from the map; using an active stone opens the map. A pillar of light over it is red until the looking player
 * activates it and blue after. Arrivals stand at the arrow, facing along it.
 */
UCLASS()
class SONHEIM_API AFastTravelPoint : public AActor, public IInteractableInterface
{
	GENERATED_BODY()
public:
	AFastTravelPoint();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Map") FName PointId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Map") FText DisplayName;
	/** The prompt's action before and after the looking player activated the stone. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Map") FText ActivateText;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Map") FText OpenMapText;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Map") float InteractionDistance = 350.f;
	/** The light pillar's material (M_TravelBeam, Color parameter) and its colours. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Map") TObjectPtr<UMaterialInterface> BeamMaterial;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Map") FLinearColor InactiveColor = FLinearColor(1.f, 0.12f, 0.08f, 1.f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Map") FLinearColor ActiveColor = FLinearColor(0.1f, 0.55f, 1.f, 1.f);

	/** Server: puts a travelling player at the arrival, the way ADungeonPortal does. */
	void Receive(ASonheimPlayer* Player) const;

	virtual bool CanInteract_Implementation() const override { return !PointId.IsNone(); }
	virtual void Interact_Implementation(ASonheimPlayer* Player) override;
	virtual void OnDetected_Implementation(bool bDetected) override;
	virtual FString GetInteractionName_Implementation() const override { return DisplayName.ToString(); }
	virtual EInteractableType GetInteractableType_Implementation() const override { return EInteractableType::Object; }
	virtual float GetHoldDuration_Implementation() const override { return 0.f; }

protected:
	virtual void BeginPlay() override;

private:
	/** Whether the player on this machine activated the stone. */
	bool IsActiveHere() const;
	/** Colours the pillar for the player on this machine. */
	void RefreshLook();

	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Base;
	/** What the interaction trace hits. */
	UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> InteractionBox;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Mesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Beam;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> Glow;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UArrowComponent> Arrival;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UMapMarkerComponent> Marker;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UWidgetComponent> DetectWidgetComponent;
	UPROPERTY() TSubclassOf<UUserWidget> DetectWidgetClass;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> BeamDrawn;
	UPROPERTY(EditAnywhere, Category="Map") float FadeSeconds = 0.6f;
	FTimerHandle LookTimer;
	int8 ShownActive = -1;
};
