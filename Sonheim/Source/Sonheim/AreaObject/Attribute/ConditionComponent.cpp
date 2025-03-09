// Fill out your copyright notice in the Description page of Project Settings.


#include "ConditionComponent.h"


// Sets default values for this component's properties
UConditionComponent::UConditionComponent(): ConditionFlags(0)
{
	PrimaryComponentTick.bCanEverTick = false;
}


#include "Sonheim/Utilities/LogMacro.h"

bool UConditionComponent::AddCondition(EConditionBitsType Condition, float Duration)
{
	// Condition 적용
	bool bApplied = _addCondition(Condition);

	// Debug
	// if (bApplied) 	LOG_SCREEN("%d : Condition 적용",Condition);
	
	// Duration이 유효한 경우 타이머 설정
	if (Duration > 0.0f)
	{
		if (UWorld* World = GetWorld())
		{
			// 기존 타이머가 있다면 제거
			if (FTimerHandle* ExistingTimer = ConditionTimers.Find(Condition))
			{
				World->GetTimerManager().ClearTimer(*ExistingTimer);
			}

			FTimerHandle& TimerHandle = ConditionTimers.FindOrAdd(Condition);

			FTimerDelegate TimerDelegate;
			TimerDelegate.BindUObject(this, &UConditionComponent::RemoveConditionInternal, Condition);

			World->GetTimerManager().SetTimer(
				TimerHandle,
				TimerDelegate,
				Duration,
				false
			);
		}
	}

	return bApplied;
}

void UConditionComponent::RemoveConditionInternal(EConditionBitsType Condition)
{
	// 타이머 제거
	ConditionTimers.Remove(Condition);

	// Condition 제거
	RemoveCondition(Condition);
}

bool UConditionComponent::RemoveCondition(EConditionBitsType Condition)
{
	// 기존 타이머가 있다면 제거
	if (FTimerHandle* ExistingTimer = ConditionTimers.Find(Condition))
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(*ExistingTimer);
		}
		ConditionTimers.Remove(Condition);
	}
	bool bApplied = _removeCondition(Condition);
	// Debug
	// if (bApplied) 	LOG_SCREEN("%d : Condition 해제",Condition);
	return bApplied;
}

bool UConditionComponent::_addCondition(EConditionBitsType Condition)
{
	if (HasCondition(Condition))
		return false;
	
	ConditionFlags |= static_cast<uint32>(Condition);
	return true;
}

bool UConditionComponent::_removeCondition(EConditionBitsType Condition)
{
	if (!HasCondition(Condition))
		return false;
	
	ConditionFlags &= ~static_cast<uint32>(Condition);
	return true;
}

bool UConditionComponent::HasCondition(EConditionBitsType Condition) const
{
	//상태 활성화 확인
	return (((ConditionFlags) & (static_cast<uint32>(Condition))) != 0);
}

//dead로 전환
bool UConditionComponent::ExchangeDead()
{
	if (HasCondition(EConditionBitsType::Dead))
		return false;

	return AddCondition(EConditionBitsType::Dead);
}

//상태 초기화
void UConditionComponent::Restart()
{
	ConditionFlags = 0;
}
