#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractableInterface.generated.h"

// 상호작용 가능한 타겟 타입
UENUM(BlueprintType)
enum class EInteractableType : uint8
{
	None,
	Item,
	Monster,
	NPC,
	Object
};

UENUM(BlueprintType)
enum class EHoldPurpose : uint8
{
	Interact,	// 기본 상호작용 (F)
	Cancel		// 취소 (C)
};

UINTERFACE(MinimalAPI, Blueprintable)
class UInteractableInterface : public UInterface
{
	GENERATED_BODY()
};

class SONHEIM_API IInteractableInterface
{
	GENERATED_BODY()

public:
	// 상호작용 가능 여부
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	bool CanInteract() const;

	// 감지됨/해제됨 알림
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void OnDetected(bool bDetected);

	// 상호작용 실행
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void Interact(class ASonheimPlayer* Player);

	// 표시할 이름
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	FString GetInteractionName() const;

	// 상호작용 타입
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	EInteractableType GetInteractableType() const;

	// 홀드 시간 (0이면 즉시 상호작용)
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	float GetHoldDuration() const;

	// DetectWidget에 표시할 홀드 진행률 갱신
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interaction|UI")
	void UpdateHoldProgressUI(float Progress, EHoldPurpose Purpose);

	// 길게 눌러서 취소 가능 여부
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interaction|UI")
	bool CanHoldCancel() const;

	// 기본: 취소 미지원
	virtual bool CanHoldCancel_Implementation() const { return false; }

	// 길게 눌러 취소 실행
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interaction|UI")
	void ExecuteCancel(class ASonheimPlayer* Player);

	// 취소 홀드 길이(미구현 시 기본값 사용)
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interaction|UI")
	float GetCancelHoldDuration() const;

	// 홀드 중 상호작용 모드 식별자(기본 0)
	// InteractionComponent는 이 값이 변하면 홀드를 중단한다.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interaction")
	int32 GetInteractionModeCode() const;
	virtual int32 GetInteractionModeCode_Implementation() const { return 0; }
};
