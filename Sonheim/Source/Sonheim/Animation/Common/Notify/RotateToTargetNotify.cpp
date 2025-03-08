// Fill out your copyright notice in the Description page of Project Settings.


#include "RotateToTargetNotify.h"
#include "Sonheim/AreaObject/Monster/BaseMonster.h"

void URotateToTargetNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (MeshComp && MeshComp->GetOwner())
	{
		ABaseMonster* owner = Cast<ABaseMonster>(MeshComp->GetOwner());
		if (owner != nullptr)
		{
			AAreaObject* target = Cast<AAreaObject>(owner->GetAggroTarget());
			if (target != nullptr)
			{
				FVector targetLocation = target->GetActorLocation();
				if (DurationOrSpeed != 0)
				{
					owner->LookAtLocation(targetLocation, RotationMode, DurationOrSpeed, RotationRatio);
				}
				else
				{
					owner->LookAtLocationDirect(targetLocation);
				}
			}
		}
	}
}
