// Fill out your copyright notice in the Description page of Project Settings.


#include "Rolling.h"

#include "AIController.h"
#include "NavigationSystem.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "Sonheim/AreaObject/Base/AreaObject.h"
#include "Sonheim/AreaObject/Monster/BaseMonster.h"
#include "Sonheim/AreaObject/Monster/Variants/NormalMonsters/Lamball/LamBall.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"

URolling::URolling()
{
}

bool URolling::Activate(class AAreaObject* Caster, AAreaObject* Target)
{
	if (!Super::Activate(Caster, Target)) return false;
	CastPal = Cast<ALamBall>(m_Caster);

	return true;
}

void URolling::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

bool URolling::Fire()
{
	if (!Super::Fire()) return false;
	StartRoll();

	return true;
}

void URolling::StartRoll()
{
	UNavigationSystemV1* NavSystem{UNavigationSystemV1::GetCurrent(GetWorld())};
	if (!NavSystem)
	{
		return;
	}

	FNavLocation Next;

	m_Caster->GetCharacterMovement()->MaxWalkSpeed = 1000.f;
	NavSystem->ProjectPointToNavigation(m_TargetPos, Next);

	AAIController* CasterAiController{Cast<ABaseMonster>(m_Caster)->AIController};
	EPathFollowingRequestResult::Type Result{CasterAiController->MoveToLocation(Next.Location)};


	if (Result == EPathFollowingRequestResult::Type::RequestSuccessful)
	{
		CasterAiController->GetPathFollowingComponent()->OnRequestFinished.AddUObject(
			this, &URolling::MoveCompleted);
	}
}

void URolling::MoveCompleted(FAIRequestID FaiRequestID, const FPathFollowingResult& PathFollowingResult)
{
	m_Caster->GetCharacterMovement()->MaxWalkSpeed = 600.f;

	// Todo: 일단 도로롱일때만 ..
	if (CastPal)
	{
		CastPal->isDizzy = true;
		CastPal->GetCapsuleComponent()->SetSimulatePhysics(true);
		GetWorld()->GetTimerManager().SetTimer(DizzyWaitTimer, this, &URolling::DizzyEnd, 1.f, false);
	}

	// 바인딩 해제
	AAIController* CasterAiController{Cast<ABaseMonster>(m_Caster)->AIController};
	CasterAiController->GetPathFollowingComponent()->OnRequestFinished.RemoveAll(this);
}

void URolling::DizzyEnd()
{
	CastPal->isDizzy = false;
	CastPal->GetCapsuleComponent()->SetSimulatePhysics(false);
	if (CastPal->GetAggroTarget())
	{
		CastPal->LookAtLocation(CastPal->GetAggroTarget()->GetActorLocation(), EPMRotationMode::Duration, 0.1f);
	}
}
