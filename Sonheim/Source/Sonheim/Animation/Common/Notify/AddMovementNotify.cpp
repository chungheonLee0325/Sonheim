// Fill out your copyright notice in the Description page of Project Settings.


#include "AddMovementNotify.h"

#include "Sonheim/AreaObject/Base/AreaObject.h"

void UAddMovementNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (MeshComp && MeshComp->GetOwner())
	{
		AAreaObject* owner = Cast<AAreaObject>(MeshComp->GetOwner());
		if (owner != nullptr)
		{
			FVector targetLocation = owner->GetActorLocation() + owner->GetActorForwardVector() * MoveLength;
			if (Duration != 0)
			{
				owner->MoveActorTo(targetLocation, Duration);
			}
		}
	}
}
