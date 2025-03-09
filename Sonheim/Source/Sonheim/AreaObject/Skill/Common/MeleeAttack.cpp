// Fill out your copyright notice in the Description page of Project Settings.


#include "MeleeAttack.h"

#include "Sonheim/AreaObject/Base/AreaObject.h"
#include "Sonheim/GameObject/ResourceObject/BaseResourceObject.h"

void UMeleeAttack::OnCastStart(class AAreaObject* Caster, AAreaObject* Target)
{
	bDebugDraw = Caster->bShowDebug;
	Super::OnCastStart(Caster, Target);
}

void UMeleeAttack::OnCastEnd()
{
	Super::OnCastEnd();
	NotifyStateMap.Empty();
}

void UMeleeAttack::CancelCast()
{
	Super::CancelCast();
}

void UMeleeAttack::OnCastTick(float DeltaTime)
{
	Super::OnCastTick(DeltaTime);
	
	if (m_CurrentPhase != ESkillPhase::PostCasting)
	{
		return;
	}
	if (NotifyStateMap.IsEmpty())
	{
		return;
	}

	// 키 배열을 복사하여 안전하게 순회
	TArray<int> Keys;
	NotifyStateMap.GetKeys(Keys);

	for (int AttackDataIndex : Keys)
	{
		// 존재하는지 다시 확인 (CancelCast가 실행될 수도 있으므로)
		if (NotifyStateMap.Contains(AttackDataIndex))
		{
			ProcessHitDetection(AttackDataIndex);
		}
	}
}

void UMeleeAttack::SetCasterMesh(int AttackDataIndex)
{
	// 초기화
	if (AttackDataIndex == 0) NotifyStateMap.Empty();

	FAttackCollision AttackCollision;
	bIsHitOnce = false;
	AttackCollision.bHasPreviousPositions = false;
	// LOG_PRINT(TEXT("히트 업데이트"));

	AttackCollision.IndexedAttackData = GetAttackDataByIndex(AttackDataIndex);
	if (AttackCollision.IndexedAttackData == nullptr)
	{
		return;
	}
	if (AttackCollision.IndexedAttackData->HitBoxData.MeshComponentTag == NAME_None)
	{
		AttackCollision.OwnerSourceMesh = m_Caster->GetMesh();
	}
	// 태그로 지정된 메시 찾기
	TArray<UActorComponent*> Components;
	m_Caster->GetComponents(USkeletalMeshComponent::StaticClass(), Components);

	for (UActorComponent* Component : Components)
	{
		if (Component->ComponentHasTag(AttackCollision.IndexedAttackData->HitBoxData.MeshComponentTag))
		{
			AttackCollision.OwnerSourceMesh = Cast<USkeletalMeshComponent>(Component);
		}
	}
	AttackCollision.IsEnableHitDetection = true;
	NotifyStateMap.Add(AttackDataIndex, AttackCollision);
}

