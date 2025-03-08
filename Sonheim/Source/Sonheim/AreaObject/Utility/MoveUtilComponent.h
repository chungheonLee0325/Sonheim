// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Sonheim/ResourceManager/SonheimGameType.h"
#include "MoveUtilComponent.generated.h"

UENUM(BlueprintType)
enum class EPMMovementMode : uint8
{
	Duration UMETA(DisplayName = "Duration Based"),
	Speed UMETA(DisplayName = "Speed Based")
};

class FMovementState
{
public:
	void Update(float DeltaTime);
	void Stop();
	bool IsActive() const  { return bIsActive; }
	float GetProgress() const  { return CurrentTime / Duration; }

	bool bIsActive = false;
	float CurrentTime = 0.0f;
	float Duration = 0.0f;
	float Speed = 0.0f;
	FVector StartPos = FVector::ZeroVector;
	FVector TargetPos = FVector::ZeroVector;
	EPMMovementMode MovementMode = EPMMovementMode::Duration;
	EMovementInterpolationType InterpType = EMovementInterpolationType::Linear;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SONHEIM_API UMoveUtilComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UMoveUtilComponent();

	void MoveActorTo(const FVector& TargetPosition, float Duration,
					 EMovementInterpolationType InterpType = EMovementInterpolationType::Linear,
					 bool bStickToGround = false);

	void MoveActorToWithSpeed(const FVector& TargetPosition, float Speed,
							  EMovementInterpolationType InterpType = EMovementInterpolationType::Linear,
							  bool bStickToGround = false);

	// Control Interface
	void StopMovement();

	// State Queries
	bool IsMoving() const { return MovementState.IsActive(); }

	// Configuration
	UPROPERTY(EditAnywhere, Category = "Performance")
	float MinUpdateInterval = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Performance")
	float LocationUpdateThreshold = 0.1f;

	UPROPERTY(EditAnywhere, Category = "Debug")
	bool bShowDebugPath = false;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
							   FActorComponentTickFunction* ThisTickFunction) override;

private:
	// State
	FMovementState MovementState;

	// Timing
	float LastUpdateTime = 0.0f;

	// Internal Methods
	void StartNewMovement(const FVector& Target,
						  EPMMovementMode Mode, float SpeedOrDuration,
						  EMovementInterpolationType InterpType);

	static float CalculateInterpolationAlpha(float RawAlpha, EMovementInterpolationType InterpType);

	void DrawDebugVisuals() const;
};
