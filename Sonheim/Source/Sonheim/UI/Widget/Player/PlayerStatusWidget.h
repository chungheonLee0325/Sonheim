#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Sonheim/ResourceManager/SonheimGameType.h"
#include "PlayerStatusWidget.generated.h"

// Forward declarations
class UWidgetSwitcher;
class UHorizontalBox;
class UVerticalBox;
class UOverlay;
class UImage;
class UTextBlock;
class UProgressBar;
class UButton;
class UBorder;
class UBackgroundBlur;
class URetainerBox;
class UInvalidationBox;
class ABaseMonster;
class ASonheimPlayer;
class UPalSlotWidget;
class UCaptureRateWidget;
class USoundBase;
class UWidgetAnimation;
class UMaterialInstanceDynamic;

// 조준 모드 enum
UENUM(BlueprintType)
enum class EPalAimMode : uint8
{
	Normal = 0,      // 일반 크로스헤어
	PalSphere = 1,   // 팰 스피어 조준
	CaptureRate = 2  // 포획률 표시
};

// 포획 진행 상태
UENUM(BlueprintType)
enum class ECaptureUIState : uint8
{
	None,
	Throwing,
	Shaking,
	Success,
	Failed
};

// 팰 슬롯 데이터
USTRUCT(BlueprintType)
struct FPalSlotData
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite)
	int32 PalID = 0;
	
	UPROPERTY(BlueprintReadWrite)
	UTexture2D* Icon = nullptr;
	
	UPROPERTY(BlueprintReadWrite)
	FString Name;
	
	UPROPERTY(BlueprintReadWrite)
	int32 Level = 1;
	
	UPROPERTY(BlueprintReadWrite)
	float HealthPercent = 1.0f;
	
	UPROPERTY(BlueprintReadWrite)
	EPalRarity Rarity = EPalRarity::Common;
	
	UPROPERTY(BlueprintReadWrite)
	bool bIsEmpty = true;
};

UCLASS()
class SONHEIM_API UPlayerStatusWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// === 초기화 ===
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// === 플레이어 상태 UI ===
	UFUNCTION()
	void UpdateHealth(float CurrentHealth, float DeltaHealth, float MaxHealth);
	
	UFUNCTION()
	void UpdateStamina(float CurrentStamina, float DeltaStamina, float MaxStamina);
	
	UFUNCTION()
	void UpdateLevel(int32 OldLevel, int32 NewLevel, bool bIsInitialized);
	
	UFUNCTION()
	void UpdateExp(int32 CurrentExp, int32 ExpToNextLevel, int32 DeltaExp);
	
	// 별도 호출용 (바인딩 없이 직접 호출)
	UFUNCTION(BlueprintCallable, Category = "Player Status")
	void UpdateHealthBar(float HealthPercent);
	
	UFUNCTION(BlueprintCallable, Category = "Player Status")
	void UpdateStaminaBar(float StaminaPercent);

	// === 조준 모드 전환 ===
	UFUNCTION(BlueprintCallable, Category = "Aim Mode")
	void SetAimMode(EPalAimMode NewMode, bool bInstant = false);

	UFUNCTION(BlueprintCallable, Category = "Aim Mode")
	void SetEnableCrossHair(bool IsActive);
	
	UFUNCTION(BlueprintCallable, Category = "Aim Mode")
	void OnPalSphereEquipped(int32 SphereItemID);
	
	UFUNCTION(BlueprintCallable, Category = "Aim Mode")
	void OnPalSphereUnequipped();

	// === 포획률 표시 ===
	UFUNCTION(BlueprintCallable, Category = "Capture")
	void ShowCaptureRate(class ABaseMonster* Target, float Rate);
	
	UFUNCTION(BlueprintCallable, Category = "Capture")
	void HideCaptureRate();
	
	UFUNCTION(BlueprintCallable, Category = "Capture")
	void UpdateTargetInfo();
	
	UFUNCTION(BlueprintCallable, Category = "Capture")
	void UpdateCaptureProgress(ECaptureUIState State, float Progress = 0.0f);
	
	UFUNCTION(BlueprintCallable, Category = "Capture")
	void ShowCaptureResult(bool bSuccess, const FString& PalName = "");
	
	UFUNCTION(BlueprintCallable, Category = "Capture")
	void ShowCaptureSuccess(const FString& PalName);
	
	UFUNCTION(BlueprintCallable, Category = "Capture")
	void ShowCaptureFailed();

	// === 팰 슬롯 관리 ===
	UFUNCTION(BlueprintCallable, Category = "Pal Slots")
	void InitializePalSlots(int32 MaxSlots = 5);
	
	UFUNCTION(BlueprintCallable, Category = "Pal Slots")
	void AddOwnedPal(int32 PalID, int32 SlotIndex);
	
	UFUNCTION(BlueprintCallable, Category = "Pal Slots")
	void AddPalToSlot(int32 SlotIndex, const FPalSlotData& PalData);
	
	UFUNCTION(BlueprintCallable, Category = "Pal Slots")
	void RemovePalFromSlot(int32 SlotIndex);
	
	UFUNCTION(BlueprintCallable, Category = "Pal Slots")
	void UpdatePalSlot(int32 SlotIndex, float HealthPercent, int32 Level);
	
	UFUNCTION(BlueprintCallable, Category = "Pal Slots")
	void UpdatePalHealth(int32 SlotIndex, float HealthPercent);
	
	UFUNCTION(BlueprintCallable, Category = "Pal Slots")
	void UpdatePalLevel(int32 SlotIndex, int32 Level);
	
	UFUNCTION()
	void SelectPalSlot(int32 SlotIndex);
	
	UFUNCTION(BlueprintCallable, Category = "Pal Slots")
	void SwitchSelectedPalIndex(int32 NewIndex);
	
	UFUNCTION(BlueprintCallable, Category = "Pal Slots")
	void HighlightPalSlot(int32 SlotIndex, bool bHighlight);

	// === 알림 시스템 ===
	UFUNCTION(BlueprintCallable, Category = "Notifications")
	void ShowNotification(const FText& Message, float Duration = 3.0f);
	
	UFUNCTION(BlueprintCallable, Category = "Notifications")
	void ShowItemPickup(int32 ItemID, int32 Count);

	// === 유틸리티 ===
	UFUNCTION(BlueprintPure, Category = "UI")
	EPalAimMode GetCurrentAimMode() const { return CurrentAimMode; }
	
	UFUNCTION(BlueprintPure, Category = "UI")
	bool IsCapturing() const { return CurrentCaptureState != ECaptureUIState::None; }

protected:
	// === 메인 UI 구조 ===
	// 중앙 조준점 스위처
	UPROPERTY(meta = (BindWidget))
	UWidgetSwitcher* AimModeSwitcher;
	
	// 각 모드별 위젯
	UPROPERTY(meta = (BindWidget))
	UUserWidget* W_NormalCrosshair;
	
	UPROPERTY(meta = (BindWidget))
	UUserWidget* W_PalSphereAim;
	
	UPROPERTY(meta = (BindWidget))
	UCaptureRateWidget* W_CaptureRate;
	
	// === 플레이어 상태 UI ===
	UPROPERTY(meta = (BindWidget))
	UProgressBar* HealthBar;
	
	UPROPERTY(meta = (BindWidget))
	UProgressBar* StaminaBar;
	
	UPROPERTY(meta = (BindWidget))
	UTextBlock* LevelText;
	
	UPROPERTY(meta = (BindWidget))
	UProgressBar* ExpBar;
	
	UPROPERTY(meta = (BindWidget))
	UTextBlock* ExpText;
	
	// === 팰 슬롯 UI ===
	UPROPERTY(meta = (BindWidget))
	UHorizontalBox* PalSlotContainer;
	
	UPROPERTY(meta = (BindWidget))
	UInvalidationBox* PalSlotInvalidationBox;
	
	// === 포획 진행 UI ===
	UPROPERTY(meta = (BindWidget))
	UOverlay* CaptureProgressOverlay;
	
	UPROPERTY(meta = (BindWidget))
	UImage* CaptureProgressSphere;
	
	UPROPERTY(meta = (BindWidget))
	UTextBlock* CaptureProgressText;
	
	UPROPERTY(meta = (BindWidget))
	UProgressBar* CaptureProgressBar;
	
	// === 알림 UI ===
	UPROPERTY(meta = (BindWidget))
	UVerticalBox* NotificationContainer;
	
	// === 위젯 클래스 ===
	UPROPERTY(EditDefaultsOnly, Category = "UI Classes")
	TSubclassOf<UPalSlotWidget> PalSlotWidgetClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "UI Classes")
	TSubclassOf<UUserWidget> NotificationWidgetClass;
	
	// === 애니메이션 ===
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	UWidgetAnimation* PalSphereEquipAnim;
	
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	UWidgetAnimation* PalSphereUnequipAnim;
	
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	UWidgetAnimation* CaptureRateFadeInAnim;
	
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	UWidgetAnimation* CaptureRateFadeOutAnim;
	
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	UWidgetAnimation* CaptureShakeAnim;
	
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	UWidgetAnimation* CaptureSuccessAnim;
	
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	UWidgetAnimation* CaptureFailAnim;
	
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	UWidgetAnimation* ItemPickupAnim;
	
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	UWidgetAnimation* LevelUpAnim;
	
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	UWidgetAnimation* ExpGainAnim;
	
	// === 사운드 ===
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	USoundBase* PalSphereEquipSound;
	
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	USoundBase* PalSphereUnequipSound;
	
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	USoundBase* CaptureStartSound;
	
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	USoundBase* CaptureShakeSound;
	
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	USoundBase* CaptureSuccessSound;
	
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	USoundBase* CaptureFailSound;
	
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	USoundBase* UISelectSound;
	
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	USoundBase* UIHoverSound;

private:
	// === 내부 상태 ===
	EPalAimMode CurrentAimMode = EPalAimMode::Normal;
	ECaptureUIState CurrentCaptureState = ECaptureUIState::None;
	
	// 위젯 인스턴스
	UPROPERTY()
	TArray<UPalSlotWidget*> PalSlotWidgets;
	
	// 타겟 추적
	UPROPERTY()
	TWeakObjectPtr<ABaseMonster> CurrentTarget;
	
	// 현재 선택된 슬롯
	int32 CurrentSelectedPalSlot = -1;
	
	// 타이머 핸들
	FTimerHandle AimModeTransitionTimer;
	FTimerHandle CaptureRateUpdateTimer;
	FTimerHandle NotificationCleanupTimer;
	
	// === 헬퍼 함수 ===
	void CreatePalSlots();
	void UpdateAimingTarget();
	bool IsTargetInRange(ABaseMonster* Target) const;
	float CalculateCaptureRate(ABaseMonster* Target) const;
	FLinearColor GetCaptureRateColor(float Rate) const;
	FLinearColor GetHealthBarColor(float HealthPercent) const;
	void PlayModeTransition(EPalAimMode FromMode, EPalAimMode ToMode);
	void CleanupOldNotifications();
	
	// 캐시된 참조
	UPROPERTY()
	ASonheimPlayer* CachedPlayer;
	
	UPROPERTY()
	class USonheimGameInstance* CachedGameInstance;
};