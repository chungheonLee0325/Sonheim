// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "SetPlayerStateNotify.generated.h"

/**
 * 
 */
UCLASS()
class SONHEIM_API USetPlayerStateNotify : public UAnimNotify
{
	GENERATED_BODY()

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;

	public:
	UPROPERTY(EditAnywhere)
	EPlayerState PlayerState = EPlayerState::NORMAL;
};
