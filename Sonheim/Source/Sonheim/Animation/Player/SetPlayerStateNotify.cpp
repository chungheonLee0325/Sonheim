// Fill out your copyright notice in the Description page of Project Settings.


#include "SetPlayerStateNotify.h"


void USetPlayerStateNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (MeshComp && MeshComp->GetOwner())
	{
		ASonheimPlayer* owner = Cast<ASonheimPlayer>(MeshComp->GetOwner());
		if (owner != nullptr)
		{
			owner->SetPlayerState(PlayerState);
			if (PlayerState == EPlayerState::NORMAL)
			{
				owner->ClearCurrentSkill();
			}
			else if(PlayerState == EPlayerState::CANACTION)
			{
				owner->ClearCurrentSkill();
			}
		}
	}
}
