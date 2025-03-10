// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "BaseItem.generated.h"

UCLASS()
class SONHEIM_API ABaseItem : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ABaseItem();
	void InitializeItem(int ItemID, int ItemValue);

	virtual bool CanBeCollectedBy(ASonheimPlayer* Player);
	virtual void OnCollected(ASonheimPlayer* Player);
	virtual void Tick(float DeltaTime) override;
	void SetItemValue(int ItemValue) { m_ItemValue = ItemValue;}

protected:
	virtual void BeginPlay() override;

	// 아이템 효과 적용(자식 클래스에서 구현)
	virtual void ApplyEffect(class ASonheimPlayer* Player);

	UPROPERTY(EditAnywhere, Category = "Collection")
	USoundBase* CollectionSound;

	UPROPERTY(EditAnywhere, Category = "Collection")
	UParticleSystem* CollectionEffect;

	UPROPERTY(EditAnywhere, Category = "Collection")
	class USphereComponent* CollectionSphere;

	UPROPERTY(EditAnywhere, Category = "Collection")
	class UStaticMeshComponent* ItemMesh;

	UFUNCTION()
	virtual void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent,
							  AActor* OtherActor,
							  UPrimitiveComponent* OtherComp,
							  int32 OtherBodyIndex,
							  bool bFromSweep,
							  const FHitResult& SweepResult);

	bool m_IsCollected;

	int m_ItemValue = 0;

	FItemData* dt_ItemData;
};
