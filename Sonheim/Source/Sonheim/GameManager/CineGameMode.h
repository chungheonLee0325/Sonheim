// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "CineGameMode.generated.h"

/**
 * 
 */
UCLASS()
class SONHEIM_API ACineGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnSequenceFinished(); 
};
