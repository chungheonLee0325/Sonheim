// Fill out your copyright notice in the Description page of Project Settings.


#include "LamballFSM.h"

#include "GatherToPlayer.h"
#include "Lumbering.h"
#include "SelectAction.h"
#include "WaitState.h"
#include "Sonheim/AreaObject/Monster/AI/Derived/CommonState/DoNothing.h"


// Sets default values for this component's properties
ULamballFSM::ULamballFSM()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void ULamballFSM::BeginPlay()
{
	Super::BeginPlay();

	// ...
}


// Called every frame
void ULamballFSM::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void ULamballFSM::InitStatePool()
{
	//Super::InitStatePool();

	// Wait 
	auto Wait = CreateState<UWaitState>(this, m_Owner, EAiStateType::SelectAction);
	AddState(EAiStateType::Wait, Wait);

	// SelectAction 
	auto SelectAction = CreateState<USelectAction>(this, m_Owner,EAiStateType::Lumbering);
	AddState(EAiStateType::SelectAction, SelectAction);

	// Lumbering 
	auto Lumbering = CreateState<ULumbering>(this, m_Owner, EAiStateType::Wait);
	AddState(EAiStateType::Lumbering, Lumbering);

	// ReturnResource 
	auto ReturnResource = CreateState<ULumbering>(this, m_Owner, EAiStateType::Lumbering);
	AddState(EAiStateType::ReturnResource, ReturnResource);

	auto DoNothing =  CreateState<UDoNothing>(this, m_Owner);
	AddState(EAiStateType::DoNothing, DoNothing);

	// SelectAction 
	auto GatherToPlayer = CreateState<UGatherToPlayer>(this, m_Owner);
	AddState(EAiStateType::Chase, GatherToPlayer);

	// 시작 State
	ChangeState(EAiStateType::Wait);
}
