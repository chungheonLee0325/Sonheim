// Fill out your copyright notice in the Description page of Project Settings.


#include "GatherToPlayer.h"

#include "AIController.h"
#include "AITypes.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"
#include "Sonheim/AreaObject/Monster/BaseMonster.h"
#include "Sonheim/Utilities/LogMacro.h"

void UGatherToPlayer::InitState()
{
}

void UGatherToPlayer::CheckIsValid()
{
}

void UGatherToPlayer::Enter()
{
	FLog::Log("USelectAction::Enter");
	m_Owner->Surprise();
	bFlow = true;
	bTarget = false;
	bArrive = false;
	m_Owner->IsWorked = true;
}

void UGatherToPlayer::Execute(float dt)
{
	if (bFlow == true)
	{
		FlowTime += dt;

		if (FlowTime > ReadyTime)
		{
			m_Owner->CalmDown();

			bFlow = false;
			bTarget = true;
			Target = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
			return;
		}
	}
	else if (bTarget == true)
	{
		if (m_Owner->AIController)
		{
			FAIMoveRequest MoveRequest;
			MoveRequest.SetGoalActor(Target);
			MoveRequest.SetAcceptanceRadius(300.0f);

			FNavPathSharedPtr NavPath;

			m_Owner->AIController->MoveTo(MoveRequest, &NavPath);

			bTarget = false;
		}
	}
	else if (bArrive == false)
	{
		float distance = FVector::Distance(m_Owner->GetActorLocation(), Target->GetActorLocation());
		bArrive = distance < 320.f ? true : false;
		if (bArrive)
		{
			m_Owner->IsWorked = false;
		}
		else
		{
		}
	}
}

void UGatherToPlayer::Exit()
{
}

void UGatherToPlayer::OnSkillCompleted()
{
}

void UGatherToPlayer::OnSkillCanceled()
{
}
