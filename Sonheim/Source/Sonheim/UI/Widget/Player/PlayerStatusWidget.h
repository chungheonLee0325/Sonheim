#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Sonheim/UI/Widget/BaseStatusWidget.h"
#include "PlayerStatusWidget.generated.h"

class UProgressBar;
class UTextBlock;
class UImage;
class UCanvasPanel;
class UBorder;
class ABaseMonster;

// 팰 슬롯 UI 데이터
USTRUCT(BlueprintType)
struct FPalSlotUI
{
	GENERATED_BODY()

	UPROPERTY()
	UImage* Icon = nullptr;
	
	UPROPERTY()
	UProgressBar* HealthBar = nullptr;
	
	UPROPERTY()
	UBorder* SelectBorder = nullptr;
	
	UPROPERTY()
	UTextBlock* LevelText = nullptr;
};

UCLASS()
class SONHEIM_API UPlayerStatusWidget : public UBaseStatusWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

public:
	// === 기존 함수들 ===
	UFUNCTION()
	void UpdateLevel(int32 OldLevel, int32 NewLevel, bool bLevelUp);

	UFUNCTION()
	void UpdateExp(int32 CurrentExp, int32 MaxExp, int32 Delta);
	
	UFUNCTION()
	void UpdateStamina(float CurrentStamina, float Delta, float MaxStamina);

	UFUNCTION()
	void SetEnableCrossHair(bool IsActive);

	// === 개선된 팰 관리 UI ===
	UFUNCTION(BlueprintCallable, Category = "Pal UI")
	void AddOwnedPal(int MonsterID, int Index);
	
	UFUNCTION(BlueprintCallable, Category = "Pal UI")
	void RemoveOwnedPal(int Index);
	
	UFUNCTION(BlueprintCallable, Category = "Pal UI")
	void UpdatePalHealth(int Index, float HealthPercent);
	
	UFUNCTION(BlueprintCallable, Category = "Pal UI")
	void UpdatePalLevel(int Index, int Level);
	
	UFUNCTION(BlueprintCallable, Category = "Pal UI")
	void SwitchSelectedPalIndex(int Index);

	// === 포획 확률 UI ===
	UFUNCTION(BlueprintCallable, Category = "Capture UI")
	void ShowCaptureRate(float CaptureRate);
	
	UFUNCTION(BlueprintCallable, Category = "Capture UI")
	void HideCaptureRate();
	
	UFUNCTION(BlueprintCallable, Category = "Capture UI")
	void UpdateCaptureRateDisplay(class ABaseMonster* Target);

	// === 포획 결과 UI ===
	UFUNCTION(BlueprintCallable, Category = "Capture UI")
	void ShowCaptureSuccess(const FString& PalName);
	
	UFUNCTION(BlueprintCallable, Category = "Capture UI")
	void ShowCaptureFailed();

	void UpdateTargetInfo();
protected:
	// === 플레이어 상태 UI ===
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

	// === 크로스헤어 ===
	UPROPERTY(meta = (BindWidget))
	UImage* CrossHair;

	// === 팰 슬롯 UI (개선) ===
	// 아이콘
	UPROPERTY(meta = (BindWidget))
	UImage* PalSlot0;
	UPROPERTY(meta = (BindWidget))
	UImage* PalSlot1;
	UPROPERTY(meta = (BindWidget))
	UImage* PalSlot2;
	UPROPERTY(meta = (BindWidget))
	UImage* PalSlot3;
	UPROPERTY(meta = (BindWidget))
	UImage* PalSlot4;
	
	// 체력바 (새로 추가)
	UPROPERTY(meta = (BindWidget))
	UProgressBar* PalHealth0;
	UPROPERTY(meta = (BindWidget))
	UProgressBar* PalHealth1;
	UPROPERTY(meta = (BindWidget))
	UProgressBar* PalHealth2;
	UPROPERTY(meta = (BindWidget))
	UProgressBar* PalHealth3;
	UPROPERTY(meta = (BindWidget))
	UProgressBar* PalHealth4;
	
	// 선택 테두리
	UPROPERTY(meta = (BindWidget))
	UBorder* SelectBG0;
	UPROPERTY(meta = (BindWidget))
	UBorder* SelectBG1;
	UPROPERTY(meta = (BindWidget))
	UBorder* SelectBG2;
	UPROPERTY(meta = (BindWidget))
	UBorder* SelectBG3;
	UPROPERTY(meta = (BindWidget))
	UBorder* SelectBG4;
	
	// 레벨 텍스트 (새로 추가)
	UPROPERTY(meta = (BindWidget))
	UTextBlock* PalLevel0;
	UPROPERTY(meta = (BindWidget))
	UTextBlock* PalLevel1;
	UPROPERTY(meta = (BindWidget))
	UTextBlock* PalLevel2;
	UPROPERTY(meta = (BindWidget))
	UTextBlock* PalLevel3;
	UPROPERTY(meta = (BindWidget))
	UTextBlock* PalLevel4;

	// === 포획 확률 UI (새로 추가) ===
	UPROPERTY(meta = (BindWidget))
	UCanvasPanel* CaptureRatePanel;
	
	UPROPERTY(meta = (BindWidget))
	UTextBlock* CaptureRateText;
	
	UPROPERTY(meta = (BindWidget))
	UProgressBar* CaptureRateBar;
	
	UPROPERTY(meta = (BindWidget))
	UTextBlock* TargetNameText;
	
	UPROPERTY(meta = (BindWidget))
	UTextBlock* TargetLevelText;

	// === 포획 결과 UI (새로 추가) ===
	UPROPERTY(meta = (BindWidget))
	UCanvasPanel* CaptureResultPanel;
	
	UPROPERTY(meta = (BindWidget))
	UTextBlock* CaptureResultText;
	
	UPROPERTY(meta = (BindWidget))
	UImage* CaptureResultIcon;

	// === 애니메이션 ===
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	UWidgetAnimation* ZoomInByLockOn;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	UWidgetAnimation* ZoomOutByLockOn;
	
	// 새 애니메이션 추가
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	UWidgetAnimation* CaptureRateFadeIn;
	
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	UWidgetAnimation* CaptureRateFadeOut;
	
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	UWidgetAnimation* CaptureSuccessAnim;
	
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	UWidgetAnimation* CaptureFailedAnim;
	
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	UWidgetAnimation* PalSlotPulse;

private:
	// 내부 데이터
	UPROPERTY()
	TArray<FPalSlotUI> PalSlotUIArray;
	
	// 포획 확률 업데이트 타이머
	FTimerHandle CaptureRateUpdateTimer;
	
	// 현재 타겟 몬스터
	UPROPERTY()
	TWeakObjectPtr<ABaseMonster> CurrentTargetMonster;
	
	// 헬퍼 함수
	void InitializePalSlots();
	FLinearColor GetHealthBarColor(float HealthPercent) const;
	
	// 포획 확률에 따른 색상
	FLinearColor GetCaptureRateColor(float Rate) const;
};