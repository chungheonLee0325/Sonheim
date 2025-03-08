// Fill out your copyright notice in the Description page of Project Settings.

#include "BaseAiState.h"

#include "Sonheim/AreaObject/Monster/BaseMonster.h"
#include "BaseAiFSM.h"


void UBaseAiState::SetAiFSM(UBaseAiFSM* AiFSM)
{
	m_AiFSM = AiFSM;
}

void UBaseAiState::SetAiKind(EAiStateType StateType)
{
	m_AiStateType = StateType;
}

EAiStateType UBaseAiState::AiStateType() const
{
	return m_AiStateType;
}

void UBaseAiState::ChangeState(EAiStateType NewState) const
{
	m_AiFSM->ChangeState(NewState);
}

void UBaseAiState::SetOwner(ABaseMonster* Owner)
{
	m_Owner = Owner;
}

void UBaseAiState::CheckIsValid()
{
}

void UBaseAiState::SetNextState(EAiStateType NextState)
{
	m_NextState = NextState;
}

void UBaseAiState::SetSuccessState(EAiStateType SuccessState)
{
	m_SuccessState = SuccessState;
}

void UBaseAiState::SetFailState(EAiStateType FailState)
{
	m_FailState = FailState;
}
