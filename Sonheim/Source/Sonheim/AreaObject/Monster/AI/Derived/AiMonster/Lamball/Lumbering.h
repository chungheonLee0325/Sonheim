// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Sonheim/AreaObject/Monster/AI/Base/BaseAiState.h"
#include "Lumbering.generated.h"

/**
 * 
 */
UCLASS()
class SONHEIM_API ULumbering : public UBaseAiState
{
	GENERATED_BODY()

public:
	virtual void InitState() override;
	virtual void CheckIsValid() override;
	virtual void Enter() override;
	virtual void Execute(float dt) override;
	virtual void Exit() override;

	int32 CheckState();
	
	void MoveToLumber();
	void Lumbering(float dt);
	void MoveToStore();
	void StoreLumber(float dt);
	
	int32 MonsterState{0};
	
	bool bIsMoving{false};
	bool bIsLumbering{false};
	bool HasItem{false};
	
	float ActionTime{0.f};
	float LumberingTime{3.f};
	float StoreTime{1.f};
	
	UPROPERTY()
	AActor* Target{nullptr};

	UPROPERTY()
	TArray<AActor*> TargetArr;

	UPROPERTY()
	TArray<AActor*> ItemArr;

	UPROPERTY()
	TArray<ABaseItem*> HaveItemArr;

	UPROPERTY()
	TArray<AActor*> AttachedActors;
};
