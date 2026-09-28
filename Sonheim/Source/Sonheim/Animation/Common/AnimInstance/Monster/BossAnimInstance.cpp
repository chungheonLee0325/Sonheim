#include "BossAnimInstance.h"

void UBossAnimInstance::NativeUpdateAnimation(const float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	if (const APawn* Pawn = TryGetPawnOwner()) Speed = Pawn->GetVelocity().Size2D();
}
