#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MapFog.generated.h"

class UMaterialInstanceDynamic;
class UTexture2D;

/**
 * The maps' fog as a small texture (one texel a cell, white where explored) for M_UI_Map's Fog parameter. Cells a player newly
 * explores brighten over FadeSeconds instead of at once, so the fog melts away as the player walks; the material blurs and
 * roughens its edge.
 */
UCLASS()
class SONHEIM_API UMapFog : public UObject
{
	GENERATED_BODY()
public:
	/** Takes the explored cells (UWorldMapComponent::GetExplored). The first call shows them at once, later ones fade in. */
	void Show(const TArray<uint8>& Explored, int32 Cells);
	/** Moves the fade on and gives the material the texture. */
	void Apply(UMaterialInstanceDynamic* Material, float DeltaSeconds);

	float FadeSeconds = 1.2f;

private:
	void Upload();

	UPROPERTY(Transient) TObjectPtr<UTexture2D> Texture;
	/** Each cell's brightness as drawn, and as it is heading for. */
	TArray<uint8> Shown;
	TArray<uint8> Target;
	int32 Size = 0;
	bool bFading = false;
};
