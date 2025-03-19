// Fill out your copyright notice in the Description page of Project Settings.


#include "PutDistance.h"

#include "AIController.h"
#include "NavigationSystem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "Sonheim/AreaObject/Monster/BaseMonster.h"
#include "Sonheim/Utilities/LogMacro.h"
#include "Sonheim/Utilities/SonheimUtility.h"

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
		if (!USonheimUtility::CheckMoveEnable(this, m_Owner, m_Owner->GetAggroTarget(), TargetLocation, Next.Location))
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

	m_Owner->LookAtLocation(m_Owner->GetAggroTarget()->GetActorLocation(), EPMRotationMode::Duration, 0.1f);

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
