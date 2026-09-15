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
