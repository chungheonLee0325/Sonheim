// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Sonheim/ResourceManager/SonheimGameType.h"
#include "RotateUtilComponent.generated.h"

// Enums
class FRotationState
{
public:
	virtual ~FRotationState() = default;
	virtual void Update(float DeltaTime);
	virtual void Stop();
	virtual bool IsActive() const { return bIsActive; }
	virtual float GetProgress() const { return CurrentTime / Duration; }

	bool bIsActive = false;
	float CurrentTime = 0.0f;
	float Duration = 0.0f;
	float Speed = 5.0f;
	float TargetRatio = 1.0f;
	FRotator StartRotation = FRotator::ZeroRotator;
	FRotator TargetRotation = FRotator::ZeroRotator;
	EPMRotationMode RotationMode = EPMRotationMode::Speed;
	EMovementInterpolationType InterpType = EMovementInterpolationType::Linear;
};


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SONHEIM_API URotateUtilComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	URotateUtilComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
	void RotateActorByRotator(const FRotator& TargetRotation, EPMRotationMode Mode, float Duration, float Ratio,
	                          EMovementInterpolationType InterpType = EMovementInterpolationType::Linear);
	
	void LookAtLocation(const FVector& TargetLocation, EPMRotationMode Mode, float DurationOrSpeed, float Ratio = 1.0f,
	                    EMovementInterpolationType InterpType = EMovementInterpolationType::Linear);

	void LookAtLocationDirect(const FVector& TargetLocation) const;


	void StopRotation();

	bool IsRotating() const { return RotationState.IsActive(); }

	// Configuration
	UPROPERTY(EditAnywhere, Category = "Performance")
	float MinUpdateInterval = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Performance")
	float RotationUpdateThreshold = 0.1f;

	UPROPERTY(EditAnywhere, Category = "Debug")
	bool bShowRotationDebug = false;

	void DrawDebugVisuals() const;

private:
	// State
	FRotationState RotationState;

	// Timing
	float LastUpdateTime = 0.0f;

	// Internal Methods
	void StartNewRotation(const FRotator& TargetRot, EPMRotationMode Mode, float SpeedOrDuration, float Ratio,
	                      EMovementInterpolationType InterpType);
	static float CalculateInterpolationAlpha(float RawAlpha, EMovementInterpolationType InterpType);
};
