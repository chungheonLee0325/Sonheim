// Fill out your copyright notice in the Description page of Project Settings.


#include "SonheimGameState.h"
#include "Net/UnrealNetwork.h"
#include "Sonheim/Utilities/LogMacro.h"

void ASonheimGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASonheimGameState, DungeonStageState);
}

void ASonheimGameState::PublishDungeonStageState(FDungeonStageRuntimeState NewState)
{
	if (!HasAuthority() || DungeonStageState.SamePresentationState(NewState)) return;
	NewState.Revision = DungeonStageState.Revision + 1;
	DungeonStageState = NewState;
	ForceNetUpdate();
	OnRep_DungeonStageState(); // Native C++ setters do not invoke RepNotify on the listen host.
}

void ASonheimGameState::OnRep_DungeonStageState()
{
	UE_LOG(SONHEIM, Log, TEXT("[DungeonSnapshot] Role=%s Run=%s Revision=%d Stage=%s Count=%d/%d Branch=%s Status=%d"),
		HasAuthority() ? TEXT("Server") : TEXT("Client"), *DungeonStageState.RunId.ToString(), DungeonStageState.Revision,
		*DungeonStageState.StageId.ToString(), DungeonStageState.CurrentCount, DungeonStageState.RequiredCount,
		*DungeonStageState.SelectedBranchId.ToString(), int32(DungeonStageState.RunStatus));
	OnDungeonStageStateChanged.Broadcast(DungeonStageState);
}
