#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Sonheim/GameObject/InteractableInterface.h"

#include "QuestTurnInActor.generated.h"

UCLASS()
class SONHEIM_API AQuestTurnInActor : public AActor, public IInteractableInterface
{
	GENERATED_BODY()

public:
	AQuestTurnInActor();

	// Interactable
	virtual bool CanInteract_Implementation() const override;
	virtual void OnDetected_Implementation(bool bDetected) override;
	virtual void Interact_Implementation(class ASonheimPlayer* Player) override;
	virtual FString GetInteractionName_Implementation() const override;
	virtual EInteractableType GetInteractableType_Implementation() const override { return EInteractableType::Object; }

protected:
	// NPC identity used to validate quest turn-in interactions.
	UPROPERTY(EditAnywhere, Category="Quest")
	int32 NpcID = 0;

	// Which quest to turn in.
	UPROPERTY(EditAnywhere, Category="Quest")
	int32 QuestID = 0;

	// Offer validity window on the server.
	UPROPERTY(EditAnywhere, Category="Quest", meta=(ClampMin="1.0", ClampMax="120.0"))
	float OfferLifetimeSeconds = 20.f;

	UPROPERTY(EditAnywhere, Category="Quest")
	FString InteractionLabel = TEXT("퀘스트 반납");
};
