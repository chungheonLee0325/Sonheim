#include "CaptureRateWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/BackgroundBlur.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Overlay.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/HorizontalBox.h"
#include "Components/VerticalBox.h"
#include "Components/CircularThrobber.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Animation/WidgetAnimation.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Engine/World.h"
#include "Kismet/KismetMathLibrary.h"

void UCaptureRateWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	// 원형 프로그레스 머티리얼 설정
	if (CircularProgressFill)
	{
		UMaterialInterface* ProgressMaterial = LoadObject<UMaterialInterface>(nullptr, 
			TEXT("/Game/UI/Materials/M_CircularProgress_Segmented"));
		
		if (ProgressMaterial)
		{
			CircularProgressMID = UMaterialInstanceDynamic::Create(ProgressMaterial, this);
			CircularProgressFill->SetBrushFromMaterial(CircularProgressMID);
			
			// 초기 파라미터 설정
			CircularProgressMID->SetScalarParameterValue("Progress", 0.0f);
			CircularProgressMID->SetScalarParameterValue("SegmentCount", 24.0f); // 팰월드 스타일 세그먼트
			CircularProgressMID->SetScalarParameterValue("SegmentGap", 0.02f);
			CircularProgressMID->SetScalarParameterValue("Thickness", 0.06f);
			CircularProgressMID->SetScalarParameterValue("StartAngle", -90.0f); // 12시 방향부터
		}
	}
	
	// 글로우 효과 설정
	if (PalSphereGlow)
	{
		UMaterialInterface* GlowMaterial = LoadObject<UMaterialInterface>(nullptr, 
			TEXT("/Game/UI/Materials/M_UI_Glow_Radial"));
		
		if (GlowMaterial)
		{
			GlowMID = UMaterialInstanceDynamic::Create(GlowMaterial, this);
			PalSphereGlow->SetBrushFromMaterial(GlowMID);
		}
	}
	
	// 스캔라인 효과 설정
	if (ScanlineEffect)
	{
		UMaterialInterface* ScanlineMaterial = LoadObject<UMaterialInterface>(nullptr, 
			TEXT("/Game/UI/Materials/M_UI_Scanline"));
		
		if (ScanlineMaterial)
		{
			ScanlineMID = UMaterialInstanceDynamic::Create(ScanlineMaterial, this);
			ScanlineEffect->SetBrushFromMaterial(ScanlineMID);
			ScanlineEffect->SetRenderOpacity(0.3f);
		}
	}
	
	// 홀로그램 효과 설정
	if (HologramEffect)
	{
		UMaterialInterface* HologramMaterial = LoadObject<UMaterialInterface>(nullptr, 
			TEXT("/Game/UI/Materials/M_UI_Hologram"));
		
		if (HologramMaterial)
		{
			HologramMID = UMaterialInstanceDynamic::Create(HologramMaterial, this);
			HologramEffect->SetBrushFromMaterial(HologramMID);
			HologramEffect->SetRenderOpacity(0.2f);
		}
	}
	
	// 백그라운드 블러 설정
	if (BackgroundBlur)
	{
		BackgroundBlur->SetBlurStrength(8.0f);
		BackgroundBlur->SetBlurRadius(4.0f);
	}
	
	// 초기 애니메이션 시작
	if (RingRotateAnim)
	{
		PlayAnimation(RingRotateAnim, 0.0f, 0, EUMGSequencePlayMode::Forward, 1.0f, true);
	}
	
	// 비네팅 효과 설정
	if (VignetteEffect)
	{
		VignetteEffect->SetRenderOpacity(0.6f);
	}
	
	// 초기 상태 설정
	UpdateVisuals(0.0f);
}

void UCaptureRateWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	
	// 부드러운 보간
	CurrentRate = FMath::FInterpTo(CurrentRate, TargetRate, InDeltaTime, RateInterpSpeed);
	
	// 표시되는 숫자 업데이트 (더 빠른 보간)
	DisplayedRate = FMath::FInterpTo(DisplayedRate, TargetRate, InDeltaTime, RateInterpSpeed * 1.5f);
	
	// 비주얼 업데이트
	UpdateCircularProgress(CurrentRate);
	UpdateTextDisplay(DisplayedRate);
	UpdateDecorations(InDeltaTime);
	UpdateEffects(InDeltaTime);
	
	// 타겟 정보 위치 업데이트
	UpdateTargetInfoPosition();
}

