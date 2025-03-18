// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseWeapon.h"
#include "Pickaxe.generated.h"

UCLASS()
class SONHEIM_API APickaxe : public ABaseWeapon
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	APickaxe();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void HandleLeftButtonPressed() override;
	virtual void HandleRightButtonPressed() override;
};