void UMeleeAttack::ProcessHitDetection(int AttackDataIndex)
{
		FAttackCollision* AttackCollision = NotifyStateMap.Find(AttackDataIndex);
	if (!AttackCollision->OwnerSourceMesh || !m_Caster)
		return;

	// 현재 위치 가져오기
	FVector CurrentStartLocation = AttackCollision->OwnerSourceMesh->GetSocketLocation(
		AttackCollision->IndexedAttackData->HitBoxData.StartSocketName);
	FVector CurrentEndLocation = AttackCollision->IndexedAttackData->HitBoxData.EndSocketName != NAME_None
		                             ? AttackCollision->OwnerSourceMesh->GetSocketLocation(
			                             AttackCollision->IndexedAttackData->HitBoxData.EndSocketName)
		                             : CurrentStartLocation;
	FRotator CurrentSocketRotation = AttackCollision->OwnerSourceMesh->GetSocketRotation(
		AttackCollision->IndexedAttackData->HitBoxData.StartSocketName);
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(m_Caster);

	TArray<FHitResult> AllHitResults;
	bool bAnyHit = false;

	// 보간 사용 여부 확인
	if (AttackCollision->IndexedAttackData->HitBoxData.bUseInterpolation && AttackCollision->bHasPreviousPositions)
	{
		// 보간 스텝 수 가져오기
		int32 Steps = FMath::Max(2, AttackCollision->IndexedAttackData->HitBoxData.InterpolationSteps);

		// 각 보간 스텝에 대해 콜리전 체크 수행
		for (int32 i = 1; i <= Steps; i++)
		{
			float Alpha = static_cast<float>(i) / Steps;

			// 위치와 회전 보간
			FVector InterpStartLocation = FMath::Lerp(AttackCollision->PreviousStartLocation, CurrentStartLocation,
			                                          Alpha);
			FVector InterpEndLocation = FMath::Lerp(AttackCollision->PreviousEndLocation, CurrentEndLocation, Alpha);
			FRotator InterpRotation = FMath::Lerp(AttackCollision->PreviousSocketRotation, CurrentSocketRotation,
			                                      Alpha);

			// 이 보간된 위치에서 콜리전 체크 수행
			TArray<FHitResult> StepHitResults;
			bool bStepHit = PerformCollisionCheck(
				AttackCollision->IndexedAttackData->HitBoxData.DetectionType,
				InterpStartLocation,
				InterpEndLocation,
				InterpRotation,
				AttackCollision->IndexedAttackData->HitBoxData,
				QueryParams,
				StepHitResults
			);

			// 디버그 드로잉 
			if (bDebugDraw)
			{
				DrawDebugHitDetection(AttackDataIndex, InterpStartLocation, InterpEndLocation, StepHitResults,
				                      InterpRotation);
			}

			if (bStepHit)
			{
				bAnyHit = true;
				AllHitResults.Append(StepHitResults);
			}
		}
	}
	else
	{
		// 기존 단일 콜리전 체크
		bAnyHit = PerformCollisionCheck(
			AttackCollision->IndexedAttackData->HitBoxData.DetectionType,
			CurrentStartLocation,
			CurrentEndLocation,
			CurrentSocketRotation,
			AttackCollision->IndexedAttackData->HitBoxData,
			QueryParams,
			AllHitResults
		);

		// 디버그 드로잉
		if (bDebugDraw)
		{
			DrawDebugHitDetection(AttackDataIndex, CurrentStartLocation, CurrentEndLocation, AllHitResults,
			                      CurrentSocketRotation);
		}
	}
	// 이전 위치 업데이트
	AttackCollision->PreviousStartLocation = CurrentStartLocation;
	AttackCollision->PreviousEndLocation = CurrentEndLocation;
	AttackCollision->PreviousSocketRotation = CurrentSocketRotation;
	AttackCollision->bHasPreviousPositions = true;

	if (!bAnyHit)
	{
		return;
	}

	bIsHitOnce = true;

	// 히트 결과 처리 (중복 제거)
	TSet<AActor*> ProcessedActors;

	for (FHitResult& Hit : AllHitResults)
	{
		AActor* hitActor = Hit.GetActor();
		// 이미 히트한 액터는 스킵
		if (hitActor == nullptr || AttackCollision->HitActors.Contains(hitActor) || ProcessedActors.Contains(hitActor))
		{
			continue;
		}

		ProcessedActors.Add(hitActor);
		AttackCollision->HitActors.Add(hitActor);

		AAreaObject* hitAreaObject = Cast<AAreaObject>(hitActor);
		if (hitAreaObject != nullptr)
		{
			m_Caster->CalcDamage(*AttackCollision->IndexedAttackData, m_Caster, hitActor, Hit);
			continue;
		}
		ABaseResourceObject* hitResourceObject = Cast<ABaseResourceObject>(hitActor);
		if (hitResourceObject != nullptr)
		{
			m_Caster->CalcDamage(*AttackCollision->IndexedAttackData, m_Caster, hitActor, Hit);
			continue;
		}
	}
}

void UMeleeAttack::ResetCollisionData(int AttackDataIndex)
{
	FAttackCollision* AttackCollision = NotifyStateMap.Find(AttackDataIndex);
	if (AttackCollision == nullptr) return;
	NotifyStateMap.Remove(AttackDataIndex);
	AttackCollision->IsEnableHitDetection = false;
	AttackCollision->HitActors.Empty();
	AttackCollision->IndexedAttackData = nullptr;
	AttackCollision->OwnerSourceMesh = nullptr;
	AttackCollision->bHasPreviousPositions = false;
}

