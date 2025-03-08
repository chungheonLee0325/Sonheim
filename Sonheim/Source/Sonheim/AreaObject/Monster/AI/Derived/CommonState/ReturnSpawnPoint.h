// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Sonheim/AreaObject/Monster/AI/Base/BaseAiState.h"
#include "ReturnSpawnPoint.generated.h"

/**
 * 
 */
UCLASS()
class SONHEIM_API UReturnSpawnPoint : public UBaseAiState
{
	GENERATED_BODY()
public:
	virtual void InitState() override;
	virtual void CheckIsValid() override;
	virtual void Enter() override;
	virtual void Execute(float dt) override;
	virtual void Exit() override;

	void SetReturnAccelSpeed(float ReturnAccelSpeed)
	{
		m_ReturnAccelSpeed = ReturnAccelSpeed;
	}

private:
	float m_ReturnAccelSpeed = 500.f;
};
