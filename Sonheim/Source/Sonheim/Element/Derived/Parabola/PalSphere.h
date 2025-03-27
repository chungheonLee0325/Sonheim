// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ParabolaElement.h"
#include "PalSphere.generated.h"

UCLASS()
class SONHEIM_API APalSphere : public AParabolaElement
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	APalSphere();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
