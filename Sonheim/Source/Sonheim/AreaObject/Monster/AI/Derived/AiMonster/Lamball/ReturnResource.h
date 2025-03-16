// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Sonheim/AreaObject/Monster/AI/Base/BaseAiState.h"
#include "ReturnResource.generated.h"

/**
 * 
 */
UCLASS()
class SONHEIM_API UReturnResource : public UBaseAiState
{
	GENERATED_BODY()

public:
	virtual void InitState() override;
	virtual void CheckIsValid() override;
	virtual void Enter() override;
	virtual void Execute(float dt) override;
	virtual void Exit() override;
};
