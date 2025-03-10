// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Sonheim/AreaObject/Monster/BaseMonster.h"
#include "LamBall.generated.h"

UCLASS()
class SONHEIM_API ALamBall : public ABaseMonster
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ALamBall();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
};
