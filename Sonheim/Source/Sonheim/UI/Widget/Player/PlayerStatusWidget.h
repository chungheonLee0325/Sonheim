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

private:
	int Level = 0;
};
