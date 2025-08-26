// CaptureProgressWidget.cpp
#include "CaptureProgressWidget.h"
#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Components/TextBlock.h"

void UCaptureProgressWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ProgressImage)
	{
		if (UMaterialInterface* Base = Cast<UMaterialInterface>(ProgressImage->GetBrush().GetResourceObject()))
		{
			MID = UMaterialInstanceDynamic::Create(Base, this);
			ProgressImage->SetBrushFromMaterial(MID);
		}
	}

	SetProgressParameter(0.f);
	UpdateColorTextByProgress(0.f);
}

void UCaptureProgressWidget::PlayCaptureProgressReveal(float Guess01, bool bSuccess, int32 InSegments,
													 float InSegmentTime, float InInterStageDelay,
													 float InStartDelay, int32 FailStageOverride)
{
	Guess = FMath::Clamp(Guess01, 0.f, 1.f);
	Segments = FMath::Clamp(InSegments, 1, 10);
	SegmentTime = FMath::Max(0.05f, InSegmentTime);
	bFinalSuccess = bSuccess;
	if (InInterStageDelay >= 0.f)
	{
		InterStageDelay = InInterStageDelay;
	}
	StartDelay = InStartDelay;

	// 세그먼트 경계 계산: Guess에서 시작 → (1-Guess)/Segments씩 증가 → 1.0
	Boundaries.Empty();
	Boundaries.Add(Guess);
	for (int i = 1; i < Segments; ++i)
	{
		const float t = Guess + (1.f - Guess) * (float(i) / (float)Segments);
		Boundaries.Add(t);
	}
	Boundaries.Add(1.f);

	// 실패 시 멈출 스테이지: round(Guess * Segments), 상한 Segments-1
	int32 FailStage = FMath::Clamp(FMath::RoundToInt(Guess * Segments), 0, Segments - 1);
	if (!bFinalSuccess && FailStageOverride >= 0)
	{
		FailStage = FMath::Clamp(FailStageOverride, 0, Segments - 1);
	}
	FinalStage = bFinalSuccess ? Segments : FailStage;

	// 시작 프레임: Guess까지 즉시 채움 + 색 반영 → StartDelay 대기 → 스테이지 시작 or 즉시 종료
	SetProgressParameter(Guess);
	UpdateColorTextByProgress(Guess);

	// 실패 즉시종료(=FinalStage==0)라도 StartDelay만큼은 보여주고 끝내자
	FTimerDelegate BeginDel;
	BeginDel.BindLambda([this]()
	{
		if (FinalStage == 0)
		{
			OnRevealFinished.Broadcast(false);
			UpdateFailResult();
			return;
		}
		CurrentStage = 0;
		StartStage(CurrentStage);
	});
	GetWorld()->GetTimerManager().SetTimer(DelayHandle, BeginDel, StartDelay, false);
}

void UCaptureProgressWidget::StartStage(int32 StageIndex)
{
	if (StageIndex >= FinalStage)
	{
		OnRevealFinished.Broadcast(bFinalSuccess);
		if (!bFinalSuccess) UpdateFailResult();
		return;
	}

	StageStartTime = GetWorld()->GetRealTimeSeconds();
	GetWorld()->GetTimerManager().SetTimer(TickHandle, this, &UCaptureProgressWidget::TickStage, 0.016f, true);
}

void UCaptureProgressWidget::TickStage()
{
	const double Now = GetWorld()->GetRealTimeSeconds();
	const float t = FMath::Clamp(float((Now - StageStartTime) / SegmentTime), 0.f, 1.f);

	const float P0 = (CurrentStage == 0) ? Guess : Boundaries[CurrentStage];
	const float P1 = Boundaries[CurrentStage + 1];
	const float P = FMath::Lerp(P0, P1, t);

	SetProgressParameter(P);
	UpdateColorTextByProgress(P);

	if (t >= 1.f)
	{
		GetWorld()->GetTimerManager().ClearTimer(TickHandle);

		OnSegmentFilled.Broadcast(CurrentStage);
		
		// 다음 스테이지로 진행
		++CurrentStage;
		if (CurrentStage < FinalStage)
		{
			// 세그먼트 사이 딜레이 후 다음 스테이지
			FTimerDelegate Del;
			Del.BindUObject(this, &UCaptureProgressWidget::StartStage, CurrentStage);
			GetWorld()->GetTimerManager().SetTimer(DelayHandle, Del, InterStageDelay, false);
		}
		else
		{
			// 끝
			OnRevealFinished.Broadcast(bFinalSuccess);
			if (!bFinalSuccess) UpdateFailResult();
		}
	}
}

void UCaptureProgressWidget::SetProgressParameter(float P)
{
	const float Clamped = FMath::Clamp(P, 0.f, 1.f);

	if (MID && ProgressParamName != NAME_None)
	{
		MID->SetScalarParameterValue(ProgressParamName, Clamped);
	}
	else if (ProgressImage)
	{
		ProgressImage->SetOpacity(1.f);
	}
}

void UCaptureProgressWidget::UpdateColorTextByProgress(float P)
{
	// 0~0.25: Red, 0.25~0.5: Orange, 0.5~0.75: Yellow, 0.75~1.0: Green
	const FLinearColor Red    = FLinearColor(1.f, 0.16f, 0.07f, 1.f);
	const FLinearColor Orange = FLinearColor(1.f, 0.55f, 0.08f, 1.f);
	const FLinearColor Yellow = FLinearColor(1.f, 0.9f,  0.1f,  1.f);
	const FLinearColor Green  = FLinearColor(0.12f, 0.9f, 0.2f, 1.f);
	const FLinearColor Blue  = FLinearColor(0.12f, 0.6f, 1.f, 1.f);

	FLinearColor C;
	if (P <= 0.25f)         C = Red;
	else if (P <= 0.50f)    C = Orange;
	else if (P <= 0.75f)    C = Yellow;
	else if (P <= 0.95f)	C = Green;
	else					C = Blue;

	if (MID && FillColorParamName != NAME_None)
	{
		MID->SetVectorParameterValue(FillColorParamName, C);
	}
	else if (ProgressImage)
	{
		ProgressImage->SetColorAndOpacity(C);
	}

	if (capturerateArrow)
	{
		capturerateArrow->SetColorAndOpacity(C);
	}
	if (CaptureRateText)
	{
		CaptureRateText->SetText(FText::FromString(FString::Printf(TEXT("%d"), static_cast<int>(P * 100))));
	}
}

void UCaptureProgressWidget::UpdateFailResult()
{
	if (MID && FillColorParamName != NAME_None)
	{
		MID->SetVectorParameterValue(FillColorParamName, FLinearColor::Red);
	}

	if (capturerateArrow)
	{
		capturerateArrow->SetColorAndOpacity(FLinearColor::Red);
	}
	if (CaptureRateText)
	{
		CaptureRateText->SetColorAndOpacity(FLinearColor::Red);
	}
}
