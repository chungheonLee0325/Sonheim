// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Sonheim/AreaObject/Attribute/HealthComponent.h"
#include "Sonheim/ResourceManager/SonheimGameType.h"
#include "BaseResourceObject.generated.h"


class USonheimGameInstance;
class UBoxComponent;class ASonheimGameMode;
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
	
	// 클라이언트에게 데미지 효과 적용 (VFX, SFX 등)
	UFUNCTION(NetMulticast, Reliable)
	void MulticastDamageEffect(float Damage, FVector HitLocation, AActor* DamageCauser, float ElementDamageMultiplier);
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	UBoxComponent* m_BoxComponent;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	UStaticMeshComponent* m_StaticMeshComponent;
private:
	UFUNCTION(BlueprintCallable, Category="Data")
	float GetWeaknessModifier(EAttackType AttackType) const;

	
	UPROPERTY()
	UHealthComponent* HealthComponent;

	FResourceObjectData* dt_ResourceObject;

	bool CanHarvest = true;

	UPROPERTY()
	USonheimGameInstance* m_GameInstance;
	UPROPERTY()
	ASonheimGameMode* m_GameMode;

	
	// GameJam으로 추가
	// ToDo: 필요한 기능들 위로 올리기
public:
		
	UPROPERTY(EditAnywhere)
	int m_ResourceObjectID = 0;
	
};
