// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AddMovementNotify.generated.h"


/**
 * 
 */
UCLASS()
class SONHEIM_API UAddMovementNotify : public UAnimNotify
{
	GENERATED_BODY()

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;

public:
	// 이동 시간 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement Setting")
	float Duration = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement Setting")
	float MoveLength = 300.0f;
};
