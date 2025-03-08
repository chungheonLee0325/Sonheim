// Fill out your copyright notice in the Description page of Project Settings.


#include "AddConditionNotify.h"

#include "Sonheim/AreaObject/Base/AreaObject.h"

void UAddConditionNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (MeshComp && MeshComp->GetOwner())
	{
		AAreaObject* owner = Cast<AAreaObject>(MeshComp->GetOwner());
		if (owner != nullptr)
		{
			owner->AddCondition(Condition, DurationTime);
		}
	}
}
