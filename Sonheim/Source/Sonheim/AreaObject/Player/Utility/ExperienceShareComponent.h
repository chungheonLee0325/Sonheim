#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ExperienceShareComponent.generated.h"

class ABaseMonster;
class ULevelComponent;
class UPalManagementComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnExpShared, ABaseMonster*, Pal, int32, ExpAmount, float, ShareRatio);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SONHEIM_API UExperienceShareComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UExperienceShareComponent();

protected:
	virtual void BeginPlay() override;

public:	
	// === 경험치 공유 ===
	UFUNCTION(BlueprintCallable, Category = "Experience Share")
	void ShareExperience(int32 Exp);
	
	UFUNCTION(BlueprintCallable, Category = "Experience Share")
	void SetShareEnabled(bool bEnabled) { bIsShareEnabled = bEnabled; }
	
	UFUNCTION(BlueprintPure, Category = "Experience Share")
	bool IsShareEnabled() const { return bIsShareEnabled; }
	
	UFUNCTION(BlueprintCallable, Category = "Experience Share")
	void SetShareRatio(float Ratio) { ShareRatio = FMath::Clamp(Ratio, 0.0f, 1.0f); }
	
	UFUNCTION(BlueprintPure, Category = "Experience Share")
	float GetShareRatio() const { return ShareRatio; }

	// === Events ===
	UPROPERTY(BlueprintAssignable, Category = "Experience Share")
	FOnExpShared OnExpShared;

protected:
	// === 내부 함수 ===
	void DistributeExperience(int32 ExpAmount);
	float CalculateDistanceModifier(ABaseMonster* Pal) const;
	float CalculateBattleContributionModifier(ABaseMonster* Pal) const;

private:
	// === 설정 ===
	UPROPERTY(EditDefaultsOnly, Category = "Experience Settings")
	bool bIsShareEnabled = true;
	
	UPROPERTY(EditDefaultsOnly, Category = "Experience Settings", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ShareRatio = 0.5f; // 파트너 팰이 받는 경험치 비율
	
	UPROPERTY(EditDefaultsOnly, Category = "Experience Settings")
	float MaxShareDistance = 2000.0f; // 경험치 공유 최대 거리
	
	UPROPERTY(EditDefaultsOnly, Category = "Experience Settings")
	bool bUseDistanceModifier = true; // 거리에 따른 경험치 감소 사용
	
	UPROPERTY(EditDefaultsOnly, Category = "Experience Settings")
	bool bUseBattleContribution = false; // 전투 기여도에 따른 보너스 사용
	
	// 소환된 팰 추가 경험치 배율
	UPROPERTY(EditDefaultsOnly, Category = "Experience Settings")
	float SummonedPalExpMultiplier = 1.2f;
	
	// 전투 참여 팰 추가 경험치 배율
	UPROPERTY(EditDefaultsOnly, Category = "Experience Settings")
	float BattleParticipantExpMultiplier = 1.5f;

	// === 참조 ===
	UPROPERTY()
	ULevelComponent* OwnerLevelComponent;
	
	UPROPERTY()
	UPalManagementComponent* PalManagementComponent;
	
	// === 전투 기여도 추적 ===
	UPROPERTY()
	TMap<TWeakObjectPtr<ABaseMonster>, float> BattleContributionMap;
	
	// 전투 기여도 초기화 타이머
	FTimerHandle ContributionResetTimer;
	
	void ResetBattleContributions();
};