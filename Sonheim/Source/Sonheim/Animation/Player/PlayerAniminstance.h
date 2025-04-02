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
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Replicated,  Category=FSM)
	bool bIsMelee = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Replicated, Category=FSM)
	bool bIsLockOn = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Replicated, Category=FSM)
	bool bUsingPartnerSkill = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=FSM)
	bool bIsThrowPalSphere = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Replicated, Category=FSM)
	bool bIsDead = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	bool bIsGliding = false;
protected:
	void NativeUpdateAnimation(float DeltaSeconds);
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
};
