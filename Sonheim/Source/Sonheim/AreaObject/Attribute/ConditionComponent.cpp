#include "ConditionComponent.h"
#include "Net/UnrealNetwork.h"
#include "Sonheim/AreaObject/Base/AreaObject.h"

UConditionComponent::UConditionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UConditionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UConditionComponent, ConditionFlags);
}

void UConditionComponent::AddCondition(EConditionBitsType Condition, float Duration)
{
    // 서버에서만 실행
    if (GetOwnerRole() != ROLE_Authority)
        return;

    // 이미 있는 조건이면 타이머만 갱신
    if (HasCondition(Condition))
    {
        if (Duration > 0.0f)
        {
            // 기존 타이머 제거 후 재설정
            if (FTimerHandle* ExistingTimer = ConditionTimers.Find(Condition))
            {
                GetWorld()->GetTimerManager().ClearTimer(*ExistingTimer);
            }
            
            FTimerHandle& TimerHandle = ConditionTimers.FindOrAdd(Condition);
            FTimerDelegate TimerDelegate;
            TimerDelegate.BindUObject(this, &UConditionComponent::RemoveConditionInternal, Condition);
            
            GetWorld()->GetTimerManager().SetTimer(TimerHandle, TimerDelegate, Duration, false);
        }
        return;
    }

    // 새로운 조건 추가
    ConditionFlags |= static_cast<uint32>(Condition);

    // Duration 타이머 설정
    if (Duration > 0.0f && GetWorld())
    {
        FTimerHandle& TimerHandle = ConditionTimers.FindOrAdd(Condition);
        FTimerDelegate TimerDelegate;
        TimerDelegate.BindUObject(this, &UConditionComponent::RemoveConditionInternal, Condition);
        
        GetWorld()->GetTimerManager().SetTimer(TimerHandle, TimerDelegate, Duration, false);
    }
}

void UConditionComponent::RemoveCondition(EConditionBitsType Condition)
{
    // 서버에서만 실행
    if (GetOwnerRole() != ROLE_Authority)
        return;

    if (!HasCondition(Condition))
        return;

    // 타이머 정리
    if (FTimerHandle* ExistingTimer = ConditionTimers.Find(Condition))
    {
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().ClearTimer(*ExistingTimer);
        }
        ConditionTimers.Remove(Condition);
    }

    // 조건 제거
    ConditionFlags &= ~static_cast<uint32>(Condition);
}

void UConditionComponent::RemoveConditionInternal(EConditionBitsType Condition)
{
    ConditionTimers.Remove(Condition);
    RemoveCondition(Condition);
}

bool UConditionComponent::HasCondition(EConditionBitsType Condition) const
{
    return (ConditionFlags & static_cast<uint32>(Condition)) != 0;
}

void UConditionComponent::ExchangeDead()
{
    if (GetOwnerRole() != ROLE_Authority || HasCondition(EConditionBitsType::Dead))
        return;
        
    AddCondition(EConditionBitsType::Dead);
}

void UConditionComponent::Restart()
{
    if (GetOwnerRole() != ROLE_Authority)
        return;

    // 모든 타이머 정리
    if (UWorld* World = GetWorld())
    {
        for (auto& TimerPair : ConditionTimers)
        {
            World->GetTimerManager().ClearTimer(TimerPair.Value);
        }
    }
    ConditionTimers.Empty();
    
    // 플래그 초기화
    ConditionFlags = 0;
}

void UConditionComponent::OnRep_ConditionFlags(uint32 OldFlags)
{
    // 변경된 조건 찾기
    uint32 ChangedFlags = ConditionFlags ^ OldFlags;
    
    // 추가된 조건 처리
    uint32 AddedFlags = ChangedFlags & ConditionFlags;
    if (AddedFlags != 0)
    {
        // 시각적 효과나 사운드 재생
        // 예: 상태이상 이펙트 시작
    }
    
    // 제거된 조건 처리
    uint32 RemovedFlags = ChangedFlags & OldFlags;
    if (RemovedFlags != 0)
    {
        // 시각적 효과 제거
        // 예: 상태이상 이펙트 종료
    }
}