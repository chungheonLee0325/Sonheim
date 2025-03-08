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
	//Health 초기화
	void InitHealth(float hpMax);

	// 체력 증감
	UFUNCTION()
	float IncreaseHP(float Delta);

	UFUNCTION()
	float DecreaseHP(float Delta);

	//최대체력기준 현재체력
	UFUNCTION()
	void SetHPByRate(float Rate);

	//현제체력
	UFUNCTION()
	float GetHP() const;

	// 최대체력
	UFUNCTION()
	float GetMaxHP() const;

	// 체력 변경 델리게이트
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnHealthChangedDelegate OnHealthChanged;

private:
	UPROPERTY(VisibleAnywhere)
	float m_HPMax = 1.0f;

	UPROPERTY(VisibleAnywhere)
	float m_HP;
};
