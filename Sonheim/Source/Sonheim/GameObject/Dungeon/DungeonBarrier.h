#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DungeonBarrier.generated.h"
class UStaticMeshComponent;
class UNavModifierComponent;
class UMaterialInstanceDynamic;
struct FDungeonStageRuntimeState;
/** A see-through wall across a doorway that holds the way on until the step before it is done. Like the shortcut gate, it reads the
 * replicated dungeon snapshot on every machine; it never calls the runtime and replicates nothing itself. */
UCLASS()
class SONHEIM_API ADungeonBarrier : public AActor
{
	GENERATED_BODY()
public:
	ADungeonBarrier();
	/** The stages of a run under way during which the barrier stands. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") TArray<FName> SealedStages;
	/** Also stands while no run is under way and after a won run. A failed run takes it down, so the players can walk back to the altar. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") bool bSealedOutsideRun = false;
	/** How long the wall takes to appear or to fade. It blocks the moment it stands and lets through the moment it falls. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") float FadeSeconds = 0.4f;
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
private:
	void RefreshFromState();
	void HandleStageState(const FDungeonStageRuntimeState& State);
	void HandleGameStateSet(class AGameStateBase* GameState);
	void ApplyFade();
	/** The engine cube, scaled to the doorway. Its material's Fade parameter shows it. */
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Wall;
	/** Closes the doorway to navigation while the barrier stands. The wall affects no navigation itself, so the modifier's failsafe box,
	 * sized to the wall, is the area it closes. This needs the level's navigation mesh to update modifiers at runtime. */
	UPROPERTY(VisibleAnywhere) TObjectPtr<UNavModifierComponent> NavModifier;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> WallMaterial;
	/** 0 gone, 1 standing; only a fading barrier ticks. */
	float Fade = 0.f;
	bool bSealed = false;
	FDelegateHandle StageStateHandle;
	FDelegateHandle WorldHandle;
};
