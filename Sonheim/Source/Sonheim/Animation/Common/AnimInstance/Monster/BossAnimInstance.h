#pragma once

#include "CoreMinimal.h"
#include "Sonheim/Animation/Common/AnimInstance/BaseAnimInstance.h"
#include "BossAnimInstance.generated.h"

/** The parent of a boss's Animation Blueprint: the locomotion blend space reads Speed, and the patterns play as montages over it. */
UCLASS()
class SONHEIM_API UBossAnimInstance : public UBaseAnimInstance
{
	GENERATED_BODY()
public:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	/** Ground speed of the boss. */
	UPROPERTY(BlueprintReadOnly, Category="Boss") float Speed = 0.f;
};
