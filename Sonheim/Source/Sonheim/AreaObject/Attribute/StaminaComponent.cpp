#include "StaminaComponent.h"
#include "Net/UnrealNetwork.h"
#include "Sonheim/AreaObject/Base/AreaObject.h"
#include "Engine/World.h"
#include "TimerManager.h"

UStaminaComponent::UStaminaComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    SetIsReplicatedByDefault(true);
}

void UStaminaComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UStaminaComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    DOREPLIFETIME(UStaminaComponent, m_Stamina);
    DOREPLIFETIME(UStaminaComponent, m_StaminaMax);
    DOREPLIFETIME(UStaminaComponent, m_RecoveryRate);
    DOREPLIFETIME(UStaminaComponent, m_RecoveryDelay);
    DOREPLIFETIME(UStaminaComponent, m_GroggyDuration);
    DOREPLIFETIME(UStaminaComponent, bCanRecover);
}

void UStaminaComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                      FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // 서버에서만 스태미나 회복 처리
    if (GetOwnerRole() != ROLE_Authority)
        return;

    // 자동 회복
    if (bCanRecover && m_Stamina < m_StaminaMax)
    {
        ModifyStamina(m_RecoveryRate * DeltaTime);
    }
}

void UStaminaComponent::InitStamina(float StaminaMax, float RecoveryRate, float GroggyDuration)
{
    // 서버에서만 실행
    if (GetOwnerRole() != ROLE_Authority)
        return;

    m_StaminaMax = StaminaMax;
    m_Stamina = m_StaminaMax;
    m_RecoveryRate = RecoveryRate;
    m_GroggyDuration = GroggyDuration;
    
    // 로컬 이벤트
    OnStaminaChanged.Broadcast(m_Stamina, 0, m_StaminaMax);
}

void UStaminaComponent::ModifyStamina(float Delta, bool bIsDamaged)
{
    // 서버에서만 실행
    if (GetOwnerRole() != ROLE_Authority || FMath::IsNearlyZero(Delta))
        return;

    float OldStamina = m_Stamina;
    m_Stamina = FMath::Clamp(m_Stamina + Delta, 0.0f, m_StaminaMax);

    // 변경사항이 있을 때만 처리
    if (FMath::IsNearlyEqual(OldStamina, m_Stamina))
        return;

    // 스태미나 감소 시 회복 중단
    if (Delta < 0.0f)
    {
        StopStaminaRecovery();
        
        // 회복 재시작 타이머
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().ClearTimer(RecoveryDelayHandle);
            World->GetTimerManager().SetTimer(
                RecoveryDelayHandle,
                this,
                &UStaminaComponent::StartStaminaRecovery,
                m_RecoveryDelay,
                false
            );
        }
    }

    // 그로기 상태 체크
    if (bIsDamaged && FMath::IsNearlyZero(m_Stamina))
    {
        ApplyGroggyState();
    }

    // 서버 이벤트
    OnStaminaChanged.Broadcast(m_Stamina, m_Stamina - OldStamina, m_StaminaMax);
}

void UStaminaComponent::SetStaminaByRate(float Rate)
{
    if (GetOwnerRole() != ROLE_Authority)
        return;

    float NewStamina = m_StaminaMax * FMath::Clamp(Rate, 0.0f, 1.0f);
    float Delta = NewStamina - m_Stamina;
    
    if (!FMath::IsNearlyZero(Delta))
    {
        m_Stamina = NewStamina;
        OnStaminaChanged.Broadcast(m_Stamina, Delta, m_StaminaMax);
    }
}

void UStaminaComponent::StartStaminaRecovery()
{
    if (GetOwnerRole() != ROLE_Authority)
        return;

    bCanRecover = true;
}

void UStaminaComponent::StopStaminaRecovery()
{
    if (GetOwnerRole() != ROLE_Authority)
        return;

    bCanRecover = false;
    LastDamageTime = GetWorld()->GetTimeSeconds();
}

void UStaminaComponent::ModifyMaxStamina(float Delta)
{
    if (GetOwnerRole() != ROLE_Authority || FMath::IsNearlyZero(Delta))
        return;

    float OldMax = m_StaminaMax;
    m_StaminaMax = FMath::Max(1.0f, m_StaminaMax + Delta);
    
    // 현재 스태미나가 최대치를 초과하지 않도록
    if (m_Stamina > m_StaminaMax)
    {
        m_Stamina = m_StaminaMax;
    }
    
    OnStaminaChanged.Broadcast(m_Stamina, 0, m_StaminaMax);
}

void UStaminaComponent::SetMaxStamina(float NewMax)
{
    if (GetOwnerRole() != ROLE_Authority)
        return;

    m_StaminaMax = FMath::Max(1.0f, NewMax);
    
    // 현재 스태미나 조정
    if (m_Stamina > m_StaminaMax)
    {
        m_Stamina = m_StaminaMax;
    }
    
    OnStaminaChanged.Broadcast(m_Stamina, 0, m_StaminaMax);
}

void UStaminaComponent::ApplyGroggyState()
{
    // 그로기 이벤트 발생
    OnApplyGroggyDelegate.Broadcast(m_GroggyDuration);
    
    // 타이머 정리
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(RecoveryDelayHandle);
    }
    
    // 스태미나 즉시 회복 (그로기 후)
    m_Stamina = m_StaminaMax;
    bCanRecover = true;
    
    // 오너에게 그로기 상태 적용
    if (AAreaObject* Owner = Cast<AAreaObject>(GetOwner()))
    {
        //Owner->AddCondition(EConditionBitsType::Groggy, m_GroggyDuration);
    }
    
    OnStaminaChanged.Broadcast(m_Stamina, m_StaminaMax, m_StaminaMax);
}

void UStaminaComponent::OnRep_Stamina(float OldStamina)
{
    // 클라이언트에서 스태미나 변경 감지
    float Delta = m_Stamina - OldStamina;
    OnStaminaChanged.Broadcast(m_Stamina, Delta, m_StaminaMax);
    
    // 스태미나 소모 시각 효과
    if (Delta < 0 && GetOwner())
    {
        if (AAreaObject* Owner = Cast<AAreaObject>(GetOwner()))
        {
            if (Owner->IsLocallyControlled())
            {
                // 스태미나 소모 UI 피드백
                // 예: 스태미나 바 깜빡임, 경고음 등
            }
        }
    }
}

void UStaminaComponent::OnRep_StaminaMax(float OldStaminaMax)
{
    // 최대 스태미나 변경 시 UI 업데이트
    OnStaminaChanged.Broadcast(m_Stamina, 0, m_StaminaMax);
}

void UStaminaComponent::OnRep_CanRecover()
{
    // 회복 상태 변경 시 시각적 피드백
    if (GetOwner())
    {
        if (AAreaObject* Owner = Cast<AAreaObject>(GetOwner()))
        {
            if (Owner->IsLocallyControlled())
            {
                if (bCanRecover)
                {
                    // 회복 시작 효과
                    // 예: 스태미나 바 색상 변경, 회복 파티클 등
                }
                else
                {
                    // 회복 중단 효과
                    // 예: 스태미나 바 빨간색 표시 등
                }
            }
        }
    }
}