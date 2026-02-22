#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Sonheim/GameObject/InteractableInterface.h"

#include "QuestGiverActor.generated.h"

UCLASS()
class SONHEIM_API AQuestGiverActor : public AActor, public IInteractableInterface
{
	GENERATED_BODY()

public:
	AQuestGiverActor();

	// Interactable
	virtual bool CanInteract_Implementation() const override;
	virtual void OnDetected_Implementation(bool bDetected) override;
	virtual void Interact_Implementation(class ASonheimPlayer* Player) override;
	virtual FString GetInteractionName_Implementation() const override;
	virtual EInteractableType GetInteractableType_Implementation() const override { return EInteractableType::NPC; }

protected:
	// NPC identity used to validate quest start interactions.
	UPROPERTY(EditAnywhere, Category="Quest")
	int32 NpcID = 0;

	// Which quest to offer.
	UPROPERTY(EditAnywhere, Category="Quest")
	int32 QuestID = 0;

	// Offer validity window on the server.
	UPROPERTY(EditAnywhere, Category="Quest", meta=(ClampMin="1.0", ClampMax="120.0"))
	float OfferLifetimeSeconds = 20.f;

	// If true, accept immediately on interact (no UI prompt).
	UPROPERTY(EditAnywhere, Category="Quest")
	bool bAutoAccept = false;

	// If true, open quest journal on accept (requires UIStack setup).
	UPROPERTY(EditAnywhere, Category="Quest")
	bool bOpenJournalOnAccept = false;

	UPROPERTY(EditAnywhere, Category="Quest")
	FString InteractionLabel = TEXT("퀘스트 받기");
};
