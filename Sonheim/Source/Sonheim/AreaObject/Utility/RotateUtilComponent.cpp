// Fill out your copyright notice in the Description page of Project Settings.


#include "RotateUtilComponent.h"

#include "Kismet/KismetMathLibrary.h"

// Fill out your copyright notice in the Description page of Project Settings.

URotateUtilComponent::URotateUtilComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void URotateUtilComponent::BeginPlay()
{
	Super::BeginPlay();
}

void URotateUtilComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                         FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastUpdateTime < MinUpdateInterval)
	{
		return;
	}
	LastUpdateTime = CurrentTime;

	// Update Rotation
	if (RotationState.IsActive())
	{
		RotationState.Update(DeltaTime);
		const float Alpha = RotationState.GetProgress();
		const float InterpolatedAlpha = CalculateInterpolationAlpha(Alpha, RotationState.InterpType);

		// 실제 회전 적용
		const FRotator NewRotation = FMath::Lerp(RotationState.StartRotation, RotationState.TargetRotation,
		                                         InterpolatedAlpha);

		if (AActor* Owner = GetOwner())
		{
			Owner->SetActorRotation(NewRotation);
		}

		if (Alpha >= RotationState.TargetRatio)
		{
			RotationState.Stop();
		}
	}
	// Debug Visualization
	if (bShowRotationDebug)
	{
		DrawDebugVisuals();
	}
}

void URotateUtilComponent::RotateActorByRotator(const FRotator& TargetRotation, EPMRotationMode Mode, float Duration,
                                                float Ratio,
                                                EMovementInterpolationType InterpType)
{
	if (!GetOwner() || Duration <= 0.0f) return;
	StartNewRotation(TargetRotation, Mode, Duration, Ratio, InterpType);
}

void URotateUtilComponent::LookAtLocation(const FVector& TargetLocation, EPMRotationMode Mode, float DurationOrSpeed,
                                          float Ratio,
                                          EMovementInterpolationType InterpType)
{
	if (!GetOwner() || DurationOrSpeed <= 0.0f) return;

	FVector actorLocation = GetOwner()->GetActorLocation();
	FVector targetLocation = TargetLocation;
	actorLocation.Z = 0;
	targetLocation.Z = 0;
	FRotator rotator = UKismetMathLibrary::FindLookAtRotation(actorLocation, targetLocation);

	RotateActorByRotator(rotator, Mode, DurationOrSpeed, Ratio, InterpType);
}

void URotateUtilComponent::LookAtLocationDirect(const FVector& TargetLocation) const
{
	if (!GetOwner()) return;
	FVector actorLocation = GetOwner()->GetActorLocation();
	FVector targetLocation = TargetLocation;
	actorLocation.Z = 0;
	targetLocation.Z = 0;
	FRotator rotator = UKismetMathLibrary::FindLookAtRotation(actorLocation, targetLocation);
	GetOwner()->SetActorRotation(rotator);
}

void URotateUtilComponent::StopRotation()
{
	if (!RotationState.IsActive()) return;

	if (AActor* Owner = GetOwner())
	{
		const FRotator CurrentRot = Owner->GetActorRotation();
		Owner->SetActorRotation(CurrentRot);
	}

	RotationState = FRotationState();
}

void URotateUtilComponent::StartNewRotation(const FRotator& TargetRot, EPMRotationMode Mode, float SpeedOrDuration,
                                            float Ratio,
                                            EMovementInterpolationType InterpType)
{
	StopRotation();

	RotationState.StartRotation = GetOwner()->GetActorRotation();
	RotationState.TargetRotation = TargetRot;
	RotationState.InterpType = InterpType;
	RotationState.TargetRatio = Ratio;
	RotationState.RotationMode = Mode;
	if (Mode == EPMRotationMode::Duration)
	{
		RotationState.Duration = SpeedOrDuration;
		RotationState.Speed = 0.0f;
	}
	else
	{
		RotationState.Speed = SpeedOrDuration;
		const float MaxRotationDelta = FMath::Abs(
			FMath::FindDeltaAngleDegrees(RotationState.StartRotation.Yaw, TargetRot.Yaw));
		RotationState.Duration = MaxRotationDelta / SpeedOrDuration;
	}

	RotationState.CurrentTime = 0.0f;
	RotationState.bIsActive = true;
}

float URotateUtilComponent::CalculateInterpolationAlpha(float RawAlpha, EMovementInterpolationType InterpType)
{
	const float n1 = 7.5625;
	const float d1 = 2.75;
	const float c4 = (2 * PI) / 3;
	switch (InterpType)
	{
	case EMovementInterpolationType::Linear:
		return FMath::Clamp(RawAlpha, 0.f, 1.f);
	case EMovementInterpolationType::EaseIn:
		return FMath::Clamp(RawAlpha * RawAlpha, 0.f, 1.f);
	case EMovementInterpolationType::EaseOut:
		return FMath::Clamp(1.0f - FMath::Square(1.0f - RawAlpha), 0.f, 1.f);
	case EMovementInterpolationType::EaseInOut:
		return RawAlpha < 0.5f ? 2.0f * RawAlpha * RawAlpha : 1.0f - FMath::Pow(-2.0f * RawAlpha + 2.0f, 2.0f) / 2.0f;
	case EMovementInterpolationType::EaseOutBounce:
		if (RawAlpha < 1 / d1)
		{
			return n1 * RawAlpha * RawAlpha;
		}
		else if (RawAlpha < 2 / d1)
		{
			return n1 * (RawAlpha -= 1.5 / d1) * RawAlpha + 0.75;
		}
		else if (RawAlpha < 2.5 / d1)
		{
			return n1 * (RawAlpha -= 2.25 / d1) * RawAlpha + 0.9375;
		}
		else
		{
			return n1 * (RawAlpha -= 2.625 / d1) * RawAlpha + 0.984375;
		}
	case EMovementInterpolationType::EaseOutElastic:
		return RawAlpha == 0
			       ? 0
			       : RawAlpha == 1
			       ? 1
			       : FMath::Pow(2, -10 * RawAlpha) * FMath::Sin((RawAlpha * 10 - 0.75) * c4) + 1;
	default:
		return RawAlpha;
	}
}

void FRotationState::Update(float DeltaTime)
{
	if (!bIsActive) return;
	CurrentTime += DeltaTime;
	if (RotationMode == EPMRotationMode::Speed)
	{
		const float MaxRotationDelta = FMath::Abs(FMath::FindDeltaAngleDegrees(StartRotation.Yaw, TargetRotation.Yaw));
		Duration = MaxRotationDelta / Speed;
	}
}

void FRotationState::Stop()
{
	bIsActive = false;
	CurrentTime = 0.0f;
	Duration = 0.0f;
	Speed = 0.0f;
	TargetRatio = 1.0f;
	StartRotation = FRotator::ZeroRotator;
	TargetRotation = FRotator::ZeroRotator;
	RotationMode = EPMRotationMode::Duration;
	InterpType = EMovementInterpolationType::Linear;
}

void URotateUtilComponent::DrawDebugVisuals() const
{
	if (!GetWorld()) return;

	if (bShowRotationDebug && RotationState.IsActive())
	{
		const FVector CurrentLocation = GetOwner()->GetActorLocation();
		DrawDebugLine(GetWorld(), CurrentLocation, RotationState.StartRotation.Vector() * 100.0f + CurrentLocation,
					  FColor::Green, false, 2.0f, 0, 4.0f);
		DrawDebugLine(GetWorld(), CurrentLocation, RotationState.TargetRotation.Vector() * 100.0f + CurrentLocation,
					  FColor::Red, false, 2.0f, 0, 4.0f);
	}
}