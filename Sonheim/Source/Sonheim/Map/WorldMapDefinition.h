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
 * The island's map: its painted art, the square of the world the art shows, and how its fog lifts as players explore. The art's top
 * is +X in the world and its right +Y, the way Scripts/Map/capture_topdown.py shoots it; ToUV and ToWorld are the one place that
 * knows.
 */
UCLASS(BlueprintType)
class SONHEIM_API UWorldMapDefinition : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Map") TObjectPtr<UTexture2D> Art;
	/** The world X and Y at the art's bottom left corner (smallest X, smallest Y) and its top right one. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Map") FVector2D WorldMin = FVector2D(0.0, 0.0);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Map") FVector2D WorldMax = FVector2D(1.0, 1.0);
	/** The fog is a grid of this many cells a side over the art; a cell clears once a player has been near it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Map") int32 FogCells = 64;
	/** How near, in world centimetres: at least the minimap's reach, so the minimap never shows fog around the player. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Map") float RevealRadius = 4000.f;
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

	/** The fog cells within RevealRadius of a world point, as indices (row * FogCells + column). */
	TArray<int32> CellsNear(const FVector& World) const
	{
		TArray<int32> Cells;
		const FVector2D UV = ToUV(World);
		const double Reach = RevealRadius / (WorldMax.X - WorldMin.X) * FogCells;
		const FVector2D At = UV * FogCells;
		for (int32 Row = FMath::Max(0, FMath::FloorToInt(At.Y - Reach)); Row <= FMath::Min(FogCells - 1, FMath::FloorToInt(At.Y + Reach)); ++Row)
			for (int32 Column = FMath::Max(0, FMath::FloorToInt(At.X - Reach)); Column <= FMath::Min(FogCells - 1, FMath::FloorToInt(At.X + Reach)); ++Column)
				if (FVector2D::Distance(FVector2D(Column + 0.5, Row + 0.5), At) <= Reach) Cells.Add(Row * FogCells + Column);
		return Cells;
	}

	/** Whether the art shows a world point: a place away from the island, such as the dungeon, is not on the map. */
	UFUNCTION(BlueprintPure, Category="Map")
	bool Shows(const FVector& World) const
	{
		const FVector2D UV = ToUV(World);
		return UV.X >= 0.0 && UV.X <= 1.0 && UV.Y >= 0.0 && UV.Y <= 1.0;
	}
};
