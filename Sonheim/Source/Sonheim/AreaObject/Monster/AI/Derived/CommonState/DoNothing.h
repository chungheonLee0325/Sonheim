// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Sonheim/AreaObject/Monster/AI/Base/BaseAiState.h"
#include "DoNothing.generated.h"

/**
 * 
 */
UCLASS()
class SONHEIM_API UDoNothing : public UBaseAiState
{
	GENERATED_BODY()
public:
	virtual void InitState() override;
	virtual void Enter() override;
	virtual void Execute(float DeltaTime) override;
	virtual void Exit() override;
};