void UCaptureRateWidget::SetCaptureRate(float Rate)
{
	float ClampedRate = FMath::Clamp(Rate, 0.0f, 1.0f);
	
	// 이전 카테고리 저장
	int32 OldCategory = GetRateCategory(TargetRate);
	int32 NewCategory = GetRateCategory(ClampedRate);
	
	TargetRate = ClampedRate;
	
	// 카테고리가 변경되면 애니메이션 재생
	if (OldCategory != NewCategory)
	{
		PlayRateCategoryAnimation(NewCategory);
	}
	
	// 색상 즉시 업데이트
	UpdateColorScheme(ClampedRate);
	
	// 높은 포획률일 때 특별 효과
	if (ClampedRate > 0.8f && HighRateAnim)
	{
		PlayAnimation(HighRateAnim, 0.0f, 0, EUMGSequencePlayMode::Forward, 1.0f, true);
	}
	else if (ClampedRate < 0.3f && LowRateAnim)
	{
		PlayAnimation(LowRateAnim, 0.0f, 0, EUMGSequencePlayMode::Forward, 1.0f, true);
	}
	else
	{
		StopAnimation(HighRateAnim);
		StopAnimation(LowRateAnim);
	}
}

void UCaptureRateWidget::SetTargetInfo(const FString& Name, int32 Level)
{
	if (TargetNameText)
	{
		TargetNameText->SetText(FText::FromString(Name));
	}
	
	if (TargetLevelText)
	{
		TargetLevelText->SetText(FText::FromString(FString::Printf(TEXT("Lv.%d"), Level)));
	}
	
	// 타겟 정보 슬라이드 애니메이션
	if (TargetInfoSlideAnim)
	{
		PlayAnimation(TargetInfoSlideAnim);
	}
}

void UCaptureRateWidget::UpdateVisuals(float Rate)
{
	UpdateCircularProgress(Rate);
	UpdateColorScheme(Rate);
	UpdateTextDisplay(Rate);
}

void UCaptureRateWidget::UpdateCircularProgress(float Rate)
{
	if (!CircularProgressMID)
		return;
	
	// 기본 진행도
	CircularProgressMID->SetScalarParameterValue("Progress", Rate);
	
	// 색상 설정
	FLinearColor ProgressColor = GetCaptureRateColor(Rate);
	CircularProgressMID->SetVectorParameterValue("ProgressColor", ProgressColor);
	CircularProgressMID->SetVectorParameterValue("BackgroundColor", ProgressColor * 0.15f);
	
	// 발광 강도
	float GlowIntensity = FMath::Lerp(0.5f, 2.5f, Rate);
	CircularProgressMID->SetScalarParameterValue("GlowIntensity", GlowIntensity);
	
	// 애니메이션 시간
	float Time = GetWorld()->GetTimeSeconds();
	CircularProgressMID->SetScalarParameterValue("Time", Time);
	
	// 세그먼트 펄스 효과
	float PulseSpeed = FMath::Lerp(1.0f, 3.0f, 1.0f - Rate); // 낮은 확률일수록 빠름
	CircularProgressMID->SetScalarParameterValue("PulseSpeed", PulseSpeed);
	
	// 활성 세그먼트 업데이트
	int32 ActiveSegments = CalculateActiveSegments(Rate);
	UpdateSegmentDisplay(ActiveSegments);
	
	// 프로그레스 글로우 업데이트
	if (ProgressGlow)
	{
		ProgressGlow->SetRenderOpacity(GlowIntensity * 0.3f);
		ProgressGlow->SetColorAndOpacity(ProgressColor);
	}
}

void UCaptureRateWidget::UpdateColorScheme(float Rate)
{
	FLinearColor Color = GetCaptureRateColor(Rate);
	
	// 텍스트 색상
	if (CaptureRateText)
	{
		CaptureRateText->SetColorAndOpacity(Color);
		
		// 그림자 효과
		FSlateFontInfo FontInfo = CaptureRateText->GetFont();
		FontInfo.OutlineSettings.OutlineSize = FMath::Lerp(1, 3, Rate);
		FontInfo.OutlineSettings.OutlineColor = Color * 0.5f;
		CaptureRateText->SetFont(FontInfo);
	}
	
	// 퍼센트 기호
	if (PercentSymbol)
	{
		PercentSymbol->SetColorAndOpacity(Color * 0.8f);
	}
	
	// 타겟 정보 박스
	if (TargetInfoBox)
	{
		TargetInfoBox->SetBrushColor(Color * 0.2f);
	}
	
	if (TargetInfoBackground)
	{
		TargetInfoBackground->SetColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.8f));
	}
	
	// 장식 요소
	if (OuterRingStatic)
	{
		OuterRingStatic->SetColorAndOpacity(Color * 0.4f);
	}
	
	if (OuterRingRotating)
	{
		OuterRingRotating->SetColorAndOpacity(Color * 0.6f);
	}
	
	if (InnerRingRotating)
	{
		InnerRingRotating->SetColorAndOpacity(Color * 0.8f);
	}
	
	// 코너 장식
	FLinearColor CornerColor = Color * 0.5f;
	if (CornerDecoration_TL) CornerDecoration_TL->SetColorAndOpacity(CornerColor);
	if (CornerDecoration_TR) CornerDecoration_TR->SetColorAndOpacity(CornerColor);
	if (CornerDecoration_BL) CornerDecoration_BL->SetColorAndOpacity(CornerColor);
	if (CornerDecoration_BR) CornerDecoration_BR->SetColorAndOpacity(CornerColor);
	
	// 글로우 효과
	if (GlowMID)
	{
		GlowMID->SetVectorParameterValue("GlowColor", Color);
		GlowMID->SetScalarParameterValue("GlowIntensity", FMath::Lerp(1.0f, 4.0f, Rate));
		GlowMID->SetScalarParameterValue("PulseSpeed", FMath::Lerp(2.0f, 0.5f, Rate));
	}
	
	// 팰 스피어 아이콘 색상
	if (PalSphereIcon)
	{
		// 아이콘은 항상 밝게 유지
		PalSphereIcon->SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
	}
}

