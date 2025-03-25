// Fill out your copyright notice in the Description page of Project Settings.

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
	// Sets default values for this component's properties
	UHealthComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
	
	// 리플리케이션 설정 함수
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	//Health 초기화
	UFUNCTION(Server, Reliable)
	void InitHealth(float hpMax);

	// 체력 증감
	UFUNCTION(Server, Reliable)
	void IncreaseHP(float Delta);

	UFUNCTION(Server, Reliable)
	void DecreaseHP(float Delta);

	//최대체력기준 현재체력
	UFUNCTION(Server, Reliable)
	void SetHPByRate(float Rate);

	//현제체력
	UFUNCTION()
	float GetHP();

	// 최대체력
	UFUNCTION()
	float GetMaxHP();

	UFUNCTION(Server, Reliable)
	void AddMaxHP(float Delta);
	
	UFUNCTION(Server, Reliable)
	void SetMaxHP(float MaxHP);

	// 클라이언트에서 HP 변경 알림을 받는 함수
	UFUNCTION(Client, Reliable)
	void Client_OnHealthChanged(float CurrentHP, float Delta, float MaxHP);

	// 체력 변경 델리게이트
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnHealthChangedDelegate OnHealthChanged;

private:
	UPROPERTY(ReplicatedUsing = OnRep_HPMax)
	float m_HPMax = 1.0f;

	UPROPERTY(ReplicatedUsing = OnRep_HP)
	float m_HP;
	
	// 복제 속성에 대한 응답 함수들
	UFUNCTION()
	void OnRep_HP();
	
	UFUNCTION()
	void OnRep_HPMax();
};
