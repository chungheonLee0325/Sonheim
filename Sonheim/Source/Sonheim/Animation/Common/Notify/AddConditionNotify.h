// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Sonheim/ResourceManager/SonheimGameType.h"
#include "AddConditionNotify.generated.h"

/**
 * 
 */
UCLASS()
class SONHEIM_API UAddConditionNotify : public UAnimNotify
{
	GENERATED_BODY()
	
	UFUNCTION()
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EConditionBitsType Condition;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DurationTime;
};