void UCaptureRateWidget::UpdateTextDisplay(float Rate)
{
	if (!CaptureRateText)
		return;
	
	int32 Percentage = FMath::RoundToInt(Rate * 100.0f);
	CaptureRateText->SetText(FText::AsNumber(Percentage));
	
	// 숫자 변경 애니메이션
	if (FMath::Abs(DisplayedRate - PreviousDisplayedRate) > 0.01f)
	{
		if (NumberChangeAnim)
		{
			PlayAnimation(NumberChangeAnim);
		}
		
		// 큰 변화일 때 스케일 효과
		float ScaleMultiplier = FMath::Clamp(FMath::Abs(DisplayedRate - PreviousDisplayedRate) * 5.0f, 1.0f, 1.3f);
		CaptureRateText->SetRenderScale(FVector2D(ScaleMultiplier));
	}
	else
	{
		// 스케일 복구
		FVector2D CurrentScale = CaptureRateText->GetRenderTransform().Scale;
		FVector2D TargetScale = FVector2D(1.0f);
		CaptureRateText->SetRenderScale(FMath::Vector2DInterpTo(CurrentScale, TargetScale, GetWorld()->GetDeltaSeconds(), 10.0f));
	}
	
	PreviousDisplayedRate = DisplayedRate;
}

void UCaptureRateWidget::UpdateTargetInfoPosition()
{
	if (!TargetInfoBox)
		return;
	
	// 타겟 정보는 원형 프로그레스 위에 위치
	// 부드러운 움직임을 위한 미세한 애니메이션
	float Time = GetWorld()->GetTimeSeconds();
	float YOffset = FMath::Sin(Time * 2.0f) * 2.0f;
	
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(TargetInfoBox->Slot))
	{
		FVector2D Position = CanvasSlot->GetPosition();
		Position.Y = -120.0f + YOffset; // 기본 위치 + 부드러운 움직임
		CanvasSlot->SetPosition(Position);
	}
}

void UCaptureRateWidget::UpdateDecorations(float DeltaTime)
{
	// 링 회전
	RotationAngle += DeltaTime * 20.0f; // 초당 20도
	if (RotationAngle > 360.0f)
		RotationAngle -= 360.0f;
	
	if (OuterRingRotating)
	{
		OuterRingRotating->SetRenderTransformAngle(RotationAngle);
	}
	
	if (InnerRingRotating)
	{
		InnerRingRotating->SetRenderTransformAngle(-RotationAngle * 1.5f); // 반대 방향, 더 빠르게
	}
	
	// 펄스 애니메이션
	PulseTimer += DeltaTime;
	
	// 중앙 글로우 펄스
	if (PalSphereGlow)
	{
		float PulseValue = (FMath::Sin(PulseTimer * 2.0f) + 1.0f) * 0.5f;
		float Alpha = FMath::Lerp(0.4f, 1.0f, PulseValue);
		PalSphereGlow->SetRenderOpacity(Alpha);
		
		// 크기 펄스
		float Scale = FMath::Lerp(0.95f, 1.05f, PulseValue);
		PalSphereGlow->SetRenderScale(FVector2D(Scale));
	}
	
	// 타겟 화살표 애니메이션
	if (TargetInfoArrow)
	{
		float ArrowBounce = FMath::Sin(PulseTimer * 3.0f) * 3.0f;
		TargetInfoArrow->SetRenderTranslation(FVector2D(0, ArrowBounce));
	}
}

