// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillEndNotify.h"

#include "Sonheim/AreaObject/Base/AreaObject.h"
#include "Sonheim/AreaObject/Skill/Base/BaseSkill.h"

void USkillEndNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (!MeshComp || !MeshComp->GetOwner()) return;
	AAreaObject* owner = Cast<AAreaObject>(MeshComp->GetOwner());
	if (!owner) return;
	if (UBaseSkill* Skill = owner->GetCurrentSkill())
	{
		if (owner->HasAuthority())
		{
			Skill->Complete();
		}
		else
		{
			owner->Server_NotifySkillComplete(Skill->GetSkillID());
		}
	}
}
