// Fill out your copyright notice in the Description page of Project Settings.


#include "MeleeAttackNotifyState.h"

#include "Sonheim/AreaObject/Base/AreaObject.h"
#include "Sonheim/AreaObject/Skill/Common/MeleeAttack.h"

void UMeleeAttackNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                          float TotalDuration)
{
	if (MeshComp && MeshComp->GetOwner())
	{
		AAreaObject* m_Owner = Cast<AAreaObject>(MeshComp->GetOwner());
		if (m_Owner != nullptr && m_Owner->GetCurrentSkill() != nullptr)
		{
			UMeleeAttack* meleeSkill = Cast<UMeleeAttack>(m_Owner->GetCurrentSkill());
			if (meleeSkill != nullptr)
			{
				meleeSkill->SetCasterMesh(AttackDataIndex);
			}
		}
	}
}

void UMeleeAttackNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (MeshComp && MeshComp->GetOwner())
	{
		AAreaObject* m_Owner = Cast<AAreaObject>(MeshComp->GetOwner());
		if (m_Owner != nullptr && m_Owner->GetCurrentSkill() != nullptr)
		{
			UMeleeAttack* meleeSkill = Cast<UMeleeAttack>(m_Owner->GetCurrentSkill());
			if (meleeSkill != nullptr)
			{
				meleeSkill->ResetCollisionData(AttackDataIndex);
			}
		}
	}
}
