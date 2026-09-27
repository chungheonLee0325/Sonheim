#include "DungeonToastWidget.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"

void UDungeonToastWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	ApplyStyle();
}

void UDungeonToastWidget::ApplyStyle()
{
	const auto* Theme = Style ? Style.Get() : GetDefault<UDungeonToastStyle>();
	const FLinearColor Accent = ViewData.Tone == EDungeonToastTone::Route ? Theme->Route :
		ViewData.Tone == EDungeonToastTone::Warning ? Theme->Warning : Theme->Information;
	if (Card) Card->SetBrushColor(Theme->Surface);
	if (AccentLine) AccentLine->SetBrushColor(Accent);
	if (IconPlate) IconPlate->SetBrushColor(FLinearColor(Accent.R * 0.13f, Accent.G * 0.13f, Accent.B * 0.13f, 1.f));
	if (SymbolText) SymbolText->SetColorAndOpacity(Accent);
	if (CategoryText) CategoryText->SetColorAndOpacity(Accent);
	if (TitleText) TitleText->SetColorAndOpacity(Theme->Text);
	if (DetailText) DetailText->SetColorAndOpacity(Theme->Muted);
	if (LifetimeBar) LifetimeBar->SetFillColorAndOpacity(Accent);
}

void UDungeonToastWidget::ShowToast(const FDungeonToastViewData& Data)
{
	ViewData = Data;
	Age = 0.f; bPlaying = true;
	SetVisibility(ESlateVisibility::HitTestInvisible);
	CategoryText->SetText(Data.Category);
	TitleText->SetText(Data.Title);
	DetailText->SetText(Data.Detail);
	SymbolText->SetText(Data.Symbol);
	ApplyStyle();
	SetRenderOpacity(0.f);
	LifetimeBar->SetPercent(1.f);
}

void UDungeonToastWidget::NativeTick(const FGeometry& Geometry, float DeltaSeconds)
{
	Super::NativeTick(Geometry, DeltaSeconds);
	if (!bPlaying) return;
	const auto* Theme = Style ? Style.Get() : GetDefault<UDungeonToastStyle>();
	const float Fade = FMath::Max(0.01f, Theme->FadeSeconds);
	const float Duration = FMath::Max(0.1f, Theme->HoldSeconds) + 2.f * Fade;
	Age += DeltaSeconds;
	if (Age >= Duration)
	{
		bPlaying = false;
		SetVisibility(ESlateVisibility::Collapsed);
		OnFinished.ExecuteIfBound();
		return;
	}
	const float Opacity = FMath::Min(FMath::Clamp(Age / Fade, 0.f, 1.f), FMath::Clamp((Duration - Age) / Fade, 0.f, 1.f));
	SetRenderOpacity(Opacity);
	Card->SetRenderTranslation(FVector2D(0.f, -Theme->SlideDistance * (1.f - Opacity)));
	LifetimeBar->SetPercent(FMath::Clamp((Duration - Age) / Duration, 0.f, 1.f));
}
