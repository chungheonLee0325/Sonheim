#pragma once
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"
#include "Engine/DataTable.h"
#include "Sonheim/GameObject/InteractableInterface.h"
#include "Sonheim/Utilities/StringTableIds.h"
#include "DungeonTestArea.generated.h"
class UBoxComponent;
class UNiagaraSystem;
class UAudioComponent;
class USoundBase;
class UTextRenderComponent;
class UUserWidget;
class UWidgetComponent;
struct FDungeonStageRuntimeState;

USTRUCT(BlueprintType)
struct FDungeonSpawnPointSet
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(Categories="Dungeon")) FGameplayTag PointSetId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(MakeEditWidget=true)) TArray<FTransform> LocalTransforms;
};
UCLASS()
class SONHEIM_API ADungeonTestArea : public AActor, public IInteractableInterface
{
	GENERATED_BODY()
public:
	ADungeonTestArea();
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") TSoftObjectPtr<UDataTable> Catalog;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FName CatalogRow;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") float InteractionDistance = 250.f;
	/** Players this close to the entrance when a run starts take part in it, alongside the player who started it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") float ParticipationRadius = 8000.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") float ParticipationHeight = 1500.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") TArray<FDungeonSpawnPointSet> PointSets;
	/** Name shown in the Detect prompt. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FText DisplayName;
	/** What F does, after the name: before the first run, after a run, and while the player's level is short ({0} the level needed). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FText StartText = LOCTABLE(SONHEIM_ST_DUNGEON, "Prompt.Altar.Start");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FText RestartText = LOCTABLE(SONHEIM_ST_DUNGEON, "Prompt.Altar.Restart");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FText LevelNeededFormat = LOCTABLE(SONHEIM_ST_DUNGEON, "Prompt.Altar.LevelNeededFormat");
	/** Played on every client where a group of monsters appears, so they arrive instead of popping into view. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") TObjectPtr<UNiagaraSystem> SpawnEffect;
	/** Played to each player of the run while the boss is alive, over the level's music, which the host turns down meanwhile. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") TObjectPtr<USoundBase> BossMusic;
	TArray<FTransform> GetSpawnTransforms(const FGameplayTag& PointSetId) const;
	UFUNCTION(NetMulticast, Unreliable) void MulticastSpawnBurst(const TArray<FVector>& Points);
	virtual bool CanInteract_Implementation() const override { return true; }
	virtual void Interact_Implementation(ASonheimPlayer* Player) override;
	virtual void OnDetected_Implementation(bool bDetected) override;
	virtual FString GetInteractionName_Implementation() const override { return DisplayName.ToString(); }
	virtual EInteractableType GetInteractableType_Implementation() const override { return EInteractableType::Object; }
	virtual float GetHoldDuration_Implementation() const override { return 0.f; }
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
	/** Start or restart while no run is active; hidden while a run loads or runs, when the server refuses a start. */
	void RefreshPrompt();
	/** The catalog row's RequiredLevel, read locally so the prompt can tell the player before they press F. */
	int32 GetRequiredLevel() const;
	int32 GetLocalPlayerLevel() const;
	void HandleStageState(const FDungeonStageRuntimeState& State);
	void RefreshBossMusic(const FDungeonStageRuntimeState& State);
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> BossMusicComponent;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> InteractionBox;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Label;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UWidgetComponent> DetectWidgetComponent;
	UPROPERTY() TSubclassOf<UUserWidget> DetectWidgetClass;
	FDelegateHandle StageStateHandle;
	bool bDetected = false;
};
