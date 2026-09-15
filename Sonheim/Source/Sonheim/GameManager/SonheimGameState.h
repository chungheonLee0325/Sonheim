// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Dungeon/DungeonStageRuntimeTypes.h"
#include "SonheimGameState.generated.h"

/**
 * 
 */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnDungeonStageStateChanged, const FDungeonStageRuntimeState&);

UCLASS()
class SONHEIM_API ASonheimGameState : public AGameStateBase
{
	GENERATED_BODY()
public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	UFUNCTION(BlueprintPure, Category="Dungeon") FDungeonStageRuntimeState GetDungeonStageState() const { return DungeonStageState; }
	void PublishDungeonStageState(FDungeonStageRuntimeState NewState);
	FOnDungeonStageStateChanged OnDungeonStageStateChanged;
private:
	UPROPERTY(ReplicatedUsing=OnRep_DungeonStageState) FDungeonStageRuntimeState DungeonStageState;
	UFUNCTION() void OnRep_DungeonStageState();
};
