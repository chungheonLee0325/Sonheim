#include "MapFog.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"

void UMapFog::Show(const TArray<uint8>& Explored, const int32 Cells)
{
	const bool bFirst = !Texture || Size != Cells;
	if (bFirst)
	{
		Size = Cells;
		Texture = UTexture2D::CreateTransient(Cells, Cells, PF_G8);
		Texture->SRGB = false;
		Texture->Filter = TF_Bilinear;
		Texture->AddressX = TA_Clamp;
		Texture->AddressY = TA_Clamp;
		Texture->UpdateResource();
		Shown.SetNumZeroed(Cells * Cells);
	}
	Target.SetNumZeroed(Cells * Cells);
	for (int32 Cell = 0; Cell < Cells * Cells; ++Cell)
		Target[Cell] = Explored.IsValidIndex(Cell / 8) && (Explored[Cell / 8] & (1 << (Cell % 8))) ? 255 : 0;
	// The map opening on what the player already explored shows it as it is; only what clears while it is open fades.
	if (bFirst) Shown = Target;
	bFading = !bFirst && Shown != Target;
	Upload();
}

void UMapFog::Apply(UMaterialInstanceDynamic* Material, const float DeltaSeconds)
{
	if (!Texture || !Material) return;
	if (bFading)
	{
		const int32 Step = FMath::Max(1, FMath::RoundToInt(255.f * DeltaSeconds / FadeSeconds));
		bFading = false;
		for (int32 Cell = 0; Cell < Shown.Num(); ++Cell)
		{
			if (Shown[Cell] == Target[Cell]) continue;
			Shown[Cell] = Shown[Cell] < Target[Cell] ? FMath::Min(Shown[Cell] + Step, (int32)Target[Cell]) : FMath::Max(Shown[Cell] - Step, (int32)Target[Cell]);
			bFading |= Shown[Cell] != Target[Cell];
		}
		Upload();
	}
	Material->SetTextureParameterValue(TEXT("Fog"), Texture);
}

void UMapFog::Upload()
{
	// The render thread reads the copy later and frees it with the region.
	uint8* Data = static_cast<uint8*>(FMemory::Malloc(Shown.Num()));
	FMemory::Memcpy(Data, Shown.GetData(), Shown.Num());
	FUpdateTextureRegion2D* Region = new FUpdateTextureRegion2D(0, 0, 0, 0, Size, Size);
	Texture->UpdateTextureRegions(0, 1, Region, Size, 1, Data, [](uint8* SrcData, const FUpdateTextureRegion2D* Regions)
	{
		FMemory::Free(SrcData);
		delete Regions;
	});
}
