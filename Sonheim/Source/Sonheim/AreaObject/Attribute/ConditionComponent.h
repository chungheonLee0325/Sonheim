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
	UConditionComponent();
    
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    
	UFUNCTION(BlueprintCallable, Category = "Condition")
	bool IsDead() const { return HasCondition(EConditionBitsType::Dead); }

	// 권한 체크를 내부에서 처리
	UFUNCTION(BlueprintCallable, Category = "Condition")
	void AddCondition(EConditionBitsType Condition, float Duration = 0.0f);

	UFUNCTION(BlueprintCallable, Category = "Condition")
	void RemoveCondition(EConditionBitsType Condition);

	UFUNCTION(BlueprintCallable, Category = "Condition")
	bool HasCondition(EConditionBitsType Condition) const;

	UFUNCTION(BlueprintCallable, Category = "Condition")
	void ExchangeDead();

	UFUNCTION(BlueprintCallable, Category = "Condition")
	void Restart();

private:
	UPROPERTY(ReplicatedUsing = OnRep_ConditionFlags)
	uint32 ConditionFlags = 0;

	UFUNCTION()
	void OnRep_ConditionFlags(uint32 OldFlags);

	// 타이머 관리
	UPROPERTY()
	TMap<EConditionBitsType, FTimerHandle> ConditionTimers;

	void RemoveConditionInternal(EConditionBitsType Condition);
};