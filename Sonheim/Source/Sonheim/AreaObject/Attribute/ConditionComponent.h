// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Sonheim/ResourceManager/SonheimGameType.h"
#include "ConditionComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SONHEIM_API UConditionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UConditionComponent();
	
	// 리플리케이션 설정
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
		
	UFUNCTION(BlueprintCallable, Category = "Condition")
	bool IsDead() const { return HasCondition(EConditionBitsType::Dead); }

	UFUNCTION(BlueprintCallable, Category = "Condition", Server, Reliable, WithValidation)
	void AddCondition(EConditionBitsType Condition, float Duration = 0.0f);

	UFUNCTION(BlueprintCallable, Category = "Condition", Server, Reliable, WithValidation)
	void RemoveCondition(EConditionBitsType Condition);

	UFUNCTION(BlueprintCallable, Category = "Condition")
	bool HasCondition(EConditionBitsType Condition) const;

	UFUNCTION(BlueprintCallable, Category = "Condition", Server, Reliable, WithValidation)
	void ExchangeDead();

	UFUNCTION(BlueprintCallable, Category = "Condition", Server, Reliable)
	void Restart();
	
	// 클라이언트에 컨디션 변경 알림
	UFUNCTION(Client, Reliable)
	void Client_OnConditionChanged(uint32 NewConditionFlags);

private:
	UPROPERTY(ReplicatedUsing = OnRep_ConditionFlags)
	uint32 ConditionFlags;

	// 컨디션 변경 시 호출되는 함수
	UFUNCTION()
	void OnRep_ConditionFlags();

	// Condition별 타이머 핸들 관리
	UPROPERTY()
	TMap<EConditionBitsType, FTimerHandle> ConditionTimers;

	bool _addCondition(EConditionBitsType Condition);
	bool _removeCondition(EConditionBitsType Condition);
	void RemoveConditionInternal(EConditionBitsType Condition);
};
