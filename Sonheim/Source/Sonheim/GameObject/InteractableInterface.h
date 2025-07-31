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
};