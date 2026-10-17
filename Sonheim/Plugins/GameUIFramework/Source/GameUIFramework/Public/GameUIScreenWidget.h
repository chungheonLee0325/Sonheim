#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "GameUIScreenWidget.generated.h"

GAMEUIFRAMEWORK_API DECLARE_LOG_CATEGORY_EXTERN(LogGameUI, Log, All);

UENUM(BlueprintType)
enum class EGameUIInputMode : uint8
{
	GameOnly,
	/** Game input still arrives, so a screen's own key can close it; gameplay checks IsGameplayBlocked. */
	GameAndUI,
	UIOnly,
};

/** What the back input (Esc, gamepad B) does while the screen is on top. */
UENUM(BlueprintType)
enum class EGameUIBack : uint8
{
	Close,
	/** Nothing, and the input goes no further. */
	Ignore,
	/** NativeOnBack decides. */
	Custom,
};

/** A screen the manager opens by tag into a stack layer (UGameUIStackLayer). How it takes the player's input, where it goes and
 * whether it outlives its closing are its class defaults, set on the Widget Blueprint. The manager tells it when it opens and
 * closes; a kept screen opens again as the same widget, its Slate rebuilt, so NativeConstruct runs on every opening too. */
UCLASS(Abstract)
class GAMEUIFRAMEWORK_API UGameUIScreenWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	/** The layer the screen opens into; empty takes GetDefaultLayer. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Screen", meta=(Categories="UI.Layer"))
	FGameplayTag Layer;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Screen") EGameUIInputMode InputMode = EGameUIInputMode::GameAndUI;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Screen") bool bShowMouse = true;
	/** Whether moving, looking, attacking and the other gameplay input wait while the screen is on top. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Screen") bool bBlockGameInput = true;
	/** The widget outlives its closing and opens again as it was, for screens opened often whose making costs a frame. What an
	 * opening takes (bindings, render targets, brushes on them, pooled entries) must be let go in NativeOnClosed. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Screen") bool bKeepWhenClosed = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Screen") EGameUIBack Back = EGameUIBack::Close;

	/** The registry tag the screen was opened by. */
	FGameplayTag GetScreenTag() const { return ScreenTag; }
	/** Closes this screen through its player's UI manager. */
	UFUNCTION(BlueprintCallable, Category="Screen") void CloseSelf();
	/** The layer of a screen whose class names none: UGameUISettings::DefaultScreenLayer. */
	virtual FGameplayTag GetDefaultLayer() const;
	/** Assets to load with the class before the first widget is made, read from the class default object. */
	virtual void GatherPreloadAssets(TArray<FSoftObjectPath>& OutPaths) const {}

protected:
	/** Just after the screen is added to its layer, on every opening. */
	virtual void NativeOnOpened() { OnOpened(); }
	/** Just before the screen leaves its layer, on every closing. */
	virtual void NativeOnClosed() { OnClosed(); }
	/** Another screen of the same layer opened above this one, which is hidden until it closes. */
	virtual void NativeOnCovered() {}
	virtual void NativeOnUncovered() {}
	/** The back input with Back == Custom; returns whether it was handled. */
	virtual bool NativeOnBack() { return false; }
	UFUNCTION(BlueprintImplementableEvent, Category="Screen") void OnOpened();
	UFUNCTION(BlueprintImplementableEvent, Category="Screen") void OnClosed();

private:
	friend class UGameUIStackLayer;
	friend class UGameUIManagerSubsystem;
	FGameplayTag ScreenTag;
	ESlateVisibility UncoveredVisibility = ESlateVisibility::Visible;
};
