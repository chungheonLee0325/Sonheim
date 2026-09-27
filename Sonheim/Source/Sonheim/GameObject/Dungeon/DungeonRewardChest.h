#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DungeonRewardChest.generated.h"
class UNiagaraSystem;
class USoundBase;
class UStaticMeshComponent;
struct FDungeonStageRuntimeState;
/** The chest of the treasure room. It opens when the run is won and closes for the next run. Like the shortcut gate, every client
 * reads the replicated snapshot and moves its own lid; the chest replicates nothing. The rewards themselves go straight to the inventories. */
UCLASS()
class SONHEIM_API ADungeonRewardChest : public AActor
{
	GENERATED_BODY()
public:
	ADungeonRewardChest();
	/** Where the lid turns, from the chest's origin on the floor. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FVector Hinge = FVector(0.f, -60.f, 70.f);
	/** The lid's turn when open, around the hinge. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FRotator OpenRotation = FRotator(0.f, 0.f, -100.f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") float OpenSeconds = 0.8f;
	/** Played where the chest stands when it opens, not when a player arrives to a chest already open. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") TObjectPtr<UNiagaraSystem> OpenEffect;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") TObjectPtr<USoundBase> OpenSound;
	bool IsOpen() const { return bOpen; }
protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
private:
	void RefreshFromState(bool bAnimate);
	void HandleStageState(const FDungeonStageRuntimeState& State);
	void HandleGameStateSet(class AGameStateBase* GameState);
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Base;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> LidPivot;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Lid;
	/** 0 shut, 1 open; only a moving lid ticks. */
	float OpenAlpha = 0.f;
	bool bOpen = false;
	FDelegateHandle StageStateHandle;
	FDelegateHandle WorldHandle;
};
