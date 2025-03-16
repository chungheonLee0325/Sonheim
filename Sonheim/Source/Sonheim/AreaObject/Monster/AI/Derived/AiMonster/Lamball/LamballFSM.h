// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Sonheim/AreaObject/Monster/AI/Base/BaseAiFSM.h"
#include "LamballFSM.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SONHEIM_API ULamballFSM : public UBaseAiFSM
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	ULamballFSM();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	virtual void InitStatePool() override;

	
};
