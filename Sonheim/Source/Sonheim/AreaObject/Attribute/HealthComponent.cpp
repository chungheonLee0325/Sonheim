#include "HealthComponent.h"
#include "Net/UnrealNetwork.h"
#include "Sonheim/AreaObject/Base/AreaObject.h"

UHealthComponent::UHealthComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    DOREPLIFETIME(UHealthComponent, m_HP);
    DOREPLIFETIME(UHealthComponent, m_HPMax);
}

void UHealthComponent::InitHealth(float hpMax)
{
    if (GetOwnerRole() != ROLE_Authority)
        return;
        
    m_HPMax = hpMax;
    m_HP = m_HPMax;
    
    // 초기화 시 로컬에서 이벤트 발생
    OnHealthChanged.Broadcast(m_HP, 0, m_HPMax);
}

void UHealthComponent::ModifyHP(float Delta)
{
    if (GetOwnerRole() != ROLE_Authority || FMath::IsNearlyZero(Delta))
        return;
        
    float OldHP = m_HP;
    m_HP = FMath::Clamp(m_HP + Delta, 0.0f, m_HPMax);
    
    // 서버에서만 이벤트 발생, OnRep이 클라이언트 처리
    if (!FMath::IsNearlyEqual(OldHP, m_HP))
    {
        OnHealthChanged.Broadcast(m_HP, m_HP - OldHP, m_HPMax);
    }
}

void UHealthComponent::SetHPByRate(float Rate)
{
    if (GetOwnerRole() != ROLE_Authority)
        return;
        
    float NewHP = m_HPMax * FMath::Clamp(Rate, 0.0f, 1.0f);
    float Delta = NewHP - m_HP;
    
    if (!FMath::IsNearlyZero(Delta))
    {
        m_HP = NewHP;
        OnHealthChanged.Broadcast(m_HP, Delta, m_HPMax);
    }
}

void UHealthComponent::ModifyMaxHP(float Delta)
{
    if (GetOwnerRole() != ROLE_Authority || FMath::IsNearlyZero(Delta))
        return;
        
    m_HPMax = FMath::Max(1.0f, m_HPMax + Delta);
    OnHealthChanged.Broadcast(m_HP, 0, m_HPMax);
}

void UHealthComponent::SetMaxHP(float MaxHP)
{
    if (GetOwnerRole() != ROLE_Authority)
        return;
        
    m_HPMax = FMath::Max(1.0f, MaxHP);
    OnHealthChanged.Broadcast(m_HP, 0, m_HPMax);
}

void UHealthComponent::OnRep_HP(float OldHP)
{
    // 클라이언트에서 HP 변경 알림
    float Delta = m_HP - OldHP;
    OnHealthChanged.Broadcast(m_HP, Delta, m_HPMax);
}

void UHealthComponent::OnRep_HPMax(float OldHPMax)
{
    // 클라이언트에서 최대 HP 변경 알림
    OnHealthChanged.Broadcast(m_HP, 0, m_HPMax);
}