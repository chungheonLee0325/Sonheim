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
	// 리플리케이션 설정 함수
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	// 초기화
	UFUNCTION(Server, Reliable)
	void InitStamina(float StaminaMax, float RecoveryRate, float GroggyDuration);

	// 스태미나 증감
	UFUNCTION(Server, Reliable)
	void DecreaseStamina(float Delta, bool bIsDamaged = true);

	UFUNCTION(Server, Reliable)
	void IncreaseStamina(float Delta);

	// 스태미나 회복 관련
	UFUNCTION(Server, Reliable)
	void StartStaminaRecovery();
	
	UFUNCTION(Server, Reliable)
	void StopStaminaRecovery();

	// Getter/Setter
	UFUNCTION()
	float GetStamina() const { return m_Stamina; }

	UFUNCTION()
	float GetMaxStamina() const { return m_StaminaMax; }

	UFUNCTION()
	bool CanUseStamina(float Cost) const { return m_Stamina >= Cost * 0.3f; }

	// 클라이언트에 스태미나 변경 알림
	UFUNCTION(Client, Reliable)
	void Client_OnStaminaChanged(float CurrentStamina, float Delta, float MaxStamina);

	// 델리게이트
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnStaminaChangedDelegate OnStaminaChanged;
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnApplyGroggyDelegate OnApplyGroggyDelegate;
	
	UPROPERTY(ReplicatedUsing = OnRep_GroggyDuration)
	float m_GroggyDuration = 4.f;

private:
	UPROPERTY(ReplicatedUsing = OnRep_StaminaMax)
	float m_StaminaMax = 100.0f;

	UPROPERTY(ReplicatedUsing = OnRep_Stamina)
	float m_Stamina;

	// 스태미나 회복 관련 변수
	UPROPERTY(Replicated)
	float m_RecoveryRate = 10.0f; // 초당 회복량

	UPROPERTY(Replicated)
	float m_RecoveryDelay = 2.0f; // 회복 시작까지 대기 시간

	UPROPERTY(ReplicatedUsing = OnRep_CanRecover)
	bool bCanRecover = true;
	
	FTimerHandle RecoveryDelayHandle;

	// 복제 속성에 대한 응답 함수들
	UFUNCTION()
	void OnRep_Stamina();
	
	UFUNCTION()
	void OnRep_StaminaMax();
	
	UFUNCTION()
	void OnRep_CanRecover();
	
	UFUNCTION()
	void OnRep_GroggyDuration();
};
