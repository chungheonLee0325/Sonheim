// Fill out your copyright notice in the Description page of Project Settings.


#include "RecoverHPSkill.h"

#include "Sonheim/AreaObject/Player/SonheimPlayer.h"

void URecoverHPSkill::OnCastFire()
{
	Super::OnCastFire();

	m_Caster->IncreaseHP(m_SkillData->AttackData[0].HealthDamageAmountMax);
}
