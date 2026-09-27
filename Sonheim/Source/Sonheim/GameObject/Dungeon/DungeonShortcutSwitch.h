#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Sonheim/GameObject/InteractableInterface.h"
#include "DungeonShortcutSwitch.generated.h"
class ADungeonTestArea;
class USoundBase;
class UStaticMeshComponent;
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
	/** Name shown in the Detect prompt. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FText DisplayName;
	/** What F does, after the name. The server takes the lever only from the player who started the run, so everyone else sees
	 * OwnerOnlyText instead. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FText ActionText = INVTEXT("당기기");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FText OwnerOnlyText = INVTEXT("원정대장만 당길 수 있음");
	/** Stage whose rules accept this switch (Stage_Combat, Wave A, in the demo definition). The prompt shows only while the run is in it; None shows it during the whole run. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FName PromptStageId = TEXT("Stage_Combat");
	/** Run flag the lever's rule sets; the handle stays pulled while the run has it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FName PulledTagName = TEXT("Dungeon.State.ShortcutUnlocked");
	/** The floor lies this far below the actor, whose origin is the middle of the interaction box. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") float FloorBelow = 100.f;
	/** Where the handle turns, from the base's origin on the floor, and its turn when pulled. The handle mesh's origin is on the axle. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FVector HandlePivotOffset = FVector(0.f, 0.f, 48.f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") FRotator PulledRotation = FRotator(0.f, 0.f, 56.f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") float PullSeconds = 0.35f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") TObjectPtr<USoundBase> PullSound;
	bool IsPulled() const { return bPulled; }
	virtual bool CanInteract_Implementation() const override { return true; }
	virtual void Interact_Implementation(ASonheimPlayer* Player) override;
	virtual void OnDetected_Implementation(bool bDetected) override;
	virtual FString GetInteractionName_Implementation() const override { return DisplayName.ToString(); }
	virtual EInteractableType GetInteractableType_Implementation() const override { return EInteractableType::Object; }
	virtual float GetHoldDuration_Implementation() const override { return 0.f; }
protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
private:
	/** Shown while the run is running in PromptStageId; the snapshot's status and stage decide, no other server check is repeated. */
	void RefreshPrompt();
	/** Pulls the handle while the run has PulledTagName, and lets it back for the next run. */
	void RefreshHandle(bool bAnimate);
	void HandleStageState(const FDungeonStageRuntimeState& State);
	UPROPERTY(VisibleAnywhere) TObjectPtr<UWidgetComponent> DetectWidgetComponent;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Base;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> HandlePivot;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Handle;
	UPROPERTY() TSubclassOf<UUserWidget> DetectWidgetClass;
	FDelegateHandle StageStateHandle;
	bool bDetected = false;
	bool bPulled = false;
	float PullAlpha = 0.f;
};
