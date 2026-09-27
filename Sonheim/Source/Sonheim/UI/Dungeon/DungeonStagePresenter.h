#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Sonheim/GameManager/Dungeon/DungeonStageRuntimeTypes.h"
#include "DungeonStagePresenter.generated.h"
class ASonheimGameState;
class AGameStateBase;
class UDungeonUIRouterSubsystem;
class UDungeonDefinitionDataAsset;
class UDungeonAssetSubsystem;
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
	void Present();
	TWeakObjectPtr<APlayerController> Owner;
	TWeakObjectPtr<ASonheimGameState> GameState;
	TWeakObjectPtr<UDungeonUIRouterSubsystem> UIRouter;
	TWeakObjectPtr<UDungeonAssetSubsystem> Assets;
	UPROPERTY(Transient) TObjectPtr<UDungeonDefinitionDataAsset> Definition;
	FDungeonStageRuntimeState Latest;
	FGuid AssetRequest;
	FPrimaryAssetId RequestedDefinition;
	FDelegateHandle WorldHandle, StateHandle;
};
