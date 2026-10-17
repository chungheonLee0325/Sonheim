#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MapMarkerComponent.generated.h"

UENUM(BlueprintType)
enum class EMapMarkerKind : uint8
{
	Player,
	QuestGiver,
	FastTravel,
	Dungeon,
	/** Not a component's kind: where a tracked quest leads, which the maps work out from the quests. */
	QuestTarget,
};

/**
 * Puts its actor on the map and the minimap. It lists itself with UWorldMapSubsystem while it plays, on every machine that has
 * the actor, so the maps read one list and know nothing of the actors' classes.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SONHEIM_API UMapMarkerComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UMapMarkerComponent();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Map") EMapMarkerKind Kind = EMapMarkerKind::QuestGiver;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Map") FText Label;
	/** Shown only to a player who has found this id (UWorldMapComponent); None shows it to everyone. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Map") FName DiscoveryId;

	UFUNCTION(BlueprintPure, Category="Map")
	FVector GetMarkerLocation() const { return GetOwner()->GetActorLocation(); }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
