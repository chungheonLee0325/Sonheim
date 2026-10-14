#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerState.h"
#include "WorldMapComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWorldMapChanged);

/**
 * What one player found of the island: regions (ARegionZone::RegionId, on walking in), fast travel points (AFastTravelPoint::PointId,
 * once activated) and the map's explored cells (the fog lifts within UWorldMapDefinition::RevealRadius of where the player has been).
 * Lives on the player's PlayerState. The server records and saves them; the owner gets them through replication, for the maps.
 * Fast travel goes only to a point the player activated.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SONHEIM_API UWorldMapComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UWorldMapComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;

	/** Server: reads the player's saved places, once the player's unique net id is known. */
	void LoadSaved();
	/** Server: records a found place. True the first time. */
	bool Discover(FName Id);

	UFUNCTION(BlueprintPure, Category="Map")
	bool IsDiscovered(FName Id) const { return Discovered.Contains(Id); }
	const TArray<FName>& GetDiscovered() const { return Discovered; }
	/** The explored cells as bits, row by row (UWorldMapDefinition::FogCells a side), first cell in bit 0 of byte 0. */
	const TArray<uint8>& GetExplored() const { return Explored; }
	UFUNCTION(BlueprintPure, Category="Map")
	bool IsExplored(int32 Cell) const { return Explored.IsValidIndex(Cell / 8) && (Explored[Cell / 8] & (1 << (Cell % 8))) != 0; }

	/** Moves the player's pawn to a fast travel point the player found, from anywhere the map shows (not from the dungeon). */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category="Map")
	void ServerFastTravel(FName PointId);

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** The found places or the explored cells changed, on the server and on the owning client. */
	UPROPERTY(BlueprintAssignable, Category="Map")
	FOnWorldMapChanged OnMapChanged;

private:
	UFUNCTION() void OnRep_Discovered() { OnMapChanged.Broadcast(); }
	/** Server: a region walked into, as the PlayerState reports it. */
	void HandleDeed(EPlayerDeed Deed, int32 Id, int32 Count, FName Place);
	/** Server: a pawn that spawns inside a region never walks into it, so the regions a new pawn stands in are found too. */
	UFUNCTION() void HandlePawnSet(APlayerState* Player, APawn* NewPawn, APawn* OldPawn);

	/** Server, a few times a second: clears the fog around the player's pawn. */
	void Explore();
	void Store();

	UPROPERTY(ReplicatedUsing=OnRep_Discovered)
	TArray<FName> Discovered;
	UPROPERTY(ReplicatedUsing=OnRep_Discovered)
	TArray<uint8> Explored;
	FTimerHandle ExploreTimer;
	/** When the explored cells were last written to the save; walking about would otherwise write it every step. */
	double LastStored = 0.0;
	bool bUnstored = false;
};
