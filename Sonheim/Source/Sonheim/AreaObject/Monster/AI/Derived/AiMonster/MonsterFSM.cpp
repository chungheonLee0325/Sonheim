// Fill out your copyright notice in the Description page of Project Settings.


#include "MonsterFSM.h"

#include "Sonheim/AreaObject/Monster/BaseMonster.h"
#include "Sonheim/AreaObject/Monster/AI/Derived/CommonState/AggroWait.h"
#include "Sonheim/AreaObject/Monster/AI/Derived/CommonState/ChaseTarget.h"
#include "Sonheim/AreaObject/Monster/AI/Derived/CommonState/CommonAttack.h"
#include "Sonheim/AreaObject/Monster/AI/Derived/CommonState/DoNothing.h"
#include "Sonheim/AreaObject/Monster/AI/Derived/CommonState/ReturnSpawnPoint.h"
#include "Sonheim/AreaObject/Monster/AI/Derived/CommonState/SelectSkill.h"


// Sets default values for this component's properties
UMonsterFSM::UMonsterFSM()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UMonsterFSM::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UMonsterFSM::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UMonsterFSM::InitStatePool()
{
	// Wait 
	auto AggroWait = CreateState<UAggroWait>(this, m_Owner, EAiStateType::SelectSkill);
	AddState(EAiStateType::Wait, AggroWait);

	// Chase
	auto ChaseTarget = CreateState<UChaseTarget>(this, m_Owner, EAiStateType::None, EAiStateType::Attack,
												 EAiStateType::SelectSkill);
	ChaseTarget->SetChaseAccelSpeed(500.f);
	//ChaseTarget->SetMaxChaseDistance(1400.f);
	ChaseTarget->SetMaxChaseDistanceState(EAiStateType::Return); // ToDo : Return 구현
	AddState(EAiStateType::Chase, ChaseTarget);

	// Attack
	auto Attack = CreateState<UCommonAttack>(this, m_Owner, EAiStateType::None, EAiStateType::SelectSkill,
											 EAiStateType::SelectSkill);
	AddState(EAiStateType::Attack, Attack);

	// SelectSkill
	auto SelectSkill = CreateState<USelectSkill>(this, m_Owner, EAiStateType::Chase);
	SelectSkill->SetSkillRoulette(m_Owner->GetSkillRoulette());
	AddState(EAiStateType::SelectSkill, SelectSkill);

	// Groggy
	auto Groggy = CreateState<UDoNothing>(this, m_Owner);
	AddState(EAiStateType::DoNothing, Groggy);
	
	// BackHome
	auto BackHome = CreateState<UReturnSpawnPoint>(this, m_Owner, EAiStateType::None, EAiStateType::Wait,
												   EAiStateType::SelectSkill);
	BackHome->SetReturnAccelSpeed(300.f);
	AddState(EAiStateType::Return, BackHome);

	// 시작 State
	ChangeState(EAiStateType::Wait);
}

