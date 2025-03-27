// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Sonheim/Element/BaseElement.h"
#include "ParabolaElement.generated.h"

UCLASS()
class SONHEIM_API AParabolaElement : public ABaseElement
{
	GENERATED_BODY()

public:
	AParabolaElement();

	virtual void InitElement(AAreaObject* Caster, AAreaObject* Target, const FVector& TargetLocation,
	                         FAttackData* AttackData) override;

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	virtual void OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	                            UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
	                            const FHitResult& SweepResult) override;

	virtual void OnComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	                            FVector NormalImpulse, const FHitResult& Hit) override;

	UFUNCTION(BlueprintImplementableEvent)
	void HandleBeginOverlap(AAreaObject* Caster, AActor* Target);
};
