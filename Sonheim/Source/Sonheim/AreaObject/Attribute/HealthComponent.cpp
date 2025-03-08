// Fill out your copyright notice in the Description page of Project Settings.


#include "HealthComponent.h"

#include "VectorUtil.h"
#include "Sonheim/AreaObject/Base/AreaObject.h"


// Sets default values for this component's properties
UHealthComponent::UHealthComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
	// ...
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

void UHealthComponent::InitHealth(float _hpMax) // 추후 사용에따라 구조체나 다른 형식으로 변환 예정
{
	m_HPMax = _hpMax;
	m_HP = m_HPMax;
	this->OnHealthChanged.Broadcast(m_HP, 0, m_HPMax);
}

float UHealthComponent::IncreaseHP(float Delta)
{
	if (Delta <= 0.0f) return m_HP;

	float oldHP = m_HP;
	m_HP = FMath::Clamp(m_HP + Delta, 0.0f, m_HPMax);

	if (!FMath::IsNearlyEqual(oldHP, m_HP))
	{
		OnHealthChanged.Broadcast(m_HP, m_HP - oldHP, m_HPMax);
	}

	return m_HP;
}

float UHealthComponent::DecreaseHP(float Delta)
{
	if (Delta <= 0.0f) return m_HP;

	float oldHP = m_HP;
	m_HP = FMath::Clamp(m_HP - Delta, 0.0f, m_HPMax);

	if (!FMath::IsNearlyEqual(oldHP, m_HP))
	{
		OnHealthChanged.Broadcast(m_HP, -(oldHP - m_HP), m_HPMax);
	}

	return m_HP;
}

void UHealthComponent::SetHPByRate(float Rate)
{
	float oldHP = m_HP;
	m_HP = m_HPMax * Rate;
	if (false == FMath::IsNearlyEqual((oldHP), (m_HP)))
	{
		this->OnHealthChanged.Broadcast(m_HP, m_HP - oldHP, m_HPMax);
	}
}

float UHealthComponent::GetHP() const
{
	return m_HP;
}

float UHealthComponent::GetMaxHP() const
{
	return m_HPMax;
}
