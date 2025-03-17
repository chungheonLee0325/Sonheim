// Fill out your copyright notice in the Description page of Project Settings.


#include "PatrolMode.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "Sonheim/AreaObject/Monster/BaseMonster.h"
#include "Sonheim/Utilities/LogMacro.h"

void UPatrolMode::InitState()
{
}

void UPatrolMode::CheckIsValid()
{
}

void UPatrolMode::Enter()
{
	if (m_Owner->bShowDebug)
	{
		FLog::Log("UPatrolMode");
	}
	
	PatrolTime = FMath::RandRange(1.5f, 2.5f);
}

void UPatrolMode::Execute(float dt)
{
	// 데미지 받으면
	if (m_Owner->GetAggroTarget())
	{
		ChangeState(m_NextState);
		return;	
	}

	FlowTime += dt;

	if (FlowTime > PatrolTime)
	{
		FlowTime = 0.f;
		Patrol();
	}
}

void UPatrolMode::Exit()
{
}

void UPatrolMode::Patrol()
{
	UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!NavSystem)
	{
		return;
	}

	if (m_Owner->bShowDebug)
	{
		FLog::Log("Patrol");
	}

	FVector Start = m_Owner->GetActorLocation();
	FNavLocation Next;
	
	NavSystem->GetRandomReachablePointInRadius(Start, 600.f, Next);

	m_Owner->AIController->MoveToLocation(Next.Location);
	
	PatrolTime = FMath::RandRange(5.f, 8.f);
}
