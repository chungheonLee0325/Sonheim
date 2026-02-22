#include "QuestTurnInActor.h"

#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerState.h"
#include "Sonheim/AreaObject/Player/Utility/QuestComponent.h"

AQuestTurnInActor::AQuestTurnInActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

bool AQuestTurnInActor::CanInteract_Implementation() const
{
	return QuestID > 0;
}

void AQuestTurnInActor::OnDetected_Implementation(bool bDetected)
{
	// v1: no-op (visual feedback is handled by DetectWidget on the player side)
}

void AQuestTurnInActor::Interact_Implementation(ASonheimPlayer* Player)
{
	if (!HasAuthority() || !Player) return;
	if (QuestID <= 0) return;

	if (ASonheimPlayerState* PS = Player->GetPlayerState<ASonheimPlayerState>())
	{
		if (PS->m_QuestComponent)
		{
			PS->m_QuestComponent->RegisterQuestTurnIn(QuestID, NpcID, this, OfferLifetimeSeconds);
			PS->m_QuestComponent->ServerTryTurnIn(QuestID);
		}
	}
}

FString AQuestTurnInActor::GetInteractionName_Implementation() const
{
	return InteractionLabel;
}
