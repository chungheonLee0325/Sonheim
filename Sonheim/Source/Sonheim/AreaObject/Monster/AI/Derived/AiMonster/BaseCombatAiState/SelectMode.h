// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Sonheim/AreaObject/Monster/AI/Base/BaseAiState.h"
#include "SelectMode.generated.h"

/**
 * 
 */
UCLASS()
class SONHEIM_API USelectMode : public UBaseAiState
{
	GENERATED_BODY()
	
public:
	virtual void InitState() override;
	virtual void CheckIsValid() override;
	virtual void ServerEnter() override;
	virtual void ServerExecute(float dt) override;
	virtual void ServerExit() override;

	float AttackAggroRange{5000.f};

	float ChooseModeTime{1.f};
	float FlowTime{};
};
