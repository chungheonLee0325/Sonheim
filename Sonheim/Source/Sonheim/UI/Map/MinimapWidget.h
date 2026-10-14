#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Sonheim/Map/MapView.h"
#include "MinimapWidget.generated.h"

class UCanvasPanel;
class UImage;
class UMaterialInstanceDynamic;
class UQuestComponent;
class UWorldMapComponent;

/**
 * The round field minimap at the HUD's top right: the island's map (DA_WorldMap) around the player, north up, with the fog of the
 * regions the player has not found, the player's arrow turned to the view and the markers in reach. Where the tracked quests lead
 * shows as a pin, held on the rim when it is farther. It hides itself away from the island, as in the dungeon. MapImage's brush is
 * M_UI_Map, which draws the art, the fog and the round frame.
 */
UCLASS(Abstract)
class SONHEIM_API UMinimapWidget : public UUserWidget
{
	GENERATED_BODY()
protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta=(BindWidget)) TObjectPtr<UImage> MapImage;
	/** Fills the circle; markers are placed on it from its centre. */
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UCanvasPanel> MarkerLayer;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UImage> PlayerArrow;

	/** How many world centimetres the circle spans across. */
	UPROPERTY(EditAnywhere, Category="Map") float WorldSpan = 7000.f;

private:
	UFUNCTION() void RefreshFog();
	UFUNCTION() void RefreshQuests();
	void PlaceMarkers(const FVector& Center, float Diameter);

	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> MapMaterial;
	FMapIconLayer Icons;
	TArray<FMapRegion> Regions;
	TArray<FVector> QuestPlaces;
	TWeakObjectPtr<UWorldMapComponent> BoundPlaces;
	TWeakObjectPtr<UQuestComponent> BoundQuests;
};
