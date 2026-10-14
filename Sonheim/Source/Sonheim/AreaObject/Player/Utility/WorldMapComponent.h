#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerState.h"
#include "WorldMapComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWorldMapChanged);

/**
 * The places one player found: regions (ARegionZone::RegionId, on walking in) and fast travel points (AFastTravelPoint::PointId,
 * on reaching them). Lives on the player's PlayerState. The server records and saves them; the owner gets them through
 * replication, for the map's fog and markers. Fast travel goes only to a point the player found.
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

	/** Moves the player's pawn to a fast travel point the player found, from anywhere the map shows (not from the dungeon). */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category="Map")
	void ServerFastTravel(FName PointId);

	/** The found places changed, on the server and on the owning client. */
	UPROPERTY(BlueprintAssignable, Category="Map")
	FOnWorldMapChanged OnMapChanged;

private:
	UFUNCTION() void OnRep_Discovered() { OnMapChanged.Broadcast(); }
	/** Server: a region walked into, as the PlayerState reports it. */
	void HandleDeed(EPlayerDeed Deed, int32 Id, int32 Count, FName Place);
	/** Server: a pawn that spawns inside a region never walks into it, so the regions a new pawn stands in are found too. */
	UFUNCTION() void HandlePawnSet(APlayerState* Player, APawn* NewPawn, APawn* OldPawn);

	UPROPERTY(ReplicatedUsing=OnRep_Discovered)
	TArray<FName> Discovered;
};
