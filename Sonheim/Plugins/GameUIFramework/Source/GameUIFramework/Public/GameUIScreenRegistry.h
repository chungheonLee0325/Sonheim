#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "GameUIScreenRegistry.generated.h"

class UGameUIScreenWidget;

USTRUCT(BlueprintType)
struct FGameUIScreenEntry
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(Categories="UI.Screen,UI.Popup")) FGameplayTag Tag;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftClassPtr<UGameUIScreenWidget> Class;
	/** Loaded, with the screen's preload assets, as the local player comes in, so the first opening does not wait for it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bPreload = false;
};

/** Every screen the UI manager can open, by tag. */
UCLASS(BlueprintType)
class GAMEUIFRAMEWORK_API UGameUIScreenRegistry : public UDataAsset
{
	GENERATED_BODY()
public:
	const FGameUIScreenEntry* Find(FGameplayTag Tag) const { return Screens.FindByPredicate([Tag](const FGameUIScreenEntry& E) { return E.Tag == Tag; }); }
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Screens", meta=(TitleProperty="Tag")) TArray<FGameUIScreenEntry> Screens;
};
