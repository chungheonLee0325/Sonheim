#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Sonheim/Animation/Common/AnimInstance/BaseAnimInstance.h"
#include "PlayerAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class SONHEIM_API UPlayerAnimInstance : public UBaseAnimInstance
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=FSM)
	float speed = 0;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=FSM)
	float direction = 0;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=FSM)
	bool bIsGuard;
	void NativeUpdateAnimation(float DeltaSeconds);
};
