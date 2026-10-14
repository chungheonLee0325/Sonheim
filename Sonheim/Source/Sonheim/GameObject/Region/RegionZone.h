#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RegionZone.generated.h"
class UBoxComponent;
class ULocalPlayer;
/** A named part of the island. A player who walks in from somewhere else sees its name on their own screen, in the notices' Title
 * slot; walking back into the region whose name they saw last shows nothing, so a player along its edge is not shown it over and
 * over. Zones with the same name are one region, for a region one box does not fit. The zone replicates nothing: each machine sees
 * its own player walk in. */
UCLASS()
class SONHEIM_API ARegionZone : public AActor
{
	GENERATED_BODY()
public:
	ARegionZone();
	/** The region's name and a line under it. Empty shows nothing. */
	/** The region's name for quests that send a player here; zones of one region share it. None counts no walk in. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Region") FName RegionId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Region") FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Region") FText Subtitle;
	/** The channel of the regions' notices. */
	static const FName NoticeChannel;
	/** Shows a local player the region their pawn stands in, for a pawn they just got: one that spawned inside a region never walked
	 * into it. Of nested regions, the smallest is the one shown. */
	static void ShowWhereStanding(const APawn* Pawn, const ULocalPlayer* Local);
	/** Whether a world point lies in the zone's box, for a pawn whose overlaps are not known yet (one just spawned). */
	bool Contains(const FVector& Point) const;
	/** The zone's box in the world. */
	FBox GetBox() const;
protected:
	virtual void BeginPlay() override;
private:
	UFUNCTION() void HandleBeginOverlap(UPrimitiveComponent* Component, AActor* Other, UPrimitiveComponent* OtherComponent, int32 BodyIndex, bool bFromSweep, const FHitResult& Sweep);
	void ShowTo(const ULocalPlayer* Local) const;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> Zone;
};
