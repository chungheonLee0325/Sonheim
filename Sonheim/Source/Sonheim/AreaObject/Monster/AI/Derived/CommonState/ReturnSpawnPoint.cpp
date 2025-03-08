// Fill out your copyright notice in the Description page of Project Settings.


#include "ReturnSpawnPoint.h"

#include "Sonheim/AreaObject/Monster/BaseMonster.h"

void UReturnSpawnPoint::InitState()
{
}

void UReturnSpawnPoint::CheckIsValid()
{
	if (m_SuccessState == EAiStateType::None) LOG_PRINT(TEXT("Please Set m_SuccessState"));
	if (m_FailState == EAiStateType::None) LOG_PRINT(TEXT("Please Set m_FailState"));
}

void UReturnSpawnPoint::Enter()
{
	// 어그로 초기화
	m_Owner->SetAggroTarget(nullptr);

	// 복귀 상태 적용
	// ToDo : 고민해보자.. 스태미너, 체력등 복구해야하는가
}

void UReturnSpawnPoint::Execute(float dt)
{
	// 어그로 타겟이 다시 잡힌다면 Fail state 전환
	if (m_Owner->GetAggroTarget() != nullptr)
	{
		ChangeState(m_FailState);
		return;
	}

	FVector dir = m_Owner->GetSpawnLocation() - m_Owner->GetActorLocation();
	// 도착하면 Success State 전이
	if (FMath::IsNearlyZero(dir.Length(), 5.f))
	{
		ChangeState(m_SuccessState);
		return;
	}
	// 일반 복귀 로직
	dir.Normalize();
	m_Owner->AddMovementInput(dir * m_ReturnAccelSpeed, 1.0f);
}

void UReturnSpawnPoint::Exit()
{
}
