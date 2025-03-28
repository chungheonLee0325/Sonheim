// Fill out your copyright notice in the Description page of Project Settings.


#include "FoxparksAnimInstance.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Sonheim/AreaObject/Monster/AI/Base/BaseAiFSM.h"
#include "Sonheim/AreaObject/Monster/AI/Base/BaseAiState.h"
#include "Sonheim/AreaObject/Monster/Variants/NormalMonsters/FoxParks/Foxparks.h"

void UFoxparksAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	
	auto Character{Cast<AFoxparks>(m_Owner)};
	if (Character)
	{
		Speed = Character->GetVelocity().Length();
		bIsDead = Character->IsDead;
		bIsJump = Character->GetCharacterMovement()->IsFalling();

		if (Character->m_AiFSM)
		{
			if (Character->m_AiFSM->m_CurrentState)
			{
				State = Character->m_AiFSM->m_CurrentState->AiStateType();
				//LOG_PRINT(TEXT("State : %d"), State);
			}
		}

		
	}	
}
