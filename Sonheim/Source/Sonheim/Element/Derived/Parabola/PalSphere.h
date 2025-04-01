// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ParabolaElement.h"
#include "PalSphere.generated.h"

class ABaseMonster;
class ASonheimPlayer;

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

	virtual void OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
							UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
							const FHitResult& SweepResult) override;

	virtual void OnComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
								FVector NormalImpulse, const FHitResult& Hit) override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void InitElement(AAreaObject* Caster, AAreaObject* Target, const FVector& TargetLocation, FAttackData* AttackData) override;

	virtual FVector Fire(AAreaObject* Caster, AAreaObject* Target, FVector TargetLocation, float ArcValue) override;

private:
	bool bCanHit = true;

	void CheckPalCatch(ASonheimPlayer* Caster, ABaseMonster* Target);
};
