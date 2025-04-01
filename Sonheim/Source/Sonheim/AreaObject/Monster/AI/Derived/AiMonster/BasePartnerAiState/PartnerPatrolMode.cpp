// Fill out your copyright notice in the Description page of Project Settings.


#include "PartnerPatrolMode.h"

#include "AIController.h"
#include "NavigationSystem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "Sonheim/AreaObject/Monster/BaseMonster.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Sonheim/Utilities/SonheimUtility.h"

void UPartnerPatrolMode::InitState()
{}

void UPartnerPatrolMode::CheckIsValid()
{}

void UPartnerPatrolMode::ServerEnter()
{
	if (m_Owner->bShowDebug)
	{
		//FLog::Log("UPartnerPatrolMode");
	}
	
	JumpTime = FMath::RandRange(1.f, 12.f);
	NavSystem = UNavigationSystemV1::GetCurrent(GetWorld());
}

void UPartnerPatrolMode::ServerExecute(float dt)
{
	FlowTime += dt;
	FlowTimeForJump += dt;

	// 장착 명령 받으면
	if (m_Owner->IsCalled && m_Owner->bIsCanCalled)
	{
		MoveToPlayer();
		return;
	}
	
	if (m_Owner->GetAggroTarget())
	{
		FLog::Log("GetAggroTarget");

		m_Owner->LookAtLocation(m_Owner->GetAggroTarget()->GetActorLocation(),EPMRotationMode::Duration,0.1f);

		// SelectMode
		ChangeState(m_SuccessState);
		return;	
	}

	// 파트너 없으면 일반 패트롤
	if (!m_Owner->PartnerOwner)
	{
		if (FlowTime > PatrolTime)
		{
			FlowTime = 0.f;
			PatrolWild();
		}
		
		if (FlowTimeForJump > JumpTime && m_Owner->bCanJump)
		{
			FlowTimeForJump = 0.f;
			JumpTime = FMath::RandRange(8.f, 12.f);
			m_Owner->Jump();
		}
		
		return;
	}

	if (StopUpdateLocation)
	{
		return;
	}
	
	float Dist{
		static_cast<float>(FVector::Distance(m_Owner->GetActorLocation(),
		                                     m_Owner->PartnerOwner->GetActorLocation()))
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

	if (FlowTime > PatrolTime)
	{
		FlowTime = 0.f;
		Patrol();
	}

	if (FlowTimeForJump > JumpTime && m_Owner->bCanJump)
	{
		FlowTimeForJump = 0.f;
		JumpTime = FMath::RandRange(3.f, 6.f);
		m_Owner->Jump();
	}
}

void UPartnerPatrolMode::ServerExit()
{}

void UPartnerPatrolMode::PatrolWild()
{
	if (!NavSystem)
	{
		return;
	}

	if (m_Owner->bShowDebug)
	{
		//FLog::Log("Patrol");
	}

	FVector Start{m_Owner->GetActorLocation()};
	FNavLocation Next;
	
	NavSystem->GetRandomReachablePointInRadius(Start, 600.f, Next);

	m_Owner->AIController->MoveToLocation(Next.Location);
	
	PatrolTime = FMath::RandRange(5.f, 8.f);
}

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


	ASonheimPlayer* PartnerOwner = {Cast<ABaseMonster>(m_Owner)->PartnerOwner};
	// ToDo : PartnerOwner 설정되면 없애기
	ASonheimPlayer* Player{Cast<ASonheimPlayer>(GetWorld()->GetFirstPlayerController()->GetPawn())};
	//PartnerOwner = Player;
	PartnerOwner = m_Owner->PartnerOwner;
	FVector Start{PartnerOwner->GetActorLocation()};
	FNavLocation Next;

	NavSystem->GetRandomReachablePointInRadius(Start, 500.f, Next);

	m_Owner->AIController->MoveToLocation(Next.Location);

	PatrolTime = FMath::RandRange(5.f, 8.f);
}

void UPartnerPatrolMode::PatrolToPlayer()
{
	StopUpdateLocation = true;

	FNavLocation Next;

	// ASonheimPlayer* PartnerOwner = {Cast<ABaseMonster>(m_Owner)->PartnerOwner};
	// // ToDo : PartnerOwner 설정되면 없애기
	// ASonheimPlayer* Player{Cast<ASonheimPlayer>(GetWorld()->GetFirstPlayerController()->GetPawn())};
	// PartnerOwner = Player;
	FVector TargetLocation{m_Owner->PartnerOwner->GetActorLocation()};
	FVector Direction{m_Owner->PartnerOwner->GetActorForwardVector()};
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

	// ASonheimPlayer* PartnerOwner = {Cast<ABaseMonster>(m_Owner)->PartnerOwner};
	// // ToDo : PartnerOwner 설정되면 없애기
	// ASonheimPlayer* Player{Cast<ASonheimPlayer>(GetWorld()->GetFirstPlayerController()->GetPawn())};
	// PartnerOwner = Player;
	FVector TargetLocation{m_Owner->PartnerOwner->GetActorLocation()};
	FVector Direction{m_Owner->PartnerOwner->GetActorForwardVector()};

	for (int32 i{}; i < 36; ++i)
	{
		// Direction 회전
		const float Yaw{0.f + 10.f * i};
		FQuat Rotation{FQuat(FRotator(0.f, Yaw, 0.f))};
		Direction = Rotation.RotateVector(Direction);
		
		FVector DesiredLocation{TargetLocation + Direction * 100.f};

		if (!NavSystem->ProjectPointToNavigation(DesiredLocation, Next))
		{
			// 갈 곳 없으면
			return;
		}

		// 공격이 안되는 곳이면
		//if (!CheckMoveEnable(TargetLocation, Next.Location))
		if (!USonheimUtility::CheckMoveEnable(this, m_Owner, m_Owner->GetAggroTarget(), TargetLocation, Next.Location))		
		{
			// 그럴 일은 없지만 마지막까지 안되면 그냥
			if (i == 15)
			{
				return;
			}
			
			continue;
		}
		
		break;
	}

	m_Owner->SetActorLocation(Next.Location);
}

void UPartnerPatrolMode::MoveToPlayer()
{
	FNavLocation Next;
	
	FVector TargetLocation{m_Owner->PartnerOwner->GetActorLocation()};
	FVector Direction{m_Owner->PartnerOwner->GetActorForwardVector()};

	for (int32 i{}; i < 36; ++i)
	{
		// Direction 회전
		const float Yaw{0.f + 10.f * i};
		FQuat Rotation{FQuat(FRotator(0.f, Yaw, 0.f))};
		Direction = Rotation.RotateVector(Direction);
		
		FVector DesiredLocation{TargetLocation + Direction * 200.f};

		if (!NavSystem->ProjectPointToNavigation(DesiredLocation, Next))
		{
			// 갈 곳 없으면
			return;
		}

		//if (!CheckMoveEnable(TargetLocation, Next.Location))
		if (!USonheimUtility::CheckMoveEnable(this, m_Owner, m_Owner->GetAggroTarget(), TargetLocation, Next.Location))		
		{
			// 그럴 일은 없지만 마지막까지 안되면 그냥
			if (i == 15)
			{
				return;
			}
			
			continue;
		}
		
		break;
	}
	
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
