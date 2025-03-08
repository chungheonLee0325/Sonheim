// Fill out your copyright notice in the Description page of Project Settings.


#include "AggroWait.h"

#include "GameFramework/Character.h"
#include "Sonheim/AreaObject/Base/AreaObject.h"
#include "Sonheim/AreaObject/Monster/BaseMonster.h"
#include "Sonheim/AreaObject/Monster/AI/Base/BaseAiFSM.h"
#include "Kismet/GameplayStatics.h"


void UAggroWait::InitState()
{
}

void UAggroWait::CheckIsValid()
{
	if (m_NextState == EAiStateType::None) LOG_PRINT(TEXT("Please Set m_NextState"));
}

void UAggroWait::Enter()
{
}

void UAggroWait::Execute(float dt)
{
	if (m_Owner == nullptr || m_Owner->GetAggroTarget())
		return;

	ChangeState(m_NextState);

	// ToDo : 순찰 기능 추가
}

void UAggroWait::Exit()
{
}
