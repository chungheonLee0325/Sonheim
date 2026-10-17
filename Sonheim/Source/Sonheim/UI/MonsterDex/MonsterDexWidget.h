#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MonsterDexView.h"
#include "MonsterDexWidget.generated.h"

class UBorder;
class UButton;
class UDynamicEntryBox;
class UImage;
class UProgressBar;
class UTextBlock;
class UMonsterDexComponent;
class AMonsterDexStage;
class UMaterialInterface;

/** A small rounded plate with an icon and a label: a reward of a tier, or an element of a species. */
UCLASS(Abstract)
class SONHEIM_API UMonsterDexChipWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	/** No icon collapses the icon slot. Fill colors the plate, Ink the label and the icon. */
	void Show(UTexture2D* Icon, const FText& Label, const FLinearColor& Fill, const FLinearColor& Ink, const FText& Tooltip = FText::GetEmpty());
protected:
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UBorder> Plate;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UImage> IconImage;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> LabelText;
};

/** One page in the book's grid: the number, the portrait (a silhouette until discovered), the name and the page's marks. */
UCLASS(Abstract)
class SONHEIM_API UMonsterDexCellWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	/** Picture is the species' still from the book's stage; without one the cell draws the species' icon. */
	void Show(const FMonsterDexPageView& Page, bool bSelected, UMaterialInterface* Picture);
	TDelegate<void(int32 AreaObjectID)> OnPicked;
protected:
	virtual void NativeOnInitialized() override;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> PickButton;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UBorder> Frame;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> NumberText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UImage> PortraitImage;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> NameText;
	/** Shown once the species was captured. */
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UImage> CapturedMark;
	/** Shown while a tier of the page can be claimed. */
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UImage> ReadyMark;

	UPROPERTY(EditAnywhere, Category="Style") FLinearColor FrameColor = FLinearColor(1.f, 1.f, 1.f, 0.1f);
	UPROPERTY(EditAnywhere, Category="Style") FLinearColor SelectedColor = FLinearColor(0.05f, 0.6f, 1.f, 1.f);
	UPROPERTY(EditAnywhere, Category="Style") FLinearColor SilhouetteColor = FLinearColor(0.f, 0.f, 0.f, 0.75f);
private:
	UFUNCTION() void HandleClicked();
	int32 AreaObjectID = 0;
};

/** One reward tier of a page: what it asks, how far the player is, what it pays, and the button that claims it. */
UCLASS(Abstract)
class SONHEIM_API UMonsterDexTierWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void Show(const FMonsterDexTierView& Tier);
	TDelegate<void(int32 Tier)> OnClaim;
protected:
	virtual void NativeOnInitialized() override;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UImage> ConditionIcon;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> ConditionText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> ProgressText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UProgressBar> ProgressBar;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UDynamicEntryBox> RewardBox;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> ClaimButton;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> ClaimText;

	UPROPERTY(EditAnywhere, Category="Style") TObjectPtr<UTexture2D> KillIcon;
	UPROPERTY(EditAnywhere, Category="Style") TObjectPtr<UTexture2D> CaptureIcon;
	UPROPERTY(EditAnywhere, Category="Style") FLinearColor ReadyColor = FLinearColor(1.f, 0.7f, 0.2f, 1.f);
	UPROPERTY(EditAnywhere, Category="Style") FLinearColor DoneColor = FLinearColor(0.55f, 0.64f, 0.75f, 1.f);
	UPROPERTY(EditAnywhere, Category="Style") FLinearColor ProgressColor = FLinearColor(0.05f, 0.6f, 1.f, 1.f);
	UPROPERTY(EditAnywhere, Category="Style") FLinearColor RewardColor = FLinearColor(1.f, 1.f, 1.f, 0.1f);
	/** The square of a reward's item slot. */
	UPROPERTY(EditAnywhere, Category="Style") float RewardSlotSize = 52.f;
private:
	UFUNCTION() void HandleClaim();
	int32 TierIndex = 0;
};

/**
 * The monster book: a grid of every page on the left, the picked page on the right with its records and reward tiers, and how
 * much of the book is filled at the top. It reads the local player's UMonsterDexComponent through MonsterDexView and draws
 * again whenever the component changes; claiming a tier asks the server.
 */
UCLASS(Abstract)
class SONHEIM_API UMonsterDexWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UMonsterDexWidget(const FObjectInitializer& ObjectInitializer);
	/** Picks a page, as a click on its cell does. */
	UFUNCTION(BlueprintCallable, Category="MonsterDex") void Pick(int32 AreaObjectID);
	UFUNCTION(BlueprintPure, Category="MonsterDex") int32 GetPicked() const { return Picked; }
	/** The book as the widget last drew it. */
	UFUNCTION(BlueprintPure, Category="MonsterDex") const FMonsterDexBookView& GetBook() const { return Book; }
	/** Claims a tier of the picked page, as its button does. */
	UFUNCTION(BlueprintCallable, Category="MonsterDex") void Claim(int32 Tier);
	/** Opens the map on the picked species' habitat, as its button does. */
	UFUNCTION(BlueprintCallable, Category="MonsterDex") void ShowHabitat();
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> CompletionText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UProgressBar> CompletionBar;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> CapturedText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UDynamicEntryBox> PageGrid;

	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> NumberText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> NameText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UImage> PortraitImage;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UDynamicEntryBox> ElementBox;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> DescriptionText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> KillCountText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> CaptureCountText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UDynamicEntryBox> TierBox;
	/** Where the picked species lives, by region name (??? for a region the player has not found). */
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> HabitatText;
	/** Opens the map on the species' habitat; off for a species not met yet. */
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> HabitatButton;

	UPROPERTY(EditAnywhere, Category="Style") TMap<EElementalAttribute, FLinearColor> ElementColors;
	UPROPERTY(EditAnywhere, Category="Style") FLinearColor SilhouetteColor = FLinearColor(0.f, 0.f, 0.f, 0.75f);
	/** A user interface material that draws the stage's render target, from its parameter Capture, with its background clear. */
	UPROPERTY(EditAnywhere, Category="Style") TObjectPtr<UMaterialInterface> StageMaterial;
	/** Degrees the species turns per unit of horizontal drag over the portrait. */
	UPROPERTY(EditAnywhere, Category="Style") float DragDegreesPerUnit = 0.6f;

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
private:
	UFUNCTION() void Redraw();
	void DrawPicked();
	UMonsterDexComponent* Dex() const;

	FMonsterDexBookView Book;
	int32 Picked = 0;
	UPROPERTY(Transient) TObjectPtr<AMonsterDexStage> Stage;
	/** A second stage that takes the grid's stills, away from the live one so neither's light reaches the other. */
	UPROPERTY(Transient) TObjectPtr<AMonsterDexStage> SnapStage;
	/** Each page's still, drawn with StageMaterial, in book order. */
	UPROPERTY(Transient) TArray<TObjectPtr<class UMaterialInstanceDynamic>> CellPictures;
	UPROPERTY(Transient) TObjectPtr<class UMaterialInstanceDynamic> StageDrawn;
	bool bDragging = false;
	TWeakObjectPtr<UMonsterDexComponent> Bound;
};
