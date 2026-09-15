#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Sonheim/GameObject/InteractableInterface.h"
#include "DungeonShortcutSwitch.generated.h"
class ADungeonTestArea;
UCLASS()
class SONHEIM_API ADungeonShortcutSwitch : public AActor, public IInteractableInterface
{
	GENERATED_BODY()
public:
	ADungeonShortcutSwitch();
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") TObjectPtr<ADungeonTestArea> TestArea;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FName SourceId = TEXT("ShortcutSwitch");
	virtual bool CanInteract_Implementation() const override { return true; }
	virtual void Interact_Implementation(ASonheimPlayer* Player) override;
	virtual FString GetInteractionName_Implementation() const override { return TEXT("Unlock shortcut during Wave A"); }
	virtual EInteractableType GetInteractableType_Implementation() const override { return EInteractableType::Object; }
	virtual float GetHoldDuration_Implementation() const override { return 0.f; }
};
