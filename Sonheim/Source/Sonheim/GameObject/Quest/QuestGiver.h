#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Sonheim/GameObject/InteractableInterface.h"
#include "Sonheim/Quest/QuestTypes.h"
#include "QuestGiver.generated.h"

class UAnimationAsset;
class UCapsuleComponent;
class USkeletalMeshComponent;
class UUserWidget;
class UWidgetComponent;

/**
 * A person on the island who gives and receives quests: the quest target QuestTargetName. F on them counts their Talk and Deliver
 * objectives on the server, then opens the quest dialog on the player's screen, where quests are accepted and reported.
 * The prompt above them says whether they have a quest to give or one to take back.
 */
UCLASS()
class SONHEIM_API AQuestGiver : public AActor, public IInteractableInterface, public IQuestTarget
{
	GENERATED_BODY()
public:
	AQuestGiver();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest") FName QuestTargetName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest") FText DisplayName;
	/** What F does, by what the person has for the looking player: a quest to give, one to take back, or neither. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest") FText OfferText;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest") FText ReportText;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest") FText TalkText;
	/** Looped on the mesh while the person stands there. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest") TObjectPtr<UAnimationAsset> IdleAnimation;
	/** Over the person's head while they have something for the looking player: a quest to give (!) or one to take back (?).
	 * Its Widget Blueprint names a TextBlock MarkText for the mark. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest") TSubclassOf<UUserWidget> MarkerWidgetClass;
	/** The server refuses a player farther than this, as other interactions do. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest") float InteractionDistance = 350.f;

	virtual FName GetQuestTargetName() const override { return QuestTargetName; }
	virtual bool CanInteract_Implementation() const override { return true; }
	virtual void Interact_Implementation(ASonheimPlayer* Player) override;
	virtual void OnDetected_Implementation(bool bDetected) override;
	virtual FString GetInteractionName_Implementation() const override { return DisplayName.ToString(); }
	virtual EInteractableType GetInteractableType_Implementation() const override { return EInteractableType::NPC; }
	virtual float GetHoldDuration_Implementation() const override { return 0.f; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UCapsuleComponent> Body;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USkeletalMeshComponent> Mesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UWidgetComponent> DetectWidgetComponent;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UWidgetComponent> MarkerWidgetComponent;
	void RefreshMarker();
	FTimerHandle MarkerTimer;
	UPROPERTY() TSubclassOf<UUserWidget> DetectWidgetClass;
};
