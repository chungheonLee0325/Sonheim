#include "StaminaComponent.h"

UStaminaComponent::UStaminaComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UStaminaComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UStaminaComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                      FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 스태미나 자동 회복
	if (bCanRecover && m_Stamina < m_StaminaMax)
	{
		float recovery = m_RecoveryRate * DeltaTime;

		// 가드 중일 때 회복률 감소
		if (bIsGuarding)
		{
			recovery *= m_GuardRecoveryRateMultiplier;
		}

		IncreaseStamina(recovery);
	}
}

void UStaminaComponent::InitStamina(float StaminaMax, float RecoveryRate, float GroggyDuration)
{
	m_StaminaMax = StaminaMax;
	m_Stamina = m_StaminaMax;
	m_RecoveryRate = RecoveryRate;
	m_GroggyDuration = GroggyDuration;
	OnStaminaChanged.Broadcast(m_Stamina, 0, m_StaminaMax);
}

float UStaminaComponent::DecreaseStamina(float Delta, bool bIsDamaged)
{
	if (Delta <= 0.0f) return m_Stamina;

	// 스태미나 감소
	float oldStamina = m_Stamina;
	m_Stamina = FMath::Clamp(m_Stamina - Delta, 0.0f, m_StaminaMax);

	if (!FMath::IsNearlyEqual(oldStamina, m_Stamina))
	{
		OnStaminaChanged.Broadcast(m_Stamina, -(oldStamina - m_Stamina), m_StaminaMax);

		// 회복 중지 및 딜레이 타이머 시작
		StopStaminaRecovery();
		GetWorld()->GetTimerManager().SetTimer(
			RecoveryDelayHandle,
			this,
			&UStaminaComponent::StartStaminaRecovery,
			m_RecoveryDelay,
			false
		);
	}
	if (bIsDamaged && FMath::IsNearlyZero(m_Stamina))
	{
		// ToDo : 수정 예정
		OnApplyGroggyDelegate.Broadcast(m_GroggyDuration);
		OnStaminaChanged.Broadcast(m_Stamina, 0, m_StaminaMax);
		GetWorld()->GetTimerManager().ClearTimer(RecoveryDelayHandle);
		m_Stamina = m_StaminaMax;
	}
	return m_Stamina;
}

float UStaminaComponent::IncreaseStamina(float Delta)
{
	if (Delta <= 0.0f) return m_Stamina;

	float oldStamina = m_Stamina;
	m_Stamina = FMath::Clamp(m_Stamina + Delta, 0.0f, m_StaminaMax);

	if (!FMath::IsNearlyEqual(oldStamina, m_Stamina))
	{
		OnStaminaChanged.Broadcast(m_Stamina, m_Stamina - oldStamina, m_StaminaMax);
	}

	return m_Stamina;
}

void UStaminaComponent::StartStaminaRecovery()
{
	bCanRecover = true;
}

void UStaminaComponent::StopStaminaRecovery()
{
	bCanRecover = false;
}

void UStaminaComponent::SetGuardState(bool IsGuarding)
{
	this->bIsGuarding = IsGuarding;
}
