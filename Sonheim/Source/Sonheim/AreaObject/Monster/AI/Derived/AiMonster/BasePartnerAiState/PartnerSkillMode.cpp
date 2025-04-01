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

void UPartnerSkillMode::ServerEnter()
{
	if (m_Owner->bShowDebug)
	{
		FLog::Log("UPartnerSkillMode");
	}
	
	m_Owner->bIsAttach = true;
	//m_Owner->GetMesh()->SetRelativeLocation(FVector(0), true);

	m_Owner->OnRep_IsAttached();
}

void UPartnerSkillMode::ServerExecute(float dt)
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

void UPartnerSkillMode::ServerExit()
{
}

void UPartnerSkillMode::ClientEnter()
{
	// if (m_Owner->PartnerOwner && m_Owner->PartnerOwner->GetMesh())
	// {
	// 	AttachToPlayer();
	// }
}

void UPartnerSkillMode::ClientExecute(float dt)
{
	// // 스킬 사용
	// if (m_Owner->bActivateSkill)
	// {
	// 	ChangeState(m_SuccessState);
	// 	return;
	// }
	// 해제
	// if (!m_Owner->IsCalled)
	// {
	// 	DetachFromPlayer();
	// 	return;
	// }

}

void UPartnerSkillMode::AttachToPlayer()
{
	if (m_Owner->PartnerOwner && m_Owner->PartnerOwner->GetMesh())
	{
		MulticastRPC_AttachToPlayer();
	}
}

void UPartnerSkillMode::Server_AttachToPlayer_Implementation()
{
	MulticastRPC_AttachToPlayer();
}

void UPartnerSkillMode::MulticastRPC_AttachToPlayer_Implementation()
{
	m_Owner->SetActorEnableCollision(false);
	m_Owner->GetMesh()->SetRelativeLocation(FVector(0));
	m_Owner->AttachToComponent(m_Owner->PartnerOwner->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, FName("PartnerWeapon"));
	m_Owner->PartnerOwner->SetUsePartnerSkill(true);
}

void UPartnerSkillMode::ClientRPC_AttachToPlayer_Implementation()
{
	m_Owner->SetActorEnableCollision(false);
	m_Owner->GetMesh()->SetRelativeLocation(FVector(0));
	m_Owner->AttachToComponent(m_Owner->PartnerOwner->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, FName("PartnerWeapon"));
	m_Owner->PartnerOwner->SetUsePartnerSkill(true);
}

void UPartnerSkillMode::DetachFromPlayer()
{
	// MultiCastRPC_DetachFromPlayer();
	// ClientRPC_DetachFromPlayer();
	m_Owner->bIsAttach = false;
	m_Owner->OnRep_IsAttached();
	ChangeState(m_NextState);
}



void UPartnerSkillMode::ClientRPC_DetachFromPlayer_Implementation()
{
	m_Owner->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	m_Owner->SetActorEnableCollision(true);
	m_Owner->GetMesh()->SetRelativeLocation(FVector(0, 0, -60));
	m_Owner->PartnerOwner->SetUsePartnerSkill(false);
}

void UPartnerSkillMode::MultiCastRPC_DetachFromPlayer_Implementation()
{
	m_Owner->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	m_Owner->SetActorEnableCollision(true);
	m_Owner->GetMesh()->SetRelativeLocation(FVector(0, 0, -60));
	m_Owner->PartnerOwner->SetUsePartnerSkill(false);

	// PartnerPatrolMode
	//ChangeState(m_NextState);
}
