// Fill out your copyright notice in the Description page of Project Settings.


#include "LambBallAnimInstance.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Sonheim/AreaObject/Monster/AI/Base/BaseAiFSM.h"
#include "Sonheim/AreaObject/Monster/AI/Base/BaseAiState.h"
#include "Sonheim/AreaObject/Monster/Variants/NormalMonsters/Lamball/LamBall.h"

void ULambBallAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	
	auto Character{Cast<ALamBall>(m_Owner)};
	if (Character)
	{
		Speed = Character->GetVelocity().Length();
		bIsDead = Character->IsDead;
		bIsJump = Character->GetCharacterMovement()->IsFalling();
		bIsDizzy = Character->isDizzy;
		
		if (Character->m_AiFSM->m_CurrentState)
		{
			State = Character->m_AiFSM->m_CurrentState->AiStateType();
			//LOG_PRINT(TEXT("State : %d"), State);
		}
	}
}
