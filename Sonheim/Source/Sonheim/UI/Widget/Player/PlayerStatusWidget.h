#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Sonheim/ResourceManager/SonheimGameType.h"
#include "Sonheim/UI/Widget/BaseStatusWidget.h"
#include "PlayerStatusWidget.generated.h"

class UProgressBar;
class UTextBlock;
class UImage;

UENUM(BlueprintType)
enum class EUIKeyGuide : uint8
{
	None UMETA(DisplayName = "None"), 
	LButton UMETA(DisplayName = "LButton"),
	RButton UMETA(DisplayName = "RButton"),
};

UCLASS()
class SONHEIM_API UPlayerStatusWidget : public UBaseStatusWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

public:
	UFUNCTION()
	void UpdateLevel(int32 OldLevel, int32 NewLevel, bool bLevelUp);

	UFUNCTION()
	void UpdateExp(int32 CurrentExp, int32 MaxExp, int32 Delta);
	
	UFUNCTION()
	void UpdateStamina(float CurrentStamina, float Delta, float MaxStamina);

	UFUNCTION()
	void SetEnableCrossHair(bool IsActive);

	UFUNCTION(BlueprintImplementableEvent, Category = "Key Guide")
	void SetEnableKeyGuide(bool IsActive, EUIKeyGuide UIKeyGuide, const FString& ShowText = "");

	UFUNCTION()
	void AddOwnedPal(int MonsterID, int Index);
	
	UFUNCTION()
	void SwitchSelectedPalIndex(int Index);
	void ClearOwnedPals();

	UFUNCTION()
	void OnItemAdded(int ItemID, int ItemCount);

	UFUNCTION(BlueprintImplementableEvent)
	void OnItemPopupDisplay(UTexture2D* ItemIcon,const FText& ItemName, int ItemCount, FLinearColor ItemRarityColor);

	// 포획 UI를 업데이트하는 함수.
	UFUNCTION(BlueprintImplementableEvent, Category = "Capture")
	void UpdateCaptureUI(const FCaptureUIInfo& UIData);
	
	// 무기 타입에 따라 크로스헤어 종류 변경
	UFUNCTION(BlueprintCallable, Category = "Crosshair")
	void SetCrosshairType(EWeaponType WeaponType);

	// 이동 속도에 따라 크로스헤어 스케일 업데이트
	UFUNCTION(BlueprintCallable, Category = "Crosshair")
	void UpdateCrosshairScale(float ScaleRatio);
	
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

	UPROPERTY(meta = (BindWidget))
	class UImage* ShotgunCrossHair;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	UWidgetAnimation* ZoomInByLockOn;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	UWidgetAnimation* ZoomOutByLockOn;

	UPROPERTY(meta = (BindWidgetAnim), Transient)
	UWidgetAnimation* ExpTextCycle = nullptr;

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

	UPROPERTY(EditAnywhere, Category="Crosshair|Scale")
	float CrosshairScaleMin = 1.0f;
	UPROPERTY(EditAnywhere, Category="Crosshair|Scale")
	float CrosshairScaleMax = 1.5f;
	UPROPERTY(EditAnywhere, Category="Crosshair|Scale")
	float CrosshairInterpSpeed = 10.0f;

private:
	int Level = 0;

	TWeakObjectPtr<class ASonheimPlayer> CachedPlayer;
	FDelegateHandle NewPawnHandle;

	void HandleNewPawn(class APawn* NewPawn);
	FORCEINLINE class ASonheimPlayer* GetPlayerFast() const { return CachedPlayer.Get(); }

	UPROPERTY()
	TMap<int,UImage*> PalSlots;
	UPROPERTY()
	TMap<int,UImage*> SelectBGs;

	float CrosshairTargetScale = 1.0f;
	float CrosshairCurrentScale = 1.0f;
};
