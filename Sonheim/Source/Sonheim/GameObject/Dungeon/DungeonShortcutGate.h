#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DungeonShortcutGate.generated.h"
class UStaticMeshComponent;
class UNavModifierComponent;
struct FDungeonStageRuntimeState;
/** Blocks the shortcut until the run unlocks it. Reads the replicated dungeon snapshot; it never calls the runtime and replicates nothing itself. */
UCLASS()
class SONHEIM_API ADungeonShortcutGate : public AActor
{
	GENERATED_BODY()
public:
	ADungeonShortcutGate();
	/** Run flag that opens this gate. A stage rule sets it the moment the lever is used, and the snapshot carries it to every client. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FName OpenTagName = TEXT("Dungeon.State.ShortcutUnlocked");
	/** Branch that also keeps this gate open, for a run that reaches it without the flag. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FName OpenBranchId = TEXT("Shortcut");
	/** Where the gate moves while it is open. Down by its own height sinks it under the floor. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FVector OpenOffset = FVector(0.f, 0.f, -340.f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") float OpenSeconds = 1.2f;
	bool IsOpen() const { return bOpen; }
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
private:
	void RefreshFromState();
	void HandleStageState(const FDungeonStageRuntimeState& State);
	void HandleGameStateSet(class AGameStateBase* GameState);
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Door;
	/** Closes the doorway to navigation while the gate is shut, so monsters can follow a player through once it opens.
	 * The door itself does not affect navigation; this needs the level's navigation mesh to update modifiers at runtime. */
	UPROPERTY(VisibleAnywhere) TObjectPtr<UNavModifierComponent> NavModifier;
	FVector ClosedLocation = FVector::ZeroVector;
	/** 0 closed, 1 open; only the moving gate ticks. */
	float OpenAlpha = 0.f;
	bool bOpen = false;
	FDelegateHandle StageStateHandle;
	FDelegateHandle WorldHandle;
};
