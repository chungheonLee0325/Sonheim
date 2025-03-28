#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Sonheim/UI/Widget/BaseStatusWidget.h"
#include "PlayerStatusWidget.generated.h"

class UProgressBar;
class UTextBlock;
class UImage;

UCLASS()
class SONHEIM_API UPlayerStatusWidget : public UBaseStatusWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

public:
	UFUNCTION()
	void UpdateLevel(int32 OldLevel, int32 NewLevel, bool bLevelUp);

	UFUNCTION()
	void UpdateExp(int32 CurrentExp, int32 MaxExp, int32 Delta);
	
	UFUNCTION()
	void UpdateStamina(float CurrentStamina, float Delta, float MaxStamina);

	UFUNCTION()
	void SetEnableCrossHair(bool IsActive);

	UFUNCTION()
	void AddOwnedPal(int MonsterID, int Index);
	
	UFUNCTION()
	void SwitchSelectedPalIndex(int Index);

protected:
	UPROPERTY(meta = (BindWidget))
	UProgressBar* StaminaBar;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* StaminaText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* LevelText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* ExpText;

	UPROPERTY(meta = (BindWidget))
	UProgressBar* ExpBar;

	UPROPERTY(meta = (BindWidget))
	class UImage* CrossHair;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	UWidgetAnimation* ZoomInByLockOn;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	UWidgetAnimation* ZoomOutByLockOn;

	UPROPERTY(meta = (BindWidget))
	class UImage* PalSlot0;
	UPROPERTY(meta = (BindWidget))
	class UImage* PalSlot1;
	UPROPERTY(meta = (BindWidget))
	class UImage* PalSlot2;
	UPROPERTY(meta = (BindWidget))
	class UImage* PalSlot3;
	UPROPERTY(meta = (BindWidget))
	class UImage* PalSlot4;
	UPROPERTY(meta = (BindWidget))
	class UImage* SelectBG0;
	UPROPERTY(meta = (BindWidget))
	class UImage* SelectBG1;
	UPROPERTY(meta = (BindWidget))
	class UImage* SelectBG2;
	UPROPERTY(meta = (BindWidget))
	class UImage* SelectBG3;
	UPROPERTY(meta = (BindWidget))
	class UImage* SelectBG4;

private:
	int Level = 0;

	UPROPERTY()
	TMap<int,UImage*> PalSlots;
	UPROPERTY()
	TMap<int,UImage*> SelectBGs;
};
