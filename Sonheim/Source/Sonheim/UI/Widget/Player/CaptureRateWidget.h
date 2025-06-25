#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CaptureRateWidget.generated.h"

class UImage;
class UTextBlock;
class UBackgroundBlur;
class UMaterialInstanceDynamic;
class UWidgetAnimation;
class UCanvasPanel;
class UOverlay;
class UBorder;
class USizeBox;
class UHorizontalBox;
class UVerticalBox;
class UCircularThrobber;

UCLASS()
class SONHEIM_API UCaptureRateWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// === 초기화 ===
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// === 공개 함수 ===
	UFUNCTION(BlueprintCallable, Category = "Capture Rate")
	void SetCaptureRate(float Rate);
	
	UFUNCTION(BlueprintCallable, Category = "Capture Rate")
	void SetTargetInfo(const FString& Name, int32 Level);
	
	UFUNCTION(BlueprintImplementableEvent, Category = "Capture Rate")
	void PlayPulseAnimation();
	
	UFUNCTION(BlueprintCallable, Category = "Capture Rate")
	void UpdateVisuals(float Rate);

protected:
	// === 메인 컨테이너 ===
	UPROPERTY(meta = (BindWidget))
	UCanvasPanel* RootCanvas;
	
	UPROPERTY(meta = (BindWidget))
	UOverlay* MainOverlay;
	
	// === 백그라운드 효과 ===
	UPROPERTY(meta = (BindWidget))
	UBackgroundBlur* BackgroundBlur;
	
	UPROPERTY(meta = (BindWidget))
	UImage* VignetteEffect;
	
	UPROPERTY(meta = (BindWidget))
	UImage* RadialGradient;
	
	// === 원형 프로그레스 ===
	UPROPERTY(meta = (BindWidget))
	USizeBox* CircularProgressContainer;
	
	UPROPERTY(meta = (BindWidget))
	UImage* CircularProgressBackground;
	
	UPROPERTY(meta = (BindWidget))
	UImage* CircularProgressFill;
	
	UPROPERTY(meta = (BindWidget))
	UImage* CircularProgressFrame;
	
	// 세그먼트 표시용 (팰월드 스타일)
	UPROPERTY(meta = (BindWidget))
	UImage* ProgressSegments;
	
	UPROPERTY(meta = (BindWidget))
	UImage* ProgressGlow;
	
	// === 중앙 요소 ===
	UPROPERTY(meta = (BindWidget))
	UOverlay* CenterContainer;
	
	UPROPERTY(meta = (BindWidget))
	UImage* PalSphereIcon;
	
	UPROPERTY(meta = (BindWidget))
	UImage* PalSphereGlow;
	
	UPROPERTY(meta = (BindWidget))
	UTextBlock* CaptureRateText;
	
	UPROPERTY(meta = (BindWidget))
	UImage* PercentSymbol;
	
	// === 타겟 정보 박스 ===
	UPROPERTY(meta = (BindWidget))
	UBorder* TargetInfoBox;
	
	UPROPERTY(meta = (BindWidget))
	UHorizontalBox* TargetInfoContent;
	
	UPROPERTY(meta = (BindWidget))
	UTextBlock* TargetNameText;
	
	UPROPERTY(meta = (BindWidget))
	UTextBlock* TargetLevelText;
	
	UPROPERTY(meta = (BindWidget))
	UImage* TargetInfoBackground;
	
	UPROPERTY(meta = (BindWidget))
	UImage* TargetInfoArrow;
	
	// === 장식 요소 ===
	UPROPERTY(meta = (BindWidget))
	UImage* OuterRingStatic;
	
	UPROPERTY(meta = (BindWidget))
	UImage* OuterRingRotating;
	
	UPROPERTY(meta = (BindWidget))
	UImage* InnerRingRotating;
	
	UPROPERTY(meta = (BindWidget))
	UImage* CornerDecoration_TL;
	
	UPROPERTY(meta = (BindWidget))
	UImage* CornerDecoration_TR;
	
	UPROPERTY(meta = (BindWidget))
	UImage* CornerDecoration_BL;
	
	UPROPERTY(meta = (BindWidget))
	UImage* CornerDecoration_BR;
	
	// 추가 이펙트
	UPROPERTY(meta = (BindWidget))
	UImage* ScanlineEffect;
	
	UPROPERTY(meta = (BindWidget))
	UImage* HologramEffect;
	
	// === 애니메이션 ===
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	UWidgetAnimation* FadeInAnim;
	
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	UWidgetAnimation* FadeOutAnim;
	
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	UWidgetAnimation* PulseAnim;
	
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	UWidgetAnimation* RingRotateAnim;
	
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	UWidgetAnimation* NumberChangeAnim;
	
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	UWidgetAnimation* HighRateAnim;
	
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	UWidgetAnimation* LowRateAnim;
	
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	UWidgetAnimation* TargetInfoSlideAnim;

private:
	// === 머티리얼 인스턴스 ===
	UPROPERTY()
	UMaterialInstanceDynamic* CircularProgressMID;
	
	UPROPERTY()
	UMaterialInstanceDynamic* GlowMID;
	
	UPROPERTY()
	UMaterialInstanceDynamic* ScanlineMID;
	
	UPROPERTY()
	UMaterialInstanceDynamic* HologramMID;
	
	// === 내부 변수 ===
	float CurrentRate = 0.0f;
	float TargetRate = 0.0f;
	float DisplayedRate = 0.0f; // 텍스트에 표시되는 값
	float RateInterpSpeed = 8.0f;
	
	// 애니메이션 변수
	float PulseTimer = 0.0f;
	float RotationAngle = 0.0f;
	float ScanlineOffset = 0.0f;
	
	// 이전 값 추적
	float PreviousDisplayedRate = 0.0f;
	int32 PreviousRateCategory = -1; // 색상 카테고리 추적
	
	// === 헬퍼 함수 ===
	void UpdateCircularProgress(float Rate);
	void UpdateColorScheme(float Rate);
	void UpdateTextDisplay(float Rate);
	void UpdateTargetInfoPosition();
	void UpdateDecorations(float DeltaTime);
	void UpdateEffects(float DeltaTime);
	
	FLinearColor GetCaptureRateColor(float Rate) const;
	int32 GetRateCategory(float Rate) const;
	void PlayRateCategoryAnimation(int32 Category);
	
	// 세그먼트 계산
	int32 CalculateActiveSegments(float Rate) const;
	void UpdateSegmentDisplay(int32 ActiveSegments);
};