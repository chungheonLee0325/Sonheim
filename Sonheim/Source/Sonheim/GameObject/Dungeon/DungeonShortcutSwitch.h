#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Sonheim/GameObject/InteractableInterface.h"
#include "DungeonShortcutSwitch.generated.h"
class ADungeonTestArea;
class UUserWidget;
class UWidgetComponent;
struct FDungeonStageRuntimeState;
UCLASS()
class SONHEIM_API ADungeonShortcutSwitch : public AActor, public IInteractableInterface
{
	GENERATED_BODY()
public:
	ADungeonShortcutSwitch();
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") TObjectPtr<ADungeonTestArea> TestArea;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FName SourceId = TEXT("ShortcutSwitch");
	/** Name shown in the Detect prompt. The server accepts the switch only from the player who started the run, and the name says so. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FText DisplayName;
	/** Stage whose rules accept this switch (Stage_Combat, Wave A, in the demo definition). The prompt shows only while the run is in it; None shows it during the whole run. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FName PromptStageId = TEXT("Stage_Combat");
	virtual bool CanInteract_Implementation() const override { return true; }
	virtual void Interact_Implementation(ASonheimPlayer* Player) override;
	virtual void OnDetected_Implementation(bool bDetected) override;
	virtual FString GetInteractionName_Implementation() const override { return DisplayName.ToString(); }
	virtual EInteractableType GetInteractableType_Implementation() const override { return EInteractableType::Object; }
	virtual float GetHoldDuration_Implementation() const override { return 0.f; }
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
	/** Shown while the run is running in PromptStageId; the snapshot's status and stage decide, no other server check is repeated. */
	void RefreshPrompt();
	void HandleStageState(const FDungeonStageRuntimeState& State);
	UPROPERTY(VisibleAnywhere) TObjectPtr<UWidgetComponent> DetectWidgetComponent;
	UPROPERTY() TSubclassOf<UUserWidget> DetectWidgetClass;
	FDelegateHandle StageStateHandle;
	bool bDetected = false;
};
