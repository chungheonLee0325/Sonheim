// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Sonheim/ResourceManager/SonheimGameType.h"
#include "MeleeAttackNotifyState.generated.h"

/**
 * 
 */
UCLASS()
class SONHEIM_API UMeleeAttackNotifyState : public UAnimNotifyState
{
	GENERATED_BODY()
	UFUNCTION()
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
							 float TotalDuration) override;

	UFUNCTION()
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;

public:
	// 사용되는 AttackData의 Index값
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttackData)
	int AttackDataIndex = 0;
};
