// Fill out your copyright notice in the Description page of Project Settings.
#include "BaseAiFSM.h"

#include "BaseAiState.h"
#include "Sonheim/AreaObject/Monster/BaseMonster.h"
#include "Sonheim/ResourceManager/SonheimGameType.h"


// Sets default values for this component's properties
UBaseAiFSM::UBaseAiFSM()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UBaseAiFSM::BeginPlay()
{
	Super::BeginPlay();
	m_Owner = Cast<ABaseMonster>(GetOwner()); 
	// ...
}


// Called every frame
void UBaseAiFSM::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateState(DeltaTime);
}

void UBaseAiFSM::AddState(EAiStateType StateType, UBaseAiState* State)
{
	if (true == m_AiStates.Contains(StateType))
	{
		return;
	}
	State->SetAiKind(StateType);
	State->SetAiFSM(this);
	
	m_AiStates.Add(StateType, State);
}

void UBaseAiFSM::ChangeState(EAiStateType StateType)
{
	if (EAiStateType::None == StateType)
	{
		return;
	}
	UBaseAiState** ChangeState = m_AiStates.Find(StateType);
	if (nullptr == ChangeState)
	{
		return;
	}
	// FSM 구동후 첫 상태
	if (nullptr == m_CurrentState)
	{
		m_CurrentState = *ChangeState;
		m_CurrentState->Enter();
		return;
	}
	// 같은 상태로는 전환하지않음 -> 고민중...
	if (*ChangeState == m_CurrentState)
	{
		return;
	}

	m_CurrentState->Exit();

	m_PreviousState = m_CurrentState;

	m_CurrentState = *ChangeState;
	m_CurrentState->Enter();
}

bool UBaseAiFSM::IsExistState(EAiStateType StateType) const
{
	return m_AiStates.Contains(StateType);
}

void UBaseAiFSM::UpdateState(float dt)
{
	if (nullptr == m_CurrentState)
	{
		return;
	}
	m_CurrentState->Execute(dt);
}

void UBaseAiFSM::CheckIsValidAiStates() const
{
	for (const auto& pair : m_AiStates)
	{
		pair.Value->CheckIsValid();
	}
}


