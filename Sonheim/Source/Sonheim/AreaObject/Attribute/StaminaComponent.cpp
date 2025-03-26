#include "StaminaComponent.h"
#include "Net/UnrealNetwork.h"
#include "Sonheim/AreaObject/Base/AreaObject.h"

UStaminaComponent::UStaminaComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	
	// 컴포넌트 리플리케이션 활성화
	SetIsReplicatedByDefault(true);
}

void UStaminaComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UStaminaComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	// 스태미나 값들 복제
	DOREPLIFETIME(UStaminaComponent, m_Stamina);
	DOREPLIFETIME(UStaminaComponent, m_StaminaMax);
	DOREPLIFETIME(UStaminaComponent, m_RecoveryRate);
	DOREPLIFETIME(UStaminaComponent, m_RecoveryDelay);
	DOREPLIFETIME(UStaminaComponent, bCanRecover);
	DOREPLIFETIME(UStaminaComponent, m_GroggyDuration);
}

void UStaminaComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                      FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 서버에서만 스태미나 회복 처리
	if (GetOwnerRole() == ROLE_Authority)
	{
		// 스태미나 자동 회복
		if (bCanRecover && m_Stamina < m_StaminaMax)
		{
			float recovery = m_RecoveryRate * DeltaTime;

			IncreaseStamina(recovery);
		}
	}
}

void UStaminaComponent::InitStamina_Implementation(float StaminaMax, float RecoveryRate, float GroggyDuration)
{
	m_StaminaMax = StaminaMax;
	m_Stamina = m_StaminaMax;
	m_RecoveryRate = RecoveryRate;
	m_GroggyDuration = GroggyDuration;
	OnStaminaChanged.Broadcast(m_Stamina, 0, m_StaminaMax);
	
	// 클라이언트에게 변경 사항 알림
	AAreaObject* Owner = Cast<AAreaObject>(GetOwner());
	if (Owner && Owner->GetLocalRole() == ROLE_Authority)
	{
		Client_OnStaminaChanged(m_Stamina, 0, m_StaminaMax);
	}
}

void UStaminaComponent::DecreaseStamina_Implementation(float Delta, bool bIsDamaged)
{
	if (Delta <= 0.0f) return;

	// 스태미나 감소
	float oldStamina = m_Stamina;
	m_Stamina = FMath::Clamp(m_Stamina - Delta, 0.0f, m_StaminaMax);

	if (!FMath::IsNearlyEqual(oldStamina, m_Stamina))
	{
		OnStaminaChanged.Broadcast(m_Stamina, -(oldStamina - m_Stamina), m_StaminaMax);
		
		// 클라이언트에게 변경 사항 알림
		AAreaObject* Owner = Cast<AAreaObject>(GetOwner());
		if (Owner && Owner->GetLocalRole() == ROLE_Authority)
		{
			Client_OnStaminaChanged(m_Stamina, -(oldStamina - m_Stamina), m_StaminaMax);
		}

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
		
		// 클라이언트에게 변경 사항 알림
		AAreaObject* Owner = Cast<AAreaObject>(GetOwner());
		if (Owner && Owner->GetLocalRole() == ROLE_Authority)
		{
			Client_OnStaminaChanged(m_Stamina, m_StaminaMax, m_StaminaMax);
		}
	}
}

void UStaminaComponent::IncreaseStamina_Implementation(float Delta)
{
	if (Delta <= 0.0f) return;

	float oldStamina = m_Stamina;
	m_Stamina = FMath::Clamp(m_Stamina + Delta, 0.0f, m_StaminaMax);

	if (!FMath::IsNearlyEqual(oldStamina, m_Stamina))
	{
		OnStaminaChanged.Broadcast(m_Stamina, m_Stamina - oldStamina, m_StaminaMax);
		
		// 클라이언트에게 변경 사항 알림
		AAreaObject* Owner = Cast<AAreaObject>(GetOwner());
		if (Owner && Owner->GetLocalRole() == ROLE_Authority)
		{
			Client_OnStaminaChanged(m_Stamina, m_Stamina - oldStamina, m_StaminaMax);
		}
	}
}

void UStaminaComponent::StartStaminaRecovery_Implementation()
{
	bCanRecover = true;
	
	// 클라이언트에게 변경 사항 알림은 OnRep_CanRecover에서 처리
}

void UStaminaComponent::StopStaminaRecovery_Implementation()
{
	bCanRecover = false;
	
	// 클라이언트에게 변경 사항 알림은 OnRep_CanRecover에서 처리
}

void UStaminaComponent::Client_OnStaminaChanged_Implementation(float CurrentStamina, float Delta, float MaxStamina)
{
	// 클라이언트에서 호출되는 함수
	OnStaminaChanged.Broadcast(CurrentStamina, Delta, MaxStamina);
}

void UStaminaComponent::OnRep_Stamina()
{
	// 스태미나가 복제되었을 때 클라이언트에서 호출됨
	OnStaminaChanged.Broadcast(m_Stamina, 0, m_StaminaMax);
}

void UStaminaComponent::OnRep_StaminaMax()
{
	// 최대 스태미나가 복제되었을 때 클라이언트에서 호출됨
	OnStaminaChanged.Broadcast(m_Stamina, 0, m_StaminaMax);
}

void UStaminaComponent::OnRep_CanRecover()
{
	// 회복 상태가 변경되었을 때 필요한 클라이언트 로직
	// 예: 회복 상태에 따른 UI 업데이트 등
}

void UStaminaComponent::OnRep_GroggyDuration()
{
	// 그로기 지속시간이 변경되었을 때 필요한 클라이언트 로직
}
