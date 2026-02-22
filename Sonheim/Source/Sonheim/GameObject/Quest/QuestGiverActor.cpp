#include "QuestGiverActor.h"

#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerController.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerState.h"
#include "Sonheim/AreaObject/Player/Utility/QuestComponent.h"

AQuestGiverActor::AQuestGiverActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

bool AQuestGiverActor::CanInteract_Implementation() const
{
	return QuestID > 0;
}

void AQuestGiverActor::OnDetected_Implementation(bool bDetected)
{
	// v1: no-op (visual feedback is handled by DetectWidget on the player side)
}

void AQuestGiverActor::Interact_Implementation(ASonheimPlayer* Player)
{
	if (!HasAuthority() || !Player) return;
	if (QuestID <= 0) return;

	ASonheimPlayerState* PS = Player->GetPlayerState<ASonheimPlayerState>();
	if (!PS || !PS->m_QuestComponent) return;

	PS->m_QuestComponent->RegisterQuestOffer(QuestID, NpcID, this, OfferLifetimeSeconds);

	if (bAutoAccept)
	{
		PS->m_QuestComponent->ServerAcceptQuest(QuestID);

		if (bOpenJournalOnAccept)
		{
			if (ASonheimPlayerController* PC = Cast<ASonheimPlayerController>(Player->GetController()))
			{
				PC->Client_OpenQuestJournal();
			}
		}
		return;
	}

	if (ASonheimPlayerController* PC = Cast<ASonheimPlayerController>(Player->GetController()))
	{
		PC->Client_ShowQuestAcceptUI(QuestID);
	}
}

FString AQuestGiverActor::GetInteractionName_Implementation() const
{
	return InteractionLabel;
}