void UCaptureRateWidget::UpdateEffects(float DeltaTime)
{
	// 스캔라인 효과
	if (ScanlineMID)
	{
		ScanlineOffset += DeltaTime * 0.5f;
		if (ScanlineOffset > 1.0f)
			ScanlineOffset -= 1.0f;
		
		ScanlineMID->SetScalarParameterValue("ScanlineOffset", ScanlineOffset);
		ScanlineMID->SetScalarParameterValue("ScanlineSpeed", 0.5f);
		ScanlineMID->SetScalarParameterValue("ScanlineThickness", 0.002f);
	}
	
	// 홀로그램 노이즈
	if (HologramMID)
	{
		float Time = GetWorld()->GetTimeSeconds();
		HologramMID->SetScalarParameterValue("Time", Time);
		HologramMID->SetScalarParameterValue("NoiseIntensity", 0.02f);
		HologramMID->SetScalarParameterValue("ChromaticAberration", 0.005f);
		
		// 낮은 포획률일 때 더 강한 노이즈
		float NoiseMultiplier = FMath::Lerp(2.0f, 1.0f, CurrentRate);
		HologramMID->SetScalarParameterValue("NoiseMultiplier", NoiseMultiplier);
	}
}

FLinearColor UCaptureRateWidget::GetCaptureRateColor(float Rate) const
{
	// 팰월드 스타일 정확한 색상 (스크린샷 분석 기반)
	if (Rate > 0.9f)
		return FLinearColor(0.0f, 1.0f, 0.4f, 1.0f); // 밝은 녹색
	else if (Rate > 0.8f)
		return FLinearColor(0.2f, 1.0f, 0.3f, 1.0f); // 녹색
	else if (Rate > 0.7f)
		return FLinearColor(0.4f, 1.0f, 0.2f, 1.0f); // 연두색
	else if (Rate > 0.6f)
		return FLinearColor(0.6f, 1.0f, 0.1f, 1.0f); // 노란연두
	else if (Rate > 0.5f)
		return FLinearColor(0.8f, 1.0f, 0.0f, 1.0f); // 노란색 (스크린샷의 79%)
	else if (Rate > 0.4f)
		return FLinearColor(1.0f, 0.9f, 0.0f, 1.0f); // 진한 노란색
	else if (Rate > 0.3f)
		return FLinearColor(1.0f, 0.7f, 0.0f, 1.0f); // 주황노란색
	else if (Rate > 0.2f)
		return FLinearColor(1.0f, 0.5f, 0.0f, 1.0f); // 주황색
	else if (Rate > 0.1f)
		return FLinearColor(1.0f, 0.3f, 0.0f, 1.0f); // 붉은주황색
	else
		return FLinearColor(1.0f, 0.15f, 0.0f, 1.0f); // 빨간색
}

int32 UCaptureRateWidget::GetRateCategory(float Rate) const
{
	if (Rate > 0.8f) return 4; // 매우 높음
	else if (Rate > 0.6f) return 3; // 높음
	else if (Rate > 0.4f) return 2; // 보통
	else if (Rate > 0.2f) return 1; // 낮음
	else return 0; // 매우 낮음
}

void UCaptureRateWidget::PlayRateCategoryAnimation(int32 Category)
{
	// 카테고리별 특별 애니메이션
	switch (Category)
	{
	case 4: // 매우 높음
		if (HighRateAnim)
		{
			PlayAnimation(HighRateAnim);
		}
		break;
		
	case 0: // 매우 낮음
		if (LowRateAnim)
		{
			PlayAnimation(LowRateAnim);
		}
		break;
		
	default:
		if (PulseAnim)
		{
			float PlayRate = FMath::Lerp(0.5f, 2.0f, 1.0f - (Category / 4.0f));
			PlayAnimation(PulseAnim, 0.0f, 0, EUMGSequencePlayMode::Forward, PlayRate, true);
		}
		break;
	}
}

int32 UCaptureRateWidget::CalculateActiveSegments(float Rate) const
{
	// 24개 세그먼트 기준 (팰월드 스타일)
	return FMath::RoundToInt(Rate * 24.0f);
}

void UCaptureRateWidget::UpdateSegmentDisplay(int32 ActiveSegments)
{
	if (!ProgressSegments || !CircularProgressMID)
		return;
	
	// 세그먼트별 애니메이션
	CircularProgressMID->SetScalarParameterValue("ActiveSegments", (float)ActiveSegments);
	
	// 마지막 세그먼트 강조 효과
	float LastSegmentGlow = FMath::Sin(GetWorld()->GetTimeSeconds() * 4.0f) * 0.5f + 0.5f;
	CircularProgressMID->SetScalarParameterValue("LastSegmentGlow", LastSegmentGlow);
}