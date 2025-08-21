#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Sonheim/GameObject/InteractableInterface.h"
#include "InteractionComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractableChanged, AActor*, NewInteractable);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAutoPickupDetected, AActor*, AutoPickupItem);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMonsterDetected, ABaseMonster*, DetectedMonster);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SONHEIM_API UInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInteractionComponent();

protected:
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 감지 설정
	UPROPERTY(EditDefaultsOnly, Category = "Detection")
	float DetectionRange = 2000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Detection")
	float InteractionRange = 500.0f;  // 상호작용 가능 거리

	UPROPERTY(EditDefaultsOnly, Category = "Detection")
	float DetectionInterval = 0.1f;

	UPROPERTY(EditDefaultsOnly, Category = "Detection")
	float HPBarDisplayDuration = 5.0f;

	// 홀드 상호작용 상태
	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	float HoldProgress = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	bool bIsHolding = false;

public:
	// 델리게이트
	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOnInteractableChanged OnInteractableChanged;

	// 몬스터 감지 델리게이트
	UPROPERTY(BlueprintAssignable, Category = "Detection")
	FOnMonsterDetected OnMonsterDetected;

	// 상호작용 실행
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void TryInteract();

	UFUNCTION(Server, Reliable, Category = "Interaction")
	void Server_TryInteract(AActor* Interactor);
	

	// 홀드 상호작용
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void StartHoldInteraction();

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void StopHoldInteraction();

	// 현재 상호작용 가능한 대상 가져오기
	UFUNCTION(BlueprintPure, Category = "Interaction")
	AActor* GetCurrentInteractable() const { return CurrentInteractable; }

	// 외부에서 감지 정보 갱신을 요청용 메서드
	UFUNCTION(BlueprintCallable, Category = "Detection")
	void RequestDetectionUpdate();
private:
	// 소유 플레이어
	UPROPERTY()
	class ASonheimPlayer* OwnerPlayer;

	// 현재 상호작용 가능한 대상
	UPROPERTY()
	AActor* CurrentInteractable = nullptr;

	// 현재 감지된 몬스터
	UPROPERTY()
	ABaseMonster* CurrentDetectedMonster = nullptr;

	// 타이머 핸들
	FTimerHandle DetectionTimerHandle;
	FTimerHandle HoldTimerHandle;

	// HP바가 표시된 몬스터와 타이머 추적
	TMap<class ABaseMonster*, FTimerHandle> DisplayedMonsters;

	// 통합 감지 수행
	void PerformDetection();

	// 몬스터 HP바 표시
	void ShowMonsterHPBar(class ABaseMonster* Monster);

	// 몬스터 HP바 숨김
	void HideMonsterHPBar(class ABaseMonster* Monster);

	// 상호작용 대상 업데이트
	void UpdateInteractable(AActor* NewTarget, float Distance);

	// 감지된 몬스터 업데이트
	void UpdateDetectedMonster(ABaseMonster* NewMonster);

	// 모든 타이머 정리
	void ClearAllTimers();
};