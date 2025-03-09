// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Sonheim/AreaObject/Attribute/HealthComponent.h"
#include "Sonheim/ResourceManager/SonheimGameType.h"
#include "BaseResourceObject.generated.h"


class ASonheimGameMode;
// damage 비율마다 item spawn
UCLASS()
class SONHEIM_API ABaseResourceObject : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ABaseResourceObject();
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	void OnDestroy();
	void SpawnPartialResources(int32 SegmentsLost);
	virtual float TakeDamage(float Damage, const FDamageEvent& DamageEvent, AController* EventInstigator,
							  AActor* DamageCauser) override;
	
	UPROPERTY(EditAnywhere)
	int m_ResourceObjectID = 0;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
	UFUNCTION(BlueprintCallable, Category="Data")
	float GetWeaknessModifier(EAttackType AttackType) const;

	
	UPROPERTY()
	UHealthComponent* HealthComponent;

	FResourceObjectData* dt_ResourceObject;

	bool CanHarvest = true;

	UPROPERTY()
	ASonheimGameMode* m_GameMode;
};
