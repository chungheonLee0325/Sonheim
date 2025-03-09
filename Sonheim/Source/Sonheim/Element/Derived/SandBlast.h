// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Sonheim/Element/BaseElement.h"
#include "SandBlast.generated.h"

UCLASS()
class SONHEIM_API ASandBlast : public ABaseElement
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ASandBlast();

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
	class UProjectileMovementComponent* SkillProjectileMovement;
};
