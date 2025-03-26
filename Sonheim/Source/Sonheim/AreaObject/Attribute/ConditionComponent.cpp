// Fill out your copyright notice in the Description page of Project Settings.


#include "ConditionComponent.h"
#include "Net/UnrealNetwork.h"
#include "Sonheim/AreaObject/Base/AreaObject.h"
#include "Sonheim/Utilities/LogMacro.h"


// Sets default values for this component's properties
UConditionComponent::UConditionComponent(): ConditionFlags(0)
{
	PrimaryComponentTick.bCanEverTick = false;

	// 컴포넌트 리플리케이션 활성화
	SetIsReplicatedByDefault(true);
}

void UConditionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 컨디션 플래그 복제
	DOREPLIFETIME(UConditionComponent, ConditionFlags);
}

bool UConditionComponent::AddCondition_Validate(EConditionBitsType Condition, float Duration)
{
	return true; // 추가적인 검증 로직이 필요하면 여기 추가
}

void UConditionComponent::AddCondition_Implementation(EConditionBitsType Condition, float Duration)
{
	// 권한 확인
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}

	// Condition 적용
	bool bApplied = _addCondition(Condition);

	// Duration이 유효한 경우 타이머 설정
	if (Duration > 0.0f && bApplied)
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

	// 클라이언트에게 변경 사항 알림
	if (bApplied)
	{
		AAreaObject* Owner = Cast<AAreaObject>(GetOwner());
		if (Owner && Owner->GetLocalRole() == ROLE_Authority)
		{
			Client_OnConditionChanged(ConditionFlags);
		}
	}

	return;
}

void UConditionComponent::RemoveConditionInternal(EConditionBitsType Condition)
{
	// 타이머 제거
	ConditionTimers.Remove(Condition);

	// Condition 제거
	RemoveCondition(Condition);
}

bool UConditionComponent::RemoveCondition_Validate(EConditionBitsType Condition)
{
	return true; // 추가적인 검증 로직이 필요하면 여기 추가
}

void UConditionComponent::RemoveCondition_Implementation(EConditionBitsType Condition)
{
	// 권한 확인
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}

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

	// 클라이언트에게 변경 사항 알림
	if (bApplied)
	{
		AAreaObject* Owner = Cast<AAreaObject>(GetOwner());
		if (Owner && Owner->GetLocalRole() == ROLE_Authority)
		{
			Client_OnConditionChanged(ConditionFlags);
		}
	}

	return;
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

bool UConditionComponent::ExchangeDead_Validate()
{
	return true;
}

void UConditionComponent::ExchangeDead_Implementation()
{
	// 권한 확인
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}

	if (HasCondition(EConditionBitsType::Dead))
		return;
	AddCondition(EConditionBitsType::Dead);
}

void UConditionComponent::Restart_Implementation()
{
	// 권한 확인
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}

	ConditionFlags = 0;

	// 클라이언트에게 변경 사항 알림
	AAreaObject* Owner = Cast<AAreaObject>(GetOwner());
	if (Owner && Owner->GetLocalRole() == ROLE_Authority)
	{
		Client_OnConditionChanged(ConditionFlags);
	}
}

void UConditionComponent::OnRep_ConditionFlags()
{
	// 클라이언트에서 컨디션 플래그가 변경되었을 때 필요한 처리
	// 특별한 시각 효과나 사운드 등을 여기서 처리할 수 있음
}

void UConditionComponent::Client_OnConditionChanged_Implementation(uint32 NewConditionFlags)
{
	// 클라이언트측에서 컨디션 변경 이벤트 처리
	// 시각적 효과나 UI 업데이트 등
}
