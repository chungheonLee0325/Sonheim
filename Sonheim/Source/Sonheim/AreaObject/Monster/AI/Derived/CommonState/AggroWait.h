// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Sonheim/AreaObject/Monster/AI/Base/BaseAiState.h"
#include "AggroWait.generated.h"

class AAreaObject;
/**
 * 
 */
UCLASS()
class SONHEIM_API UAggroWait : public UBaseAiState
{
	GENERATED_BODY()

public:
	virtual void InitState() override;
	virtual void CheckIsValid() override;
	virtual void Enter() override;
	virtual void Execute(float dt) override;
	virtual void Exit() override;

private:
	UPROPERTY()
	AAreaObject* m_Target = nullptr;
};
