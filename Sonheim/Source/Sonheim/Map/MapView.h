#pragma once

#include "CoreMinimal.h"
#include "MapMarkerComponent.h"

class UCanvasPanel;
class UImage;
class UQuestComponent;
class UTexture2D;
class UWorldMapComponent;
class UWorldMapDefinition;

/** A named region of the island as the maps show it: its name, the line under it and where its label stands. */
struct FMapRegion
{
	FName Id;
	FText Name;
	FText Subtitle;
	/** The middle of the region's biggest zone, where its label goes. */
	FVector Center = FVector::ZeroVector;
	/** The region's zones, as world boxes, to tell which region a point is in. */
	TArray<FBox> Boxes;
};

/** A found fast travel point as the map's travel list shows it. */
struct FMapTravelView
{
	FName PointId;
	FText Name;
	FText Region;
};

/** What the world map shows of one player's records: the places found, the cells explored, where the tracked quests lead and the
 * stones the player can travel to. The map widget draws only this, built anew on each change of the records. */
struct FWorldMapView
{
	TSet<FName> Discovered;
	TArray<uint8> Explored;
	TArray<FVector> QuestPlaces;
	/** The found points, in the world's order. */
	TArray<FMapTravelView> TravelPoints;
};

namespace SonheimMap
{
	FWorldMapView BuildView(const UWorld* World, const TArray<FMapRegion>& Regions, const UWorldMapComponent* Places, const UQuestComponent* Quests);
	/** The regions of the ARegionZone actors in the world, which stand on every machine. */
	TArray<FMapRegion> CollectRegions(const UWorld* World);
	/** The region a world point is in seen from above, the smallest zone first (as ARegionZone::ShowWhereStanding), or null. */
	const FMapRegion* RegionAt(const TArray<FMapRegion>& Regions, const FVector& Point);
	/** Where the player's tracked, unfinished quests lead next: a region to walk into, the person or site an objective or the
	 * report names, the dungeon to clear. */
	TArray<FVector> TrackedQuestPlaces(const UWorld* World, const TArray<FMapRegion>& Regions, const UQuestComponent* Quests);
	/** Which kind is drawn over which where markers meet: the tracked quest's pin on top, then players, stones, quest givers, the
	 * dungeon. Region names lie under every marker. */
	int32 LayerOf(EMapMarkerKind Kind);
	/** A quest pin's tip, at the bottom of its picture, is what points at the place. */
	inline const FVector2D PinTip(0.5, 0.92);
}

/** Draws icons on a canvas, reusing its images from one frame to the next: Begin, Add each, End. */
struct FMapIconLayer
{
	void Begin() { Used = 0; }
	/** At Position on the canvas (by its Alignment point, the centre unless said), Size across, turned by Angle degrees, over the
	 * images of lower Layers. Returns the image. */
	UImage* Add(UCanvasPanel* Canvas, UTexture2D* Icon, const FLinearColor& Color, float Size, const FVector2D& Position, float Angle = 0.f,
	            int32 Layer = 0, const FVector2D& Alignment = FVector2D(0.5, 0.5));
	/** Hides the images not added since Begin. */
	void End();

private:
	TArray<TWeakObjectPtr<UImage>> Images;
	TArray<TWeakObjectPtr<UTexture2D>> Icons;
	int32 Used = 0;
};
