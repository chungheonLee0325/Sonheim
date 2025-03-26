// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Sonheim/AreaObject/Monster/AI/Base/BaseAiState.h"
#include "Chase.generated.h"

/**
 * 
 */
UCLASS()
class SONHEIM_API UChase : public UBaseAiState
{
	GENERATED_BODY()
	
public:
	virtual void InitState() override;
	virtual void CheckIsValid() override;
	virtual void ServerEnter() override;
	virtual void ServerExecute(float dt) override;
	virtual void ServerExit() override;

	void Chase(float dt);
	void DoJump();

	float JumpTime{2.f};
	float FlowTime{};

	float PatrolRange{};
	float AttackRange{};
};
