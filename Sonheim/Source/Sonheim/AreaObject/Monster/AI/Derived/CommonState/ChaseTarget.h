// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Sonheim/AreaObject/Monster/AI/Base/BaseAiState.h"
#include "ChaseTarget.generated.h"

/**
 * 
 */
UCLASS()
class SONHEIM_API UChaseTarget : public UBaseAiState
{
	GENERATED_BODY()

public:
	virtual void InitState() override;
	virtual void CheckIsValid() override;
	virtual void Enter() override;
	virtual void Execute(float dt) override;
	virtual void Exit() override;

#pragma region Setter
	void SetSelectedSkillRange(float SelectedSkillRange)
	{
		m_SelectedSkillRange = SelectedSkillRange;
	}

	void SetSkillRangeMargin(float SkillRangeMargin)
	{
		m_SkillRangeMargin = SkillRangeMargin;
	}

	void SetChaseAccelSpeed(float ChaseAccelSpeed)
	{
		m_ChaseAccelSpeed = ChaseAccelSpeed;
	}

	void SetChaseExtendedTime(float ChaseExtendedTime)
	{
		m_ChaseExtendedTime = ChaseExtendedTime;
	}

	void SetChaseExtendedState(EAiStateType ChaseExtendedState)
	{
		m_ChaseExtendedState = ChaseExtendedState;
	}

	void SetMaxChaseDistance(float MaxChaseDistance)
	{
		m_MaxChaseDistance = MaxChaseDistance;
	}

	void SetMaxChaseDistanceState(EAiStateType MaxChaseDistanceState)
	{
		m_MaxChaseDistanceState = MaxChaseDistanceState;
	}
#pragma endregion Setter
private:
	// 스킬 룰렛에 의해 정해진 스킬 사정거리 - enter에서 Set
	float m_SelectedSkillRange = 0.0f;
	// 스킬 사정거리 마진 - 스킬 사정거리보다 해당 변수만큼 더 접근
	float m_SkillRangeMargin = 5.0f;
	// 추적 가속도(ToDo : 최대 이속은 walk, run, sprint mode 전환 구현)
	float m_ChaseAccelSpeed = 500.f;

	// 현재 추적 시간
	float m_CurrentChaseTime = 0.0f;
	// 최대 추적 시간 - 이후 m_ChaseExtendedState 로 전이
	float m_ChaseExtendedTime = 0.0f;
	EAiStateType m_ChaseExtendedState = EAiStateType::None;

	// 최대 추적 거리 - 이후 m_MaxChaseDistanceState 로 전이
	float m_MaxChaseDistance = 0.0f;
	EAiStateType m_MaxChaseDistanceState = EAiStateType::None;
	bool bDebug = false;
};
