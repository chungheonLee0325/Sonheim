// Fill out your copyright notice in the Description page of Project Settings.


#include "ComboNotifyState.h"

#include "Sonheim/AreaObject/Player/SonheimPlayer.h"

void UComboNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration)
{
	if (MeshComp && MeshComp->GetOwner())
	{
		ASonheimPlayer* owner = Cast<ASonheimPlayer>(MeshComp->GetOwner());
		if (owner != nullptr)
		{
			owner->SetComboState(true, NextComboSkillID);
		}
	}
}

void UComboNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (MeshComp && MeshComp->GetOwner())
	{
		ASonheimPlayer* owner = Cast<ASonheimPlayer>(MeshComp->GetOwner());
		if (owner != nullptr)
		{
			owner->SetComboState(false, 0);
		}
	}
}
