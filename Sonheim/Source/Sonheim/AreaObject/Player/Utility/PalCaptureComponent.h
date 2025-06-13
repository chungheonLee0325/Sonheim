#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Sonheim/ResourceManager/SonheimGameType.h"
#include "PalCaptureComponent.generated.h"

class ABaseMonster;
class ASonheimPlayer;
class USonheimGameInstance;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnCaptureAttempt, ABaseMonster*, Target, float, CaptureRate, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCaptureSuccess, ABaseMonster*, CapturedPal);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCaptureFailed, ABaseMonster*, Target, float, CaptureRate);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SONHEIM_API UPalCaptureComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UPalCaptureComponent();

protected:
	virtual void BeginPlay() override;

public:	
	// === 포획 시도 ===
	UFUNCTION(BlueprintCallable, Category = "Pal Capture")
	void AttemptCapture(ABaseMonster* Target, int32 SphereItemID = 0);
	
	UFUNCTION(BlueprintPure, Category = "Pal Capture")
	float CalculateCaptureRate(ABaseMonster* Target, int32 SphereItemID = 0) const;
	
	UFUNCTION(BlueprintPure, Category = "Pal Capture")
	bool CanCapture(ABaseMonster* Target) const;

	// === Events ===
	UPROPERTY(BlueprintAssignable, Category = "Pal Capture")
	FOnCaptureAttempt OnCaptureAttempt;
	
	UPROPERTY(BlueprintAssignable, Category = "Pal Capture")
	FOnCaptureSuccess OnCaptureSuccess;
	
	UPROPERTY(BlueprintAssignable, Category = "Pal Capture")
	FOnCaptureFailed OnCaptureFailed;

protected:
	// === Server RPCs ===
	UFUNCTION(Server, Reliable)
	void Server_AttemptCapture(ABaseMonster* Target, int32 SphereItemID);
	
	// === Multicast RPCs ===
	UFUNCTION(NetMulticast, Reliable)
	void MultiCast_OnCaptureResult(ABaseMonster* Target, float CaptureRate, bool bSuccess);

private:
	// === 내부 함수 ===
	FPalCaptureData* GetCaptureData(ABaseMonster* Target) const;
	float GetSphereBonus(int32 SphereItemID) const;
	float GetConditionBonus(ABaseMonster* Target, const FPalCaptureData* CaptureData) const;
	void ProcessCaptureSuccess(ABaseMonster* Target);
	void ProcessCaptureFailed(ABaseMonster* Target);

private:
	// === 참조 ===
	UPROPERTY()
	ASonheimPlayer* OwnerPlayer;
	
	UPROPERTY()
	USonheimGameInstance* GameInstance;
	
	// === 설정 ===
	// 포획률 계산 설정
	UPROPERTY(EditDefaultsOnly, Category = "Capture Settings")
	float MinCaptureRate = 0.01f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Capture Settings")
	float MaxCaptureRate = 0.95f;
	
	// HP 임계값 설정
	UPROPERTY(EditDefaultsOnly, Category = "Capture Settings")
	float LowHPThreshold = 0.3f; // 30% 이하면 보너스
	
	UPROPERTY(EditDefaultsOnly, Category = "Capture Settings")
	float CriticalHPThreshold = 0.1f; // 10% 이하면 큰 보너스
	
	// 스피어 아이템별 보너스
	UPROPERTY(EditDefaultsOnly, Category = "Capture Settings")
	TMap<int32, float> SphereBonusMap;
	
	// 포획 시 획득 경험치 배율
	UPROPERTY(EditDefaultsOnly, Category = "Capture Settings")
	float CaptureExpMultiplier = 1.5f;
	
	// 디버그
	UPROPERTY(EditAnywhere, Category = "Debug")
	bool bShowDebugInfo = false;
};