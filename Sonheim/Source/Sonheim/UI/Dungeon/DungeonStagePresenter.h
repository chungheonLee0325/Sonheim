#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Sonheim/GameManager/Dungeon/DungeonStageRuntimeTypes.h"
#include "DungeonViewData.h"
#include "DungeonStagePresenter.generated.h"
class ASonheimGameState;
class AGameStateBase;
class UDungeonUIRouterSubsystem;
class UDungeonDefinitionDataAsset;
class UDungeonAssetSubsystem;
class UHealthComponent;
UCLASS()
class SONHEIM_API UDungeonStagePresenter : public UObject
{
	GENERATED_BODY()
public:
	void Start(APlayerController* Controller, UDungeonUIRouterSubsystem* Router);
	void Stop();
private:
	void BindGameState(AGameStateBase* State);
	void OnSnapshot(const FDungeonStageRuntimeState& Snapshot);
	/** Banners for what changed since Previous, from the presentation's toast maps. */
	void ShowToasts(const FDungeonStageRuntimeState& Previous);
	/** The local player takes part in the latest run; the run's screens and banners show only to them. */
	bool IsParticipant() const;
	/** When the run leaves a stage, its optional lines stay a few seconds to say whether they were taken. */
	void ResolveOptional(const FDungeonStageRuntimeState& Previous);
	/** Follows the health of the run's players, which changes without a snapshot. */
	void BindMembers(const TArray<UHealthComponent*>& Healths);
	UFUNCTION() void HandleMemberHealth(float CurrentHP, float Delta, float MaxHP);
	void Present();
	TWeakObjectPtr<APlayerController> Owner;
	TWeakObjectPtr<ASonheimGameState> GameState;
	TWeakObjectPtr<UDungeonUIRouterSubsystem> UIRouter;
	TWeakObjectPtr<UDungeonAssetSubsystem> Assets;
	UPROPERTY(Transient) TObjectPtr<UDungeonDefinitionDataAsset> Definition;
	FDungeonStageRuntimeState Latest;
	TArray<FDungeonObjectiveViewData> ResolvedOptional;
	FTimerHandle ResolvedTimer;
	TArray<TWeakObjectPtr<UHealthComponent>> BoundHealth;
	FGuid AssetRequest;
	FPrimaryAssetId RequestedDefinition;
	FDelegateHandle WorldHandle, StateHandle;
};
