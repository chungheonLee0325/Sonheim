#include "DungeonClientBridgeComponent.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Sonheim/UI/Dungeon/DungeonUIRouterSubsystem.h"
#include "Sonheim/GameManager/Dungeon/DungeonStageRuntimeSubsystem.h"
void UDungeonClientBridgeComponent::BeginPlay()
{
	Super::BeginPlay();
	if (auto* PC = Cast<APlayerController>(GetOwner()); PC && PC->IsLocalController() && PC->GetLocalPlayer())
		if (auto* Router = PC->GetLocalPlayer()->GetSubsystem<UDungeonUIRouterSubsystem>()) Router->Attach(PC);
}
void UDungeonClientBridgeComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	if (auto* PC = Cast<APlayerController>(GetOwner()))
	{
		if (PC->GetLocalPlayer())
			if (auto* Router = PC->GetLocalPlayer()->GetSubsystem<UDungeonUIRouterSubsystem>()) Router->Detach(PC);
		if (PC->HasAuthority() && GetWorld())
			if (auto* Runtime = GetWorld()->GetSubsystem<UDungeonStageRuntimeSubsystem>()) Runtime->AbortForOwner(PC, EDungeonFailReason::OwnerLeft);
	}
	Super::EndPlay(Reason);
}
