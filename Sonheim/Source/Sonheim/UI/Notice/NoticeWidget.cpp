#include "NoticeWidget.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

namespace
{
	/** A line with nothing to say folds away, so a title without a subtitle stands alone. */
	void SetNoticeLine(UTextBlock* Block, const FText& Text)
	{
		if (!Block) return;
		Block->SetText(Text);
		Block->SetVisibility(Text.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
}

void UNoticeWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	ApplyStyle();
}

void UNoticeWidget::ApplyStyle()
{
	const auto* Theme = Style ? Style.Get() : GetDefault<UNoticeStyle>();
	const FLinearColor Accent = ViewData.Tone == ENoticeTone::Highlight ? Theme->Highlight :
		ViewData.Tone == ENoticeTone::Warning ? Theme->Warning : Theme->Information;
	if (Card) Card->SetBrushColor(Theme->Surface);
	if (AccentLine) AccentLine->SetBrushColor(Accent);
	if (IconPlate) IconPlate->SetBrushColor(FLinearColor(Accent.R * 0.13f, Accent.G * 0.13f, Accent.B * 0.13f, 1.f));
	if (SymbolText) SymbolText->SetColorAndOpacity(Accent);
	if (SymbolIcon) SymbolIcon->SetColorAndOpacity(Accent);
	if (CategoryText) CategoryText->SetColorAndOpacity(Accent);
	if (TitleText) TitleText->SetColorAndOpacity(Theme->Text);
	if (DetailText) DetailText->SetColorAndOpacity(Theme->Muted);
	if (LifetimeBar) LifetimeBar->SetFillColorAndOpacity(Accent);
}

void UNoticeWidget::Show(const FNoticeData& Data)
{
	ViewData = Data;
	Age = 0.f; bPlaying = true;
	SetVisibility(ESlateVisibility::HitTestInvisible);
	SetNoticeLine(CategoryText, Data.Category);
	SetNoticeLine(TitleText, Data.Title);
	SetNoticeLine(DetailText, Data.Detail);
	// The icon, when the notice has one, stands in for the symbol.
	UTexture2D* Icon = SymbolIcon ? Data.Icon.LoadSynchronous() : nullptr;
	if (Icon) SymbolIcon->SetBrushFromTexture(Icon);
	if (SymbolIcon) SymbolIcon->SetVisibility(Icon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (SymbolText)
	{
		SymbolText->SetText(Data.Symbol);
		SymbolText->SetVisibility(Icon ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	ApplyStyle();
	SetRenderOpacity(0.f);
	if (LifetimeBar) LifetimeBar->SetPercent(1.f);
}

void UNoticeWidget::Stop()
{
	bPlaying = false;
	SetVisibility(ESlateVisibility::Collapsed);
}

void UNoticeWidget::NativeTick(const FGeometry& Geometry, float DeltaSeconds)
{
	Super::NativeTick(Geometry, DeltaSeconds);
	if (!bPlaying) return;
	const auto* Theme = Style ? Style.Get() : GetDefault<UNoticeStyle>();
	const float Fade = FMath::Max(0.01f, Theme->FadeSeconds);
	const float Duration = FMath::Max(0.1f, Theme->HoldSeconds) + 2.f * Fade;
	Age += DeltaSeconds;
	if (Age >= Duration)
	{
		Stop();
		OnFinished.ExecuteIfBound();
		return;
	}
	const float Opacity = FMath::Min(FMath::Clamp(Age / Fade, 0.f, 1.f), FMath::Clamp((Duration - Age) / Fade, 0.f, 1.f));
	SetRenderOpacity(Opacity);
	if (Card) Card->SetRenderTranslation(FVector2D(0.f, -Theme->SlideDistance * (1.f - Opacity)));
	if (LifetimeBar) LifetimeBar->SetPercent(FMath::Clamp((Duration - Age) / Duration, 0.f, 1.f));
}