bool UMeleeAttack::PerformCollisionCheck(EHitDetectionType DetectionType, const FVector& StartLocation,
	const FVector& EndLocation, const FRotator& SocketRotation, const FHitBoxData& HitBoxData,
	const FCollisionQueryParams& QueryParams, TArray<FHitResult>& OutHitResults)
{
	bool bHit = false;
	UWorld* World = m_Caster->GetWorld();

	switch (DetectionType)
	{
	case EHitDetectionType::Line:
		{
			bHit = World->LineTraceMultiByChannel(
				OutHitResults,
				StartLocation,
				EndLocation,
				ECC_GameTraceChannel2,
				QueryParams
			);
			break;
		}
	case EHitDetectionType::Sphere:
		{
			bHit = World->SweepMultiByChannel(
				OutHitResults,
				StartLocation,
				EndLocation,
				FQuat::Identity,
				ECC_GameTraceChannel2,
				FCollisionShape::MakeSphere(HitBoxData.Radius),
				QueryParams
			);
			break;
		}
	case EHitDetectionType::Capsule:
		{
			FVector location = (StartLocation + EndLocation) / 2.f;
			bHit = World->SweepMultiByChannel(
				OutHitResults,
				location,
				location,
				SocketRotation.Quaternion(),
				ECC_GameTraceChannel2,
				FCollisionShape::MakeCapsule(HitBoxData.Radius, HitBoxData.HalfHeight),
				QueryParams
			);
			break;
		}
	case EHitDetectionType::Box:
		{
			FVector location = (StartLocation + EndLocation) / 2.f;
			bHit = World->SweepMultiByChannel(
				OutHitResults,
				location,
				location,
				SocketRotation.Quaternion(),
				ECC_GameTraceChannel2,
				FCollisionShape::MakeBox(HitBoxData.BoxExtent),
				QueryParams
			);
			break;
		}
	}

	return bHit;
}

void UMeleeAttack::ResetNextSkillByBHit()
{
	Super::ResetNextSkillByBHit();
	if (bIsHitOnce)
	{
		return;
	}
	m_NextSkillID = 0;
}

void UMeleeAttack::DrawDebugHitDetection(int AttackDataIndex, const FVector& Start, const FVector& End,
	const TArray<FHitResult>& HitResults, const FRotator& SocketRotation)
{
	FAttackCollision* AttackCollision = NotifyStateMap.Find(AttackDataIndex);
	auto& HitBoxData = AttackCollision->IndexedAttackData->HitBoxData;
	UWorld* World = m_Caster->GetWorld();
	if (!World) return;

	const float Duration = DebugDrawDuration;
	const bool bPersistent = bPersistentLines;
	const FColor TraceColor = DebugColor;
	const FColor HitColor = FColor::Green;

	switch (HitBoxData.DetectionType)
	{
	case EHitDetectionType::Line:
		{
			DrawDebugLine(
				World,
				Start,
				End,
				TraceColor,
				bPersistent,
				Duration,
				0,
				2.0f
			);
			break;
		}

	case EHitDetectionType::Sphere:
		{
			// 시작점과 끝점에 구체 그리기
			DrawDebugSphere(
				World,
				Start,
				HitBoxData.Radius,
				12,
				TraceColor,
				bPersistent,
				Duration
			);

			if (Start != End)
			{
				DrawDebugSphere(
					World,
					End,
					HitBoxData.Radius,
					12,
					TraceColor,
					bPersistent,
					Duration
				);

				// 구체들을 연결하는 라인
				DrawDebugLine(
					World,
					Start,
					End,
					TraceColor,
					bPersistent,
					Duration
				);
			}
			break;
		}

	case EHitDetectionType::Capsule:
		{
			DrawDebugCapsule(
				World,
				(Start + End) * 0.5f,
				HitBoxData.HalfHeight,
				HitBoxData.Radius,
				SocketRotation.Quaternion(),
				TraceColor,
				bPersistent,
				Duration
			);
			break;
		}

	case EHitDetectionType::Box:
		{
			DrawDebugBox(
				World,
				(Start + End) * 0.5f,
				HitBoxData.BoxExtent,
				SocketRotation.Quaternion(),
				TraceColor,
				bPersistent,
				Duration
			);
			break;
		}
	}

	// 히트 포인트 표시
	for (const FHitResult& Hit : HitResults)
	{
		DrawDebugPoint(
			World,
			Hit.ImpactPoint,
			10.0f,
			HitColor,
			bPersistent,
			Duration
		);

		DrawDebugLine(
			World,
			Hit.ImpactPoint,
			Hit.ImpactPoint + Hit.ImpactNormal * 30.0f,
			HitColor,
			bPersistent,
			Duration
		);
	}
}
