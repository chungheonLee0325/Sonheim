#pragma once

#include "CoreMinimal.h"
#include "Components/Overlay.h"
#include "GameplayTagContainer.h"
#include "GameUILayer.generated.h"

class UGameUIScreenWidget;

/** A layer of the root layout, placed in its Widget Blueprint and named by a tag. Widgets added to it fill it and all show, in the
 * order they came; it takes no part in the input mode or the back input (the HUD, the toasts). */
UCLASS()
class GAMEUIFRAMEWORK_API UGameUILayer : public UOverlay
{
	GENERATED_BODY()
public:
	UGameUILayer();
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Layer", meta=(Categories="UI.Layer")) FGameplayTag LayerTag;
	void AddLayerWidget(UWidget* Widget);
};

/** A layer of screens, newest on top. Only the top one shows; the ones under it stay open, hidden, and are told so. The top screen
 * of the highest stack layer decides the player's input mode and takes the back input. */
UCLASS()
class GAMEUIFRAMEWORK_API UGameUIStackLayer : public UGameUILayer
{
	GENERATED_BODY()
public:
	/** Adds the screen on top and opens it, or brings it to the top if it is in the stack already. */
	void PushScreen(UGameUIScreenWidget* Screen);
	/** Closes the screen and takes it out; the one under it shows again. */
	void RemoveScreen(UGameUIScreenWidget* Screen);
	UGameUIScreenWidget* GetTop() const { return Screens.IsEmpty() ? nullptr : Screens.Last().Get(); }
	const TArray<TObjectPtr<UGameUIScreenWidget>>& GetScreens() const { return Screens; }
private:
	void Cover(UGameUIScreenWidget* Screen);
	void Uncover(UGameUIScreenWidget* Screen);
	UPROPERTY(Transient) TArray<TObjectPtr<UGameUIScreenWidget>> Screens;
};
