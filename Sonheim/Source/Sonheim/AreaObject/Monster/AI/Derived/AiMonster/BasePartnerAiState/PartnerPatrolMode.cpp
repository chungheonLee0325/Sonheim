// Fill out your copyright notice in the Description page of Project Settings.


#include "PartnerPatrolMode.h"

#include "AIController.h"
#include "NavigationSystem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "Sonheim/AreaObject/Monster/BaseMonster.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"

void UPartnerPatrolMode::InitState()
{}

void UPartnerPatrolMode::CheckIsValid()
{}

void UPartnerPatrolMode::Enter()
{
	if (m_Owner->bShowDebug)
	{
		FLog::Log("UPartnerPatrolMode");
	}

	NavSystem = UNavigationSystemV1::GetCurrent(GetWorld());
}

void UPartnerPatrolMode::Execute(float dt)
{
	// 장착 명령 받으면
	if (m_Owner->IsCalled)
	{
		MoveToPlayer();
		return;
	}

	if (StopUpdateLocation)
	{
		return;
	}

	// Todo : 시작 위치를 소환한 파트너로 설정
	ASonheimPlayer* Player{Cast<ASonheimPlayer>(GetWorld()->GetFirstPlayerController()->GetPawn())};
	float Dist{
		static_cast<float>(FVector::Distance(m_Owner->GetActorLocation(),
		                                     Player->GetActorLocation()))
	};
	// 너무 멀면 텔레포트
	if (Dist > 2000.f)
	{
		TeleportToPlayer();

		return;
	}
	// 적당히 멀면 쫓아오게
	if (Dist > 1000.f)
	{
		PatrolToPlayer();

		return;
	}

	FlowTime += dt;

	if (FlowTime > PatrolTime)
	{
		FlowTime = 0.f;
		Patrol();
	}

	FlowTimeForJump += dt;
	if (FlowTimeForJump > JumpTime)
	{
		FlowTimeForJump = 0.f;
		JumpTime = FMath::RandRange(3.f, 6.f);
		m_Owner->Jump();
	}
}

void UPartnerPatrolMode::Exit()
{}

void UPartnerPatrolMode::Patrol()
{
	if (!NavSystem)
	{
		return;
	}

	if (m_Owner->bShowDebug)
	{
		//FLog::Log("PartnerPatrolling");
	}

	// Todo : 시작 위치를 소환한 파트너로 설정
	ASonheimPlayer* Player{Cast<ASonheimPlayer>(GetWorld()->GetFirstPlayerController()->GetPawn())};
	FVector Start{Player->GetActorLocation()};
	FNavLocation Next;

	NavSystem->GetRandomReachablePointInRadius(Start, 500.f, Next);

	m_Owner->AIController->MoveToLocation(Next.Location);

	PatrolTime = FMath::RandRange(5.f, 8.f);
}

void UPartnerPatrolMode::PatrolToPlayer()
{
	StopUpdateLocation = true;

	FNavLocation Next;
	// Todo : 시작 위치를 소환한 파트너로 설정
	ASonheimPlayer* Player{Cast<ASonheimPlayer>(GetWorld()->GetFirstPlayerController()->GetPawn())};
	FVector TargetLocation{Player->GetActorLocation()};
	FVector Direction{Player->GetActorForwardVector()};
	FVector DesiredLocation{TargetLocation + Direction * 100.f};

	NavSystem->ProjectPointToNavigation(DesiredLocation, Next);

	m_Owner->GetCharacterMovement()->MaxWalkSpeed = 1000.f;

	EPathFollowingRequestResult::Type Result{m_Owner->AIController->MoveToLocation(Next.Location)};

	if (Result == EPathFollowingRequestResult::Type::RequestSuccessful)
	{
		m_Owner->AIController->GetPathFollowingComponent()->OnRequestFinished.AddUObject(
			this, &UPartnerPatrolMode::PatrolMoveCompleted);
	}
}

void UPartnerPatrolMode::TeleportToPlayer()
{
	FLog::Log("TeleportToPlayer");

	FNavLocation Next;
	// Todo : 시작 위치를 소환한 파트너로 설정
	ASonheimPlayer* Player{Cast<ASonheimPlayer>(GetWorld()->GetFirstPlayerController()->GetPawn())};
	FVector TargetLocation{Player->GetActorLocation()};
	FVector Direction{Player->GetActorForwardVector()};
	FVector DesiredLocation{TargetLocation + Direction * 100.f};

	NavSystem->ProjectPointToNavigation(DesiredLocation, Next);

	m_Owner->SetActorLocation(Next.Location);
}

void UPartnerPatrolMode::MoveToPlayer()
{
	FNavLocation Next;
	// Todo : 시작 위치를 소환한 파트너로 설정
	ASonheimPlayer* Player{Cast<ASonheimPlayer>(GetWorld()->GetFirstPlayerController()->GetPawn())};
	FVector TargetLocation{Player->GetActorLocation()};
	FVector Direction{Player->GetActorForwardVector()};
	FVector DesiredLocation{TargetLocation + Direction * 100.f};

	NavSystem->ProjectPointToNavigation(DesiredLocation, Next);

	m_Owner->GetCharacterMovement()->MaxWalkSpeed = 1000.f;

	EPathFollowingRequestResult::Type Result{m_Owner->AIController->MoveToLocation(Next.Location)};

	if (Result == EPathFollowingRequestResult::Type::RequestSuccessful)
	{
		m_Owner->AIController->GetPathFollowingComponent()->OnRequestFinished.AddUObject(
			this, &UPartnerPatrolMode::MoveCompleted);
	}
}

void UPartnerPatrolMode::MoveCompleted(struct FAIRequestID RequestID, const struct FPathFollowingResult& Result)
{
	// 이동 완료
	if (Result.IsSuccess())
	{
		//FLog::Log("UPutDistance::MoveCompleted");
		m_Owner->GetCharacterMovement()->MaxWalkSpeed = 600.f;
		// PartnerSkillMode
		ChangeState(m_NextState);
	}

	// 바인딩 해제
	m_Owner->AIController->GetPathFollowingComponent()->OnRequestFinished.RemoveAll(this);
}

void UPartnerPatrolMode::PatrolMoveCompleted(struct FAIRequestID RequestID, const struct FPathFollowingResult& Result)
{
	if (Result.IsSuccess())
	{
		FLog::Log("PatrolMoveCompleted");

		m_Owner->GetCharacterMovement()->MaxWalkSpeed = 600.f;
		StopUpdateLocation = false;
	}
}

bool UPartnerPatrolMode::CheckMoveEnable(const FVector& StartLoc, const FVector& EndLoc)
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
