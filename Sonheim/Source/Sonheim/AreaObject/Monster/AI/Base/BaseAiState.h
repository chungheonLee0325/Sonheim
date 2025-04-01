// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Sonheim/ResourceManager/SonheimGameType.h"
#include "BaseAiState.generated.h"

class ABaseMonster;
class UBaseAiFSM;

/**
 * 
 */
UCLASS(Abstract)
class SONHEIM_API UBaseAiState : public UObject
{
	GENERATED_BODY()

public:
	// Create State - Base (Factory Pattern)
	void SetAiFSM(UBaseAiFSM* AiFSM);
	void SetAiKind(EAiStateType StateType);
	void SetOwner(ABaseMonster* Owner);
	virtual void InitState() PURE_VIRTUAL(UBaseAiState::InitState);

	// Create State - Special Initialize (Builder Pattern)
	virtual void CheckIsValid() /*PURE_VIRTUAL(UBaseAiState::InitState)*/;
	virtual void SetNextState(EAiStateType NextState);
	void SetSuccessState(EAiStateType SuccessState);
	void SetFailState(EAiStateType FailState);

	// State 기본 동작
	virtual void Enter();
	virtual void Execute(float dt);
	virtual void Exit();

	// 서버 전용 State 동작 함수
	virtual void ServerEnter();
	virtual void ServerExecute(float dt);
	virtual void ServerExit();
	
	// 클라 전용 State 동작 함수
	virtual void ClientEnter();
	virtual void ClientExecute(float dt);
	virtual void ClientExit();
	
	// 서버인지 확인하는 함수
	virtual bool IsServer();
	
	// Getter
	EAiStateType AiStateType() const;

	// Interface
	void ChangeState(EAiStateType NewState) const;

	// 
protected:
	EAiStateType m_AiStateType;

	UPROPERTY()
	UBaseAiFSM* m_AiFSM;

	UPROPERTY(Replicated)
	ABaseMonster* m_Owner;

	UPROPERTY()
	// 기본 전이 State
	EAiStateType m_NextState;
	// 성공 후 전이 State
	UPROPERTY()
	EAiStateType m_SuccessState;
	// 실패 후 전이 State
	UPROPERTY()
	EAiStateType m_FailState;

public:
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
};
