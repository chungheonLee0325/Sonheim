// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GhostTrail.generated.h"


UCLASS()
class SONHEIM_API AGhostTrail : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AGhostTrail();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
	void Init(USkeletalMeshComponent* Pawn, float FadeOutDuration = 0.5f, float EnableDelay = 0.0f);
	void InitByMaterials(USkeletalMeshComponent* Pawn, float FadeOutDuration = 0.5f, float EnableDelay = 0.0f);
	void SetInitialOpacity(float Opacity) { InitialOpacity = Opacity; };

private:
	UPROPERTY(EditDefaultsOnly)
	class UPoseableMeshComponent* PoseableMesh;

	//UPROPERTY(EditDefaultsOnly)
	//class UMaterialInstance* GhostMaterial;

	UPROPERTY(EditDefaultsOnly)
	TArray<class UMaterialInstanceDynamic*> DynamicMaterials;

	bool IsSpawned = false;
	float FadeCountDown;
	float EnableCountDown;
	bool IsEnabled = false;

	UPROPERTY()
	TArray<UMaterialInterface*> OriginalMaterials;
	UPROPERTY()
	UMaterialInterface* GhostMaterial;
	float FadeOutTime;
	float InitialOpacity = 0.7f;
};
