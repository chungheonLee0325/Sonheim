// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Sonheim/Element/BaseElement.h"
#include "ElectricBall.generated.h"

UCLASS()
class SONHEIM_API AElectricBall : public ABaseElement
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AElectricBall();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

public:
	// fire actor
	virtual void InitElement(AAreaObject* Caster, AAreaObject* Target,const FVector& TargetLocation, FAttackData* AttackData) override;

	virtual void OnBeginOverlap(UPrimitiveComponent* OverlappedComponent,
									AActor* OtherActor,
									UPrimitiveComponent* OtherComp,
									int32 OtherBodyIndex,
									bool bFromSweep,
									const FHitResult& SweepResult) override;

public:
	FVector StartPos;
	FVector EndPos;
	float Range{1000.f};
	float Speed{250.f};
};
