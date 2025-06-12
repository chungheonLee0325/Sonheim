// StaminaComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StaminaComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnStaminaChangedDelegate, float, CurrentStamina, float, Delta, float, MaxStamina);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnApplyGroggyDelegate, float, Duration);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SONHEIM_API UStaminaComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStaminaComponent();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
                               FActorComponentTickFunction* ThisTickFunction) override;

public:
    // 리플리케이션 설정
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    
    // 초기화 - 서버에서만 호출
    void InitStamina(float StaminaMax, float RecoveryRate, float GroggyDuration);

    // 스태미나 수정 - 권한 체크 내부 처리
    void ModifyStamina(float Delta, bool bIsDamaged = false);
    
    // 스태미나 비율로 설정
    void SetStaminaByRate(float Rate);

    // 스태미나 회복 제어
    void StartStaminaRecovery();
    void StopStaminaRecovery();

    // Getter
    UFUNCTION(BlueprintCallable, Category = "Stamina")
    float GetStamina() const { return m_Stamina; }

    UFUNCTION(BlueprintCallable, Category = "Stamina")
    float GetMaxStamina() const { return m_StaminaMax; }

    UFUNCTION(BlueprintCallable, Category = "Stamina")
    float GetStaminaPercent() const { return (m_StaminaMax > 0) ? (m_Stamina / m_StaminaMax) : 0.0f; }

    UFUNCTION(BlueprintCallable, Category = "Stamina")
    bool CanUseStamina(float Cost) const { return m_Stamina >= Cost; }

    UFUNCTION(BlueprintCallable, Category = "Stamina")
    bool IsRecovering() const { return bCanRecover; }

    // 최대 스태미나 수정
    void ModifyMaxStamina(float Delta);
    void SetMaxStamina(float NewMax);

    // 델리게이트
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnStaminaChangedDelegate OnStaminaChanged;
    
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnApplyGroggyDelegate OnApplyGroggyDelegate;

protected:
    // OnRep 함수들
    UFUNCTION()
    void OnRep_Stamina(float OldStamina);
    
    UFUNCTION()
    void OnRep_StaminaMax(float OldStaminaMax);
    
    UFUNCTION()
    void OnRep_CanRecover();

private:
    // 그로기 처리
    void ApplyGroggyState();

private:
    // Replicated 변수들
    UPROPERTY(ReplicatedUsing = OnRep_Stamina)
    float m_Stamina = 100.0f;
    
    UPROPERTY(ReplicatedUsing = OnRep_StaminaMax)
    float m_StaminaMax = 100.0f;

    UPROPERTY(Replicated)
    float m_RecoveryRate = 10.0f; // 초당 회복량

    UPROPERTY(Replicated)
    float m_RecoveryDelay = 2.0f; // 회복 시작까지 대기 시간

    UPROPERTY(Replicated)
    float m_GroggyDuration = 4.0f;

    UPROPERTY(ReplicatedUsing = OnRep_CanRecover)
    bool bCanRecover = true;
    
    // 로컬 변수
    FTimerHandle RecoveryDelayHandle;
    
    // 회복 중단 시간 추적 (서버)
    float LastDamageTime = 0.0f;
};