// Fill out your copyright notice in the Description page of Project Settings.


#include "MoveUtilComponent.h"


// Sets default values for this component's properties
UMoveUtilComponent::UMoveUtilComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}


// Called when the game starts
void UMoveUtilComponent::BeginPlay()
{
	Super::BeginPlay();
}


// Called every frame
void UMoveUtilComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                       FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastUpdateTime < MinUpdateInterval)
	{
		return;
	}
	LastUpdateTime = CurrentTime;

	// Update Movement
	if (MovementState.IsActive())
	{
		MovementState.Update(DeltaTime);
		const float Alpha = MovementState.GetProgress();
		const float InterpolatedAlpha = CalculateInterpolationAlpha(Alpha, MovementState.InterpType);

		// 실제 이동 적용
		const FVector NewPosition = FMath::Lerp(MovementState.StartPos, MovementState.TargetPos, InterpolatedAlpha);
		AActor* Owner = GetOwner();
		Owner->SetActorLocation(NewPosition, true);


		if (Alpha >= 1.0f)
		{
			MovementState.Stop();
		}
	}
	// Debug Visualization
	if (bShowDebugPath)
	{
		DrawDebugVisuals();
	}
}


void UMoveUtilComponent::MoveActorTo(const FVector& TargetPosition, float Duration,
                                     EMovementInterpolationType InterpType, bool bStickToGround)
{
	if (!GetOwner()) return;

	FVector AdjustedTargetPosition = TargetPosition;

	if (bStickToGround)
	{
		// 라인 트레이스를 통해 지형의 높이를 감지
		FHitResult HitResult;
		FVector TraceStart = TargetPosition + FVector(0.0f, 0.0f, 1000.0f); // 위에서 시작
		FVector TraceEnd = TargetPosition - FVector(0.0f, 0.0f, 1000.0f); // 아래로 트레이스

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(GetOwner());

		if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams))
		{
			AdjustedTargetPosition.Z = HitResult.Location.Z;
		}
	}

	StartNewMovement(AdjustedTargetPosition, EPMMovementMode::Duration, Duration, InterpType);
}

void UMoveUtilComponent::MoveActorToWithSpeed(const FVector& TargetPosition, float Speed,
                                              EMovementInterpolationType InterpType, bool bStickToGround)
{
	if (!GetOwner()) return;

	FVector AdjustedTargetPosition = TargetPosition;

	if (bStickToGround)
	{
		// 라인 트레이스를 통해 지형의 높이를 감지
		FHitResult HitResult;
		FVector TraceStart = TargetPosition + FVector(0.0f, 0.0f, 1000.0f); // 위에서 시작
		FVector TraceEnd = TargetPosition - FVector(0.0f, 0.0f, 1000.0f); // 아래로 트레이스

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(GetOwner());

		if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams))
		{
			AdjustedTargetPosition.Z = HitResult.Location.Z;
		}
	}

	StartNewMovement(AdjustedTargetPosition, EPMMovementMode::Speed, Speed, InterpType);
}

void UMoveUtilComponent::StopMovement()
{
	if (!MovementState.IsActive()) return;

	AActor* Owner = GetOwner();
	const FVector CurrentPos = Owner->GetActorLocation();
	Owner->SetActorLocation(CurrentPos);

	MovementState = FMovementState();
}

void UMoveUtilComponent::StartNewMovement(const FVector& Target, EPMMovementMode Mode,
                                          float SpeedOrDuration, EMovementInterpolationType InterpType)
{
	StopMovement();


	MovementState.StartPos = GetOwner()->GetActorLocation();
	MovementState.TargetPos = Target;
	MovementState.MovementMode = Mode;
	MovementState.InterpType = InterpType;

	if (Mode == EPMMovementMode::Duration)
	{
		MovementState.Duration = SpeedOrDuration;
		MovementState.Speed = 0.0f;
	}
	else
	{
		MovementState.Speed = SpeedOrDuration;
		MovementState.Duration = (Target - MovementState.StartPos).Size() / SpeedOrDuration;
	}

	MovementState.CurrentTime = 0.0f;
	MovementState.bIsActive = true;
}

float UMoveUtilComponent::CalculateInterpolationAlpha(float RawAlpha, EMovementInterpolationType InterpType)
{
	const float n1 = 7.5625;
	const float d1 = 2.75;
	const float c4 = (2 * PI) / 3;
	switch (InterpType)
	{
	case EMovementInterpolationType::Linear:
		return RawAlpha;
	case EMovementInterpolationType::EaseIn:
		return RawAlpha * RawAlpha;
	case EMovementInterpolationType::EaseOut:
		return 1.0f - FMath::Square(1.0f - RawAlpha);
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

void FMovementState::Update(float DeltaTime)
{
	if (!bIsActive) return;

	CurrentTime += DeltaTime;
	if (MovementMode == EPMMovementMode::Speed)
	{
		Duration = (TargetPos - StartPos).Size() / Speed;
	}
}

void FMovementState::Stop()
{
	bIsActive = false;
	CurrentTime = 0.0f;
	Duration = 0.0f;
	Speed = 0.0f;
	StartPos = FVector::ZeroVector;
	TargetPos = FVector::ZeroVector;
	MovementMode = EPMMovementMode::Duration;
	InterpType = EMovementInterpolationType::Linear;
}

void UMoveUtilComponent::DrawDebugVisuals() const
{
	if (!GetWorld()) return;

	if (bShowDebugPath && MovementState.IsActive())
	{
		DrawDebugLine(GetWorld(), MovementState.StartPos, MovementState.TargetPos,
		              FColor::Green, false, 1.0f, 0, 2.0f);
		DrawDebugPoint(GetWorld(), MovementState.TargetPos, 10.0f, FColor::Red, false, 1.0f);
	}
}
