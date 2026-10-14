#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MapMarkerComponent.h"
#include "WorldMapDefinition.generated.h"

class UTexture2D;

/** How a kind of marker looks on the maps. */
USTRUCT(BlueprintType)
struct FMapMarkerStyle
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Map") TObjectPtr<UTexture2D> Icon;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Map") FLinearColor Color = FLinearColor::White;
	/** The icon's size on the minimap, in slate units; the world map scales it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Map") float Size = 22.f;
};

/**
 * The island's map: its painted art, the square of the world the art shows, and the regions its fog covers. The art's top is +X
 * in the world and its right +Y, the way Scripts/Map/capture_topdown.py shoots it; ToUV and ToWorld are the one place that knows.
 */
UCLASS(BlueprintType)
class SONHEIM_API UWorldMapDefinition : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Map") TObjectPtr<UTexture2D> Art;
	/** Each pixel the index + 1 of its region in FogRegions, 0 where no fog falls (the sea). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Map") TObjectPtr<UTexture2D> RegionMask;
	/** The world X and Y at the art's bottom left corner (smallest X, smallest Y) and its top right one. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Map") FVector2D WorldMin = FVector2D(0.0, 0.0);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Map") FVector2D WorldMax = FVector2D(1.0, 1.0);
	/** The regions (ARegionZone::RegionId) the mask draws fog over, in the mask's order. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Map") TArray<FName> FogRegions;
	/** Markers by kind, for the minimap and the world map; a kind left out is not drawn. Player is another player's arrow. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Map") TMap<EMapMarkerKind, FMapMarkerStyle> MarkerStyles;

	/** A world point on the art, 0..1 from its top left corner. */
	UFUNCTION(BlueprintPure, Category="Map")
	FVector2D ToUV(const FVector& World) const
	{
		return FVector2D((World.Y - WorldMin.Y) / (WorldMax.Y - WorldMin.Y), (WorldMax.X - World.X) / (WorldMax.X - WorldMin.X));
	}

	UFUNCTION(BlueprintPure, Category="Map")
	FVector ToWorld(const FVector2D& UV, double Z = 0.0) const
	{
		return FVector(WorldMax.X - UV.Y * (WorldMax.X - WorldMin.X), WorldMin.Y + UV.X * (WorldMax.Y - WorldMin.Y), Z);
	}

	/** The map material's fog: 1 in a channel per fog region (R first, in FogRegions' order) the player found, for Found1..Found4. */
	FLinearColor FoundFog(const TArray<FName>& Discovered) const
	{
		// ponytail: four channels, four fog regions; a fifth needs a second parameter or a small texture of found flags.
		float Found[4] = {0.f, 0.f, 0.f, 0.f};
		for (int32 Index = 0; Index < FMath::Min(FogRegions.Num(), 4); ++Index) Found[Index] = Discovered.Contains(FogRegions[Index]) ? 1.f : 0.f;
		return FLinearColor(Found[0], Found[1], Found[2], Found[3]);
	}

	/** Whether the art shows a world point: a place away from the island, such as the dungeon, is not on the map. */
	UFUNCTION(BlueprintPure, Category="Map")
	bool Shows(const FVector& World) const
	{
		const FVector2D UV = ToUV(World);
		return UV.X >= 0.0 && UV.X <= 1.0 && UV.Y >= 0.0 && UV.Y <= 1.0;
	}
};
