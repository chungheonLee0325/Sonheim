#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MenuTabWidget.generated.h"

class UBorder;
class UButton;
class UDynamicEntryBox;
class UImage;
class UTextBlock;
class UTexture2D;

/** One tab of the menu: the UIStack screen it opens, and its name and glyph. A tab with no screen yet is drawn dimmed. */
USTRUCT(BlueprintType)
struct FMenuTab
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Screen;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FText Label;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UTexture2D> Icon;
};

/** A tab of the bar; a click opens its screen in place of the one showing. */
UCLASS(Abstract)
class SONHEIM_API UMenuTabEntryWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void Show(const FMenuTab& InTab, bool bActive);
protected:
	virtual void NativeOnInitialized() override;
	UFUNCTION() void Clicked();
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> TabButton;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UImage> IconImage;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> LabelText;
	/** Filled in the accent colour under the active tab's glyph and name, with a small mark below it. */
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UBorder> ActivePlate;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UWidget> ActiveMark;
	UPROPERTY(EditAnywhere, Category="Style") FLinearColor Accent = FLinearColor(0.0252f, 0.5776f, 1.f);
	UPROPERTY(EditAnywhere, Category="Style") FLinearColor Text = FLinearColor::White;
	UPROPERTY(EditAnywhere, Category="Style") FLinearColor TextOnActive = FLinearColor(0.003f, 0.008f, 0.019f);
	UPROPERTY(EditAnywhere, Category="Style") FLinearColor Dimmed = FLinearColor(0.35f, 0.45f, 0.55f, 0.55f);
private:
	FMenuTab Tab;
};

/** The bar across the top of the menu's screens (inventory, monster book, quest journal): every tab, the showing one lit, and the
 * Q and E keys that step between them. UUIStackSubsystem shows it while a tab's screen is on top. */
UCLASS(Abstract)
class SONHEIM_API UMenuTabBarWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void ShowActive(FName Screen);
	/** The tabs in order, set on the Widget Blueprint. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Tabs") TArray<FMenuTab> Tabs;
protected:
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UDynamicEntryBox> TabBox;
};
