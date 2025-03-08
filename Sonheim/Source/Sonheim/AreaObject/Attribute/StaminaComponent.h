#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StaminaComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnStaminaChangedDelegate, float, CurrentStamina, float, Delta, float,
                                               MaxStamina);

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
	// 초기화
	void InitStamina(float StaminaMax, float RecoveryRate, float GroggyDuration);

	// 스태미나 증감
	UFUNCTION()
	float DecreaseStamina(float Delta, bool bIsDamaged = true);

	UFUNCTION()
	float IncreaseStamina(float Delta);

	// 스태미나 회복 관련
	void StartStaminaRecovery();
	void StopStaminaRecovery();

	// Getter/Setter
	UFUNCTION()
	float GetStamina() const { return m_Stamina; }

	UFUNCTION()
	float GetMaxStamina() const { return m_StaminaMax; }

	UFUNCTION()
	bool CanUseStamina(float Cost) const { return m_Stamina >= Cost * 0.3f; }
	//bool CanUseStamina(float Cost) const { return !FMath::IsNearlyZero(m_Stamina); }

	// 델리게이트
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnStaminaChangedDelegate OnStaminaChanged;
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnApplyGroggyDelegate OnApplyGroggyDelegate;

	// 가드 중 스태미나 회복률 감소 설정
	UPROPERTY(EditAnywhere, Category = "Recovery")
	float m_GuardRecoveryRateMultiplier = 0.3f; // 가드 중 회복률 50%
	float m_GroggyDuration = 4.f;

	// 가드 상태 설정/해제
	void SetGuardState(bool bIsGuarding);

private:
	UPROPERTY(VisibleAnywhere)
	float m_StaminaMax = 100.0f;

	UPROPERTY(VisibleAnywhere)
	float m_Stamina;

	// 스태미나 회복 관련 변수
	UPROPERTY(EditAnywhere, Category = "Recovery")
	float m_RecoveryRate = 10.0f; // 초당 회복량

	UPROPERTY(EditAnywhere, Category = "Recovery")
	float m_RecoveryDelay = 1.0f; // 회복 시작까지 대기 시간

	bool bCanRecover = true;
	FTimerHandle RecoveryDelayHandle;

	bool bIsGuarding = false;
};
