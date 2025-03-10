// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Sonheim/ResourceManager/SonheimGameType.h"
#include "BaseElement.generated.h"

class AAreaObject;

UCLASS()
class SONHEIM_API ABaseElement : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ABaseElement();

	// define parameters, change collision profile
	virtual void InitElement(AAreaObject* Caster, AAreaObject* Target,const FVector& TargetLocation, FAttackData* AttackData);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class USphereComponent* Root;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class UStaticMeshComponent* Mesh;
	
	UPROPERTY(EditAnywhere)
	class AAreaObject* m_Caster;
	UPROPERTY(EditAnywhere)
	AAreaObject* m_Target;
	FVector m_TargetLocation;
	FAttackData* m_AttackData;
	
	UFUNCTION()
	virtual FVector Fire(AAreaObject* Caster, AAreaObject* Target, FVector TargetLocation, float ArcValue);

	UFUNCTION(BlueprintCallable)
	virtual void OnBeginOverlap(UPrimitiveComponent* OverlappedComponent,
									AActor* OtherActor,
									UPrimitiveComponent* OtherComp,
									int32 OtherBodyIndex,
									bool bFromSweep,
									const FHitResult& SweepResult);

	virtual void DestroySelf();
};
