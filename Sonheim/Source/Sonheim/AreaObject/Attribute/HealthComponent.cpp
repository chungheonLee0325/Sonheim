// Fill out your copyright notice in the Description page of Project Settings.


#include "HealthComponent.h"
#include "Net/UnrealNetwork.h"
#include "VectorUtil.h"
#include "Sonheim/AreaObject/Base/AreaObject.h"


// Sets default values for this component's properties
UHealthComponent::UHealthComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
	
	// 컴포넌트 리플리케이션 활성화
	SetIsReplicatedByDefault(true);
}


// Called when the game starts
void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
}


// Called every frame
void UHealthComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	// HP 값을 모든 클라이언트에 복제
	DOREPLIFETIME(UHealthComponent, m_HP);
	DOREPLIFETIME(UHealthComponent, m_HPMax);
}

void UHealthComponent::InitHealth_Implementation(float _hpMax)
{
	m_HPMax = _hpMax;
	m_HP = m_HPMax;
	OnHealthChanged.Broadcast(m_HP, 0, m_HPMax);
}

void UHealthComponent::IncreaseHP_Implementation(float Delta)
{
	if (Delta <= 0.0f) return;

	float oldHP = m_HP;
	m_HP = FMath::Clamp(m_HP + Delta, 0.0f, m_HPMax);

	if (!FMath::IsNearlyEqual(oldHP, m_HP))
	{
		OnHealthChanged.Broadcast(m_HP, m_HP - oldHP, m_HPMax);
		
		// 클라이언트에게 변경 사항 알림
		AAreaObject* Owner = Cast<AAreaObject>(GetOwner());
		if (Owner && Owner->GetLocalRole() == ROLE_Authority)
		{
			Client_OnHealthChanged(m_HP, m_HP - oldHP, m_HPMax);
		}
	}

	return;
}

void UHealthComponent::DecreaseHP_Implementation(float Delta)
{
	if (Delta <= 0.0f) return;

	float oldHP = m_HP;
	m_HP = FMath::Clamp(m_HP - Delta, 0.0f, m_HPMax);

	if (!FMath::IsNearlyEqual(oldHP, m_HP))
	{
		OnHealthChanged.Broadcast(m_HP, -(oldHP - m_HP), m_HPMax);
		
		// 클라이언트에게 변경 사항 알림
		AAreaObject* Owner = Cast<AAreaObject>(GetOwner());
		if (Owner && Owner->GetLocalRole() == ROLE_Authority)
		{
			Client_OnHealthChanged(m_HP, -(oldHP - m_HP), m_HPMax);
		}
	}

	return;
}

void UHealthComponent::SetHPByRate_Implementation(float Rate)
{
	float oldHP = m_HP;
	m_HP = m_HPMax * Rate;
	if (false == FMath::IsNearlyEqual((oldHP), (m_HP)))
	{
		OnHealthChanged.Broadcast(m_HP, m_HP - oldHP, m_HPMax);
		
		// 클라이언트에게 변경 사항 알림
		AAreaObject* Owner = Cast<AAreaObject>(GetOwner());
		if (Owner && Owner->GetLocalRole() == ROLE_Authority)
		{
			Client_OnHealthChanged(m_HP, m_HP - oldHP, m_HPMax);
		}
	}
}

float UHealthComponent::GetHP()
{
	// 클라이언트에게 변경 사항 알림
	AAreaObject* Owner = Cast<AAreaObject>(GetOwner());
	if (Owner && Owner->GetLocalRole() == ROLE_Authority)
	{
		Client_OnHealthChanged(m_HP, 0.f, m_HPMax);
	}
	return m_HP;
}

float UHealthComponent::GetMaxHP()
{
	// 클라이언트에게 변경 사항 알림
	AAreaObject* Owner = Cast<AAreaObject>(GetOwner());
	if (Owner && Owner->GetLocalRole() == ROLE_Authority)
	{
		Client_OnHealthChanged(m_HP, 0.f, m_HPMax);
	}
	return m_HPMax;
}

void UHealthComponent::AddMaxHP_Implementation(float Delta)
{
	float oldHPMax = m_HPMax;
	m_HPMax = FMath::Clamp(m_HPMax + Delta, 1.0f, 99999);

	if (!FMath::IsNearlyEqual(oldHPMax, m_HPMax))
	{
		OnHealthChanged.Broadcast(m_HP, 0.f, m_HPMax);
		
		// 클라이언트에게 변경 사항 알림
		AAreaObject* Owner = Cast<AAreaObject>(GetOwner());
		if (Owner && Owner->GetLocalRole() == ROLE_Authority)
		{
			Client_OnHealthChanged(m_HP, 0.f, m_HPMax);
		}
	}

	return;
}

void UHealthComponent::SetMaxHP_Implementation(float MaxHP)
{
	m_HPMax = MaxHP;
	OnHealthChanged.Broadcast(m_HP, 0.f, m_HPMax);
	
	// 클라이언트에게 변경 사항 알림
	AAreaObject* Owner = Cast<AAreaObject>(GetOwner());
	if (Owner && Owner->GetLocalRole() == ROLE_Authority)
	{
		Client_OnHealthChanged(m_HP, 0.f, m_HPMax);
	}
	
	return;
}

void UHealthComponent::OnRep_HP()
{
	// HP가 복제되었을 때 클라이언트에서 호출됨
	OnHealthChanged.Broadcast(m_HP, 0, m_HPMax);
}

void UHealthComponent::OnRep_HPMax()
{
	// HPMax가 복제되었을 때 클라이언트에서 호출됨
	OnHealthChanged.Broadcast(m_HP, 0, m_HPMax);
}

void UHealthComponent::Client_OnHealthChanged_Implementation(float CurrentHP, float Delta, float MaxHP)
{
	// 클라이언트에서 호출되는 함수
	OnHealthChanged.Broadcast(CurrentHP, Delta, MaxHP);
}
