// Fill out your copyright notice in the Description page of Project Settings.


#include "PutDistance.h"

#include "AIController.h"
#include "NavigationSystem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "Sonheim/AreaObject/Monster/BaseMonster.h"
#include "Sonheim/Utilities/LogMacro.h"

void UPutDistance::InitState()
{}

void UPutDistance::CheckIsValid()
{}

void UPutDistance::Enter()
{
	//FLog::Log("UPutDistance");

	MoveToAttack();
	m_Owner->ChangeFace(EFaceType::Sad);
}

void UPutDistance::Execute(float dt)
{}

void UPutDistance::Exit()
{}

void UPutDistance::MoveToAttack()
{
	UNavigationSystemV1* NavSystem{UNavigationSystemV1::GetCurrent(GetWorld())};
	if (!NavSystem)
	{
		return;
	}

	if (m_Owner->bShowDebug)
	{
		FLog::Log("MoveToAttack");
	}

	FNavLocation Next;
	for (int32 i{}; i < 36; ++i)
	{
		FVector TargetLocation{m_Owner->GetAggroTarget()->GetActorLocation()};
		FVector Direction{(m_Owner->GetActorLocation() - TargetLocation).GetSafeNormal()};

		// Direction 회전
		const float Yaw{0.f + 10.f * i};
		FQuat Rotation{FQuat(FRotator(0.f, Yaw, 0.f))};
		Direction = Rotation.RotateVector(Direction);
		
		FVector DesiredLocation{TargetLocation + Direction * 1000.f};

		if (!NavSystem->ProjectPointToNavigation(DesiredLocation, Next))
		{
			// 갈 곳 없으면 그냥 스킬 발사해
			ChangeState(m_NextState);
			return;
		}

		// 공격이 안되는 곳이면
		if (!CheckAttackEnable(TargetLocation, Next.Location))
		{
			// 마지막까지 안되면 그냥 거기서 스킬 발사해
			if (i == 15)
			{
				ChangeState(m_NextState);
				return;
			}
			
			// 이동할 위치 다시 찾기
			//FLog::Log("UPutDistance::CheckIsValid");
			continue;
		}
		
		break;
	}

	m_Owner->LookAtLocation(m_Owner->GetAggroTarget()->GetActorLocation(),EPMRotationMode::Duration,0.1f);

	m_Owner->GetCharacterMovement()->MaxWalkSpeed = 2000.f;
	EPathFollowingRequestResult::Type Result{m_Owner->AIController->MoveToLocation(Next.Location)};

	if (Result == EPathFollowingRequestResult::Type::RequestSuccessful)
	{
		m_Owner->AIController->GetPathFollowingComponent()->OnRequestFinished.AddUObject(
			this, &UPutDistance::MoveCompleted);
	}
	else
	{
		FLog::Log("Failed to move to PathFollowingRequestResult");
	}
}

void UPutDistance::MoveCompleted(struct FAIRequestID RequestID, const struct FPathFollowingResult& Result)
{
	// 이동 완료
	if (Result.IsSuccess())
	{
		//FLog::Log("UPutDistance::MoveCompleted");
		m_Owner->GetCharacterMovement()->MaxWalkSpeed = 600.f;
		// UseSkill
		ChangeState(m_NextState);
	}

	// 바인딩 해제
	m_Owner->AIController->GetPathFollowingComponent()->OnRequestFinished.RemoveAll(this);
}

bool UPutDistance::CheckAttackEnable(const FVector& StartLoc, const FVector& EndLoc)
{
	// StartLoc에서 EndLoc까지 중심점
	const FVector TraceCenter{(StartLoc + EndLoc) * 0.5f};

	// StartLoc과 EndLoc 방향
	const FVector Direction{(EndLoc - StartLoc).GetSafeNormal()};
	// StartLoc과 EndLoc 거리 계산
	const float Distance{static_cast<float>(FVector::Distance(StartLoc, EndLoc))};

	// 박스의 크기
	const FVector TraceExtent{FVector(Distance * 0.5f, 50.f, 5.f)};
	// 방향 벡터로 회전 계산
	const FQuat BoxRotation{FRotationMatrix::MakeFromX(Direction).ToQuat()};

	// BoxTrace
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(m_Owner);
	QueryParams.AddIgnoredActor(m_Owner->GetAggroTarget());
	const FCollisionShape CollisionShape{FCollisionShape::MakeBox(TraceExtent)};
	const bool bHit{
		GetWorld()->SweepSingleByChannel(
			HitResult,
			TraceCenter,
			TraceCenter,
			BoxRotation,
			ECC_Visibility,
			CollisionShape,
			QueryParams
		)
	};

	// 디버그용으로 BoxTrace 시각화
	if (m_Owner->bShowDebug)
	{
		if (bHit)
		{
			DrawDebugBox(GetWorld(), TraceCenter, TraceExtent, BoxRotation, FColor::Red, false, 2.f);
		}
		else
		{
			DrawDebugBox(GetWorld(), TraceCenter, TraceExtent, BoxRotation, FColor::Green, false, 2.f);
		}
	}

	// hit 없으면 공격 가능
	return !bHit;
}
