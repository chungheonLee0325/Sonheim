#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DungeonTriggerZone.generated.h"
class ADungeonTestArea;
class UBoxComponent;
struct FDungeonStageRuntimeState;
/** Raises AreaEntered with its SourceId when a player of the run steps in. What that does, such as waking a wave, is up to the
 * stage rules. Only the server listens; the zone replicates nothing. */
UCLASS()
class SONHEIM_API ADungeonTriggerZone : public AActor
{
	GENERATED_BODY()
public:
	ADungeonTriggerZone();
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") TObjectPtr<ADungeonTestArea> TestArea;
	/** The name rules use for this zone, such as GuardRoom. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FName SourceId;
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
	UFUNCTION() void HandleBeginOverlap(UPrimitiveComponent* Component, AActor* Other, UPrimitiveComponent* OtherComponent, int32 BodyIndex, bool bFromSweep, const FHitResult& Sweep);
	/** A stage that listens for this zone can begin while a player already stands in it; that player counts as entering then. */
	void HandleStageState(const FDungeonStageRuntimeState& State);
	void Report(AActor* Other) const;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> Zone;
	FName LastStageId;
	FDelegateHandle StageStateHandle;
};
