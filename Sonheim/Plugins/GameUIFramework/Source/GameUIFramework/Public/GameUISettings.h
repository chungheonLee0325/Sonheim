#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"
#include "GameUIToastSubsystem.h"
#include "GameUISettings.generated.h"

class UGameUIRootWidget;
class UGameUIScreenRegistry;

/** Project Settings > Game > Game UI, saved in DefaultGame.ini. */
UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Game UI"))
class GAMEUIFRAMEWORK_API UGameUISettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	virtual FName GetCategoryName() const override { return TEXT("Game"); }
	UPROPERTY(Config, EditAnywhere, Category="Layout") TSoftClassPtr<UGameUIRootWidget> RootClass;
	/** The root's place among other widgets on the player's screen, if the game adds any. */
	UPROPERTY(Config, EditAnywhere, Category="Layout") int32 RootZOrder = 0;
	UPROPERTY(Config, EditAnywhere, Category="Screens") TSoftObjectPtr<UGameUIScreenRegistry> Registry;
	/** The layer of a screen whose class names none. */
	UPROPERTY(Config, EditAnywhere, Category="Screens", meta=(Categories="UI.Layer")) FGameplayTag DefaultScreenLayer;
	/** The layer of a popup whose class names none. */
	UPROPERTY(Config, EditAnywhere, Category="Screens", meta=(Categories="UI.Layer")) FGameplayTag DefaultPopupLayer;
	/** The layer the toasts' widgets fill. */
	UPROPERTY(Config, EditAnywhere, Category="Toasts", meta=(Categories="UI.Layer")) FGameplayTag ToastLayer;
	/** The toast slots, in the order their widgets are drawn, back to front. */
	UPROPERTY(Config, EditAnywhere, Category="Toasts", meta=(TitleProperty="Tag")) TArray<FGameUIToastSlot> ToastSlots;
};
