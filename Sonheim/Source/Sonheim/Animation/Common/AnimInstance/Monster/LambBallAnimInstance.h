// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Sonheim/Animation/Common/AnimInstance/BaseAnimInstance.h"
#include "LambBallAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class SONHEIM_API ULambBallAnimInstance : public UBaseAnimInstance
{
	GENERATED_BODY()
	
public:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Speed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EAiStateType State;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsDead{false};
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsJump{false};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsDizzy{false};
};
