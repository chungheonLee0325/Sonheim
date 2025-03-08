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
		
	UFUNCTION(BlueprintCallable, Category = "Condition")
	bool IsDead() const { return HasCondition(EConditionBitsType::Dead); }

	UFUNCTION(BlueprintCallable, Category = "Condition")
	bool AddCondition(EConditionBitsType Condition, float Duration = 0.0f);

	UFUNCTION(BlueprintCallable, Category = "Condition")
	bool RemoveCondition(EConditionBitsType Condition);

	UFUNCTION(BlueprintCallable, Category = "Condition")
	bool HasCondition(EConditionBitsType Condition) const;

	UFUNCTION(BlueprintCallable, Category = "Condition")
	bool ExchangeDead();

	UFUNCTION(BlueprintCallable, Category = "Condition")
	void Restart();

private:
	UPROPERTY()
	uint32 ConditionFlags;

	// Condition별 타이머 핸들 관리
	UPROPERTY()
	TMap<EConditionBitsType, FTimerHandle> ConditionTimers;

	bool _addCondition(EConditionBitsType Condition);
	bool _removeCondition(EConditionBitsType Condition);
	void RemoveConditionInternal(EConditionBitsType Condition);
};
