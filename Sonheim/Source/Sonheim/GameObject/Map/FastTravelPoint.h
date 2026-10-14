#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FastTravelPoint.generated.h"

class ASonheimPlayer;
class UArrowComponent;
class UMapMarkerComponent;
class USphereComponent;
class UStaticMeshComponent;

/**
 * A place players fast travel to. A player who walks up to it finds it (UWorldMapComponent) and from then on can travel here from
 * the map. Arrivals stand at the arrow, facing along it.
 */
UCLASS()
class SONHEIM_API AFastTravelPoint : public AActor
{
	GENERATED_BODY()
public:
	AFastTravelPoint();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Map") FName PointId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Map") FText DisplayName;

	/** Server: puts a travelling player at the arrival, the way ADungeonPortal does. */
	void Receive(ASonheimPlayer* Player) const;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION() void HandleBeginOverlap(UPrimitiveComponent* Component, AActor* Other, UPrimitiveComponent* OtherComponent, int32 BodyIndex, bool bFromSweep, const FHitResult& Sweep);

	UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> Reach;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Mesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UArrowComponent> Arrival;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UMapMarkerComponent> Marker;
	UPROPERTY(EditAnywhere, Category="Map") float FadeSeconds = 0.6f;
};
