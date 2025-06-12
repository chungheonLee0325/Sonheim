#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnHealthChangedDelegate, float, CurrentHP, float, Delta, float, MaxHP);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SONHEIM_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHealthComponent();

	// 리플리케이션 설정
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    
	// 서버에서만 호출되는 초기화
	void InitHealth(float hpMax);

	// 체력 변경 - 내부에서 권한 체크
	void ModifyHP(float Delta);
	void SetHPByRate(float Rate);
    
	// 단순 Getter - RPC 불필요
	UFUNCTION(BlueprintCallable, Category = "Health")
	float GetHP() const { return m_HP; }
    
	UFUNCTION(BlueprintCallable, Category = "Health")
	float GetMaxHP() const { return m_HPMax; }
    
	// 최대 체력 수정
	void ModifyMaxHP(float Delta);
	void SetMaxHP(float MaxHP);

	// 체력 변경 이벤트
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnHealthChangedDelegate OnHealthChanged;

private:
	UPROPERTY(ReplicatedUsing = OnRep_HP)
	float m_HP = 100.0f;
    
	UPROPERTY(ReplicatedUsing = OnRep_HPMax)
	float m_HPMax = 100.0f;
    
	UFUNCTION()
	void OnRep_HP(float OldHP);
    
	UFUNCTION()
	void OnRep_HPMax(float OldHPMax);
};