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
	
	virtual void OnComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
						FVector NormalImpulse, const FHitResult& Hit) override;

public:
	UPROPERTY(Replicated)
	FVector StartPos;
	UPROPERTY(Replicated)
	FVector EndPos;
	UPROPERTY(Replicated)
	float Range{5000.f};
	UPROPERTY(Replicated)
	float Speed{500.f};

	UPROPERTY(Replicated)
	float FlowTime{0.f};
	UPROPERTY(Replicated)
	float AttackTime{5.f};

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;	
};
