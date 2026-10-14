#pragma once

#include "CoreMinimal.h"
#include "MapMarkerComponent.h"

class UCanvasPanel;
class UImage;
class UQuestComponent;
class UTexture2D;
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

namespace SonheimMap
{
	/** The regions of the ARegionZone actors in the world, which stand on every machine. */
	TArray<FMapRegion> CollectRegions(const UWorld* World);
	/** The region a world point is in seen from above, the smallest zone first (as ARegionZone::ShowWhereStanding), or null. */
	const FMapRegion* RegionAt(const TArray<FMapRegion>& Regions, const FVector& Point);
	/** Where the player's tracked, unfinished quests lead next: a region to walk into, the person or site an objective or the
	 * report names, the dungeon to clear. */
	TArray<FVector> TrackedQuestPlaces(const UWorld* World, const TArray<FMapRegion>& Regions, const UQuestComponent* Quests);
}

/** Draws icons on a canvas, reusing its images from one frame to the next: Begin, Add each, End. */
struct FMapIconLayer
{
	void Begin() { Used = 0; }
	/** At Position on the canvas (its centre), Size across, turned by Angle degrees. Returns the image. */
	UImage* Add(UCanvasPanel* Canvas, UTexture2D* Icon, const FLinearColor& Color, float Size, const FVector2D& Position, float Angle = 0.f);
	/** Hides the images not added since Begin. */
	void End();

private:
	TArray<TWeakObjectPtr<UImage>> Images;
	TArray<TWeakObjectPtr<UTexture2D>> Icons;
	int32 Used = 0;
};
