// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Sonheim/GameObject/ResourceObject/BaseResourceObject.h"
#include "Tree.generated.h"

UCLASS()
class SONHEIM_API ATree : public ABaseResourceObject
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ATree();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
