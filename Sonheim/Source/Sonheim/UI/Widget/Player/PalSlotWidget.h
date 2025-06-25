#pragma once

#include "CoreMinimal.h"
#include "PlayerStatusWidget.h"
#include "Blueprint/UserWidget.h"
#include "Sonheim/ResourceManager/SonheimGameType.h"
#include "PalSlotWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSlotClicked, int32, SlotIndex);

class UImage;
class UTextBlock;
class UButton;
class UProgressBar;
class UOverlay;
class UBorder;
class USizeBox;
class UWidgetAnimation;
class UMaterialInstanceDynamic;

UCLASS()
class SONHEIM_API UPalSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// === 초기화 ===
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	
	// === 델리게이트 ===
	UPROPERTY(BlueprintAssignable, Category = "Pal Slot")
	FOnSlotClicked OnSlotClicked;

	// === 공개 함수 ===
	UFUNCTION(BlueprintCallable, Category = "Pal Slot")
	void SetSlotIndex(int32 Index);
	
	UFUNCTION(BlueprintCallable, Category = "Pal Slot")
	void SetPalData(const struct FPalSlotData& InPalData);
	
	UFUNCTION(BlueprintCallable, Category = "Pal Slot")
	void SetEmpty();
	
	UFUNCTION(BlueprintCallable, Category = "Pal Slot")
	void SetSelected(bool bIsSelected);
	
	UFUNCTION(BlueprintCallable, Category = "Pal Slot")
	void SetHighlighted(bool bIsHighlighted);
	
	UFUNCTION(BlueprintCallable, Category = "Pal Slot")
	void UpdateHealth(float HealthPercent);
	
	UFUNCTION(BlueprintCallable, Category = "Pal Slot")
	void UpdateLevel(int32 Level);
	
	UFUNCTION(BlueprintCallable, Category = "Pal Slot")
	void PlayAddedAnimation();
	
	UFUNCTION(BlueprintCallable, Category = "Pal Slot")
	void PlayRemovedAnimation();
	
	UFUNCTION(BlueprintCallable, Category = "Pal Slot")
	void PlayDamagedAnimation();
	
	UFUNCTION(BlueprintCallable, Category = "Pal Slot")
	void PlayHealedAnimation();

protected:
	// === UI 구조 ===
	UPROPERTY(meta = (BindWidget))
	USizeBox* RootSizeBox;
	
	UPROPERTY(meta = (BindWidget))
	UOverlay* MainOverlay;
	
	// 배경 및 프레임
	UPROPERTY(meta = (BindWidget))
	UBorder* SlotBorder;
	
	UPROPERTY(meta = (BindWidget))
	UImage* SlotBackground;
	
	UPROPERTY(meta = (BindWidget))
	UImage* SlotFrame;
	
	UPROPERTY(meta = (BindWidget))
	UImage* RarityFrame;
	
	UPROPERTY(meta = (BindWidget))
	UImage* SelectionHighlight;
	
	UPROPERTY(meta = (BindWidget))
	UImage* HoverHighlight;
	
	// 팰 정보
	UPROPERTY(meta = (BindWidget))
	UImage* PalIcon;
	
	UPROPERTY(meta = (BindWidget))
	UImage* PalSilhouette;
	
	UPROPERTY(meta = (BindWidget))
	UTextBlock* SlotNumberText;
	
	UPROPERTY(meta = (BindWidget))
	UTextBlock* PalLevelText;
	
	// 상태 표시
	UPROPERTY(meta = (BindWidget))
	UProgressBar* HealthBar;
	
	UPROPERTY(meta = (BindWidget))
	UImage* HealthBarFrame;
	
	UPROPERTY(meta = (BindWidget))
	UImage* StatusIcon;
	
	// 효과
	UPROPERTY(meta = (BindWidget))
	UImage* GlowEffect;
	
	UPROPERTY(meta = (BindWidget))
	UImage* ShineEffect;
	
	// 버튼
	UPROPERTY(meta = (BindWidget))
	UButton* SlotButton;
	
	// === 애니메이션 ===
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	UWidgetAnimation* HoverAnim;
	
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	UWidgetAnimation* SelectAnim;
	
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	UWidgetAnimation* AddAnim;
	
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	UWidgetAnimation* RemoveAnim;
	
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	UWidgetAnimation* DamageAnim;
	
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	UWidgetAnimation* HealAnim;
	
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	UWidgetAnimation* PulseAnim;

private:
	// === 내부 데이터 ===
	int32 SlotIndex = -1;
	bool bIsEmpty = true;
	bool bIsSelected = false;
	bool bIsHovered = false;
	
	UPROPERTY()
	FPalSlotData CurrentPalData;
	
	UPROPERTY()
	UMaterialInstanceDynamic* HealthBarMID;
	
	UPROPERTY()
	UMaterialInstanceDynamic* GlowMID;
	
	// === 내부 함수 ===
	UFUNCTION()
	void OnSlotButtonClicked();
	
	UFUNCTION()
	void OnSlotButtonHovered();
	
	UFUNCTION()
	void OnSlotButtonUnhovered();
	
	void UpdateVisualState();
	void UpdateRarityDisplay(EPalRarity Rarity);
	FLinearColor GetRarityColor(EPalRarity Rarity) const;
	void SetupHealthBarMaterial();
};