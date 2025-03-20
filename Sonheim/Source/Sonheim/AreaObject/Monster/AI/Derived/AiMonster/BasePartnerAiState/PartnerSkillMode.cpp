// Fill out your copyright notice in the Description page of Project Settings.


#include "PartnerSkillMode.h"

#include "Sonheim/AreaObject/Monster/BaseMonster.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"

void UPartnerSkillMode::InitState()
{
}

void UPartnerSkillMode::CheckIsValid()
{
}

void UPartnerSkillMode::Enter()
{
	if (m_Owner->bShowDebug)
	{
		FLog::Log("UPartnerSkillMode");
	}

	ASonheimPlayer* PartnerOwner = {Cast<ABaseMonster>(m_Owner)->PartnerOwner};
	// ToDo : PartnerOwner 설정되면 없애기
	ASonheimPlayer* Player{Cast<ASonheimPlayer>(GetWorld()->GetFirstPlayerController()->GetPawn())};
	PartnerOwner = Player;
	
	if (Player && Player->GetMesh())
	{
		m_Owner->SetActorEnableCollision(false);
		m_Owner->GetMesh()->SetRelativeLocation(FVector(0));
		m_Owner->AttachToComponent(PartnerOwner->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, FName("PartnerWeapon"));
	}
}

void UPartnerSkillMode::Execute(float dt)
{
	// 스킬 사용
	if (m_Owner->bActivateSkill)
	{
		ChangeState(m_SuccessState);
		return;
	}
	// 해제
	if (!m_Owner->IsCalled)
	{
		DetachFromPlayer();
		return;
	}
}

void UPartnerSkillMode::Exit()
{
}

void UPartnerSkillMode::DetachFromPlayer()
{
	m_Owner->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	m_Owner->SetActorEnableCollision(true);
	m_Owner->GetMesh()->SetRelativeLocation(FVector(0, 0, -60));

	// PartnerPatrolMode
	ChangeState(m_NextState);
}
