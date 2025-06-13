#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Sonheim/ResourceManager/SonheimGameType.h"
#include "AggroComponent.generated.h"

class AAreaObject;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAggroTargetChanged, AActor*, OldTarget, AActor*, NewTarget);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnThreatAdded, AActor*, Target, float, ThreatValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnThreatRemoved, AActor*, Target);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SONHEIM_API UAggroComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UAggroComponent();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:	
	// === 위협 관리 ===
	UFUNCTION(BlueprintCallable, Category = "Aggro")
	void AddThreat(AActor* Target, float ThreatValue, EAggroPriority Priority = EAggroPriority::Medium);
	
	UFUNCTION(BlueprintCallable, Category = "Aggro")
	void RemoveThreat(AActor* Target);
	
	UFUNCTION(BlueprintCallable, Category = "Aggro")
	void ClearAllThreats();
	
	UFUNCTION(BlueprintCallable, Category = "Aggro")
	void ModifyThreat(AActor* Target, float DeltaThreat);
	
	UFUNCTION(BlueprintCallable, Category = "Aggro")
	void SetThreatMultiplier(AActor* Target, float Multiplier);

	// === 대상 선택 ===
	UFUNCTION(BlueprintPure, Category = "Aggro")
	AActor* GetCurrentTarget() const { return CurrentTarget; }
	
	UFUNCTION(BlueprintPure, Category = "Aggro")
	AActor* GetHighestThreatTarget() const;
	
	UFUNCTION(BlueprintPure, Category = "Aggro")
	TArray<AActor*> GetThreatList() const;
	
	UFUNCTION(BlueprintPure, Category = "Aggro")
	float GetThreatValue(AActor* Target) const;
	
	UFUNCTION(BlueprintPure, Category = "Aggro")
	bool HasThreat(AActor* Target) const;

	// === 설정 ===
	UFUNCTION(BlueprintCallable, Category = "Aggro")
	void SetAggroEnabled(bool bEnabled) { bIsAggroEnabled = bEnabled; }
	
	UFUNCTION(BlueprintPure, Category = "Aggro")
	bool IsAggroEnabled() const { return bIsAggroEnabled; }
	
	UFUNCTION(BlueprintCallable, Category = "Aggro")
	void SetMaxThreatDistance(float Distance) { MaxThreatDistance = Distance; }

	// === Events ===
	UPROPERTY(BlueprintAssignable, Category = "Aggro")
	FOnAggroTargetChanged OnAggroTargetChanged;
	
	UPROPERTY(BlueprintAssignable, Category = "Aggro")
	FOnThreatAdded OnThreatAdded;
	
	UPROPERTY(BlueprintAssignable, Category = "Aggro")
	FOnThreatRemoved OnThreatRemoved;

	// === 특수 기능 ===
	UFUNCTION(BlueprintCallable, Category = "Aggro")
	void Taunt(AActor* Taunter, float Duration = 3.0f);
	
	UFUNCTION(BlueprintCallable, Category = "Aggro")
	void DropAggro(float Percentage = 0.5f);

protected:
	// === Server RPCs ===
	UFUNCTION(Server, Reliable)
	void Server_AddThreat(AActor* Target, float ThreatValue, EAggroPriority Priority);
	
	UFUNCTION(Server, Reliable)
	void Server_RemoveThreat(AActor* Target);
	
	UFUNCTION(Server, Reliable)
	void Server_ClearAllThreats();
	
	// === Multicast RPCs ===
	UFUNCTION(NetMulticast, Reliable)
	void MultiCast_OnTargetChanged(AActor* OldTarget, AActor* NewTarget);

private:
	// === 내부 함수 ===
	void UpdateCurrentTarget();
	void CleanupInvalidEntries();
	void DecayThreats(float DeltaTime);
	bool IsValidTarget(AActor* Target) const;
	float CalculateDistanceModifier(AActor* Target) const;
	float CalculatePriorityModifier(EAggroPriority Priority) const;

private:
	// === 위협 테이블 ===
	UPROPERTY(Replicated)
	TArray<FAggroEntry> ThreatTable;
	
	UPROPERTY(ReplicatedUsing = OnRep_CurrentTarget)
	AActor* CurrentTarget;
	
	UFUNCTION()
	void OnRep_CurrentTarget(AActor* OldTarget);

	// === 설정 ===
	UPROPERTY(EditDefaultsOnly, Category = "Aggro Settings")
	bool bIsAggroEnabled = true;
	
	UPROPERTY(EditDefaultsOnly, Category = "Aggro Settings")
	float MaxThreatDistance = 3000.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Aggro Settings")
	float ThreatDecayRate = 1.0f; // 초당 감소량
	
	UPROPERTY(EditDefaultsOnly, Category = "Aggro Settings")
	float ThreatUpdateInterval = 0.5f; // 타겟 업데이트 주기
	
	UPROPERTY(EditDefaultsOnly, Category = "Aggro Settings")
	float OutOfRangeThreatMultiplier = 0.5f; // 범위 밖 위협 배수
	
	// 우선순위별 가중치
	UPROPERTY(EditDefaultsOnly, Category = "Aggro Settings")
	TMap<EAggroPriority, float> PriorityWeights;

	// === 상태 ===
	float LastUpdateTime = 0.0f;
	
	// 도발 관련
	UPROPERTY()
	AActor* TauntTarget;
	
	FTimerHandle TauntTimerHandle;
	
	// === 참조 ===
	UPROPERTY()
	AAreaObject* OwnerAreaObject;
};