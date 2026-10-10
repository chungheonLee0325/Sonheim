#include "NoticeWidget.h"
#include "Components/Border.h"
#include "Components/DynamicEntryBox.h"
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

	FLinearColor WithAlpha(FLinearColor Color, const float Alpha)
	{
		Color.A = Alpha;
		return Color;
	}
}

FLinearColor UNoticeStyle::AccentOf(const ENoticeTone Tone) const
{
	switch (Tone)
	{
	case ENoticeTone::Highlight: return Highlight;
	case ENoticeTone::Warning: return Warning;
	case ENoticeTone::Success: return Success;
	case ENoticeTone::Update: return Update;
	case ENoticeTone::Legend: return Legend;
	default: return Information;
	}
}

void UNoticeLineWidget::Show(const FNoticeLine& Line, const FLinearColor& Accent)
{
	UTexture2D* Icon = Line.Icon.LoadSynchronous();
	if (Icon) IconImage->SetBrushFromTexture(Icon);
	IconImage->SetVisibility(Icon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	LabelText->SetText(Line.Label);
	ValueText->SetText(Line.Value);
	ValueText->SetColorAndOpacity(Accent);
}

void UNoticeWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	ApplyStyle();
}

void UNoticeWidget::ApplyStyle()
{
	const UNoticeStyle& Look = Theme();
	const FLinearColor Accent = Look.AccentOf(ViewData.Tone);
	if (UBorder* Surface = Cast<UBorder>(Card)) Surface->SetBrushColor(Look.Surface);
	if (AccentLine) AccentLine->SetBrushColor(Accent);
	if (IconPlate) IconPlate->SetBrushColor(FLinearColor(Accent.R * 0.13f, Accent.G * 0.13f, Accent.B * 0.13f, 1.f));
	if (EmblemPlate) EmblemPlate->SetBrushColor(Accent);
	// The band's material fades it toward its ends; over the layout's dark base it only gives the tone, softer than its edge lines.
	if (BandImage) BandImage->SetColorAndOpacity(WithAlpha(Accent, 0.5f));
	if (BandEdgeTop) BandEdgeTop->SetColorAndOpacity(Accent);
	if (BandEdgeBottom) BandEdgeBottom->SetColorAndOpacity(Accent);
	if (SymbolText) SymbolText->SetColorAndOpacity(EmblemPlate ? Look.Text : Accent);
	if (SymbolIcon) SymbolIcon->SetColorAndOpacity(EmblemPlate ? Look.Text : Accent);
	if (CategoryText) CategoryText->SetColorAndOpacity(Accent);
	if (TitleText) TitleText->SetColorAndOpacity(Look.Text);
	if (DetailText) DetailText->SetColorAndOpacity(Look.Muted);
	if (FooterText) FooterText->SetColorAndOpacity(Look.Text);
	if (LifetimeBar) LifetimeBar->SetFillColorAndOpacity(Accent);
}

void UNoticeWidget::ShowFrom(const FName Channel, const FNoticeData& Data)
{
	ViewData = Data;
	ShowingChannel = Channel;
	Age = 0.f; bPlaying = true;
	SetVisibility(ESlateVisibility::HitTestInvisible);
	SetNoticeLine(CategoryText, Data.Category);
	SetNoticeLine(TitleText, Data.Title);
	SetNoticeLine(DetailText, Data.Detail);
	SetNoticeLine(FooterText, Data.Footer);
	if (FooterRow) FooterRow->SetVisibility(Data.Footer.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	// The icon, when the notice has one, stands in for the symbol.
	UTexture2D* Icon = SymbolIcon ? Data.Icon.LoadSynchronous() : nullptr;
	if (Icon) SymbolIcon->SetBrushFromTexture(Icon);
	if (SymbolIcon) SymbolIcon->SetVisibility(Icon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (SymbolText)
	{
		SymbolText->SetText(Data.Symbol);
		SymbolText->SetVisibility(Icon ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	UTexture2D* Portrait = PortraitImage ? Data.Portrait.LoadSynchronous() : nullptr;
	if (Portrait) PortraitImage->SetBrushFromTexture(Portrait);
	if (UWidget* Frame = PortraitPanel ? PortraitPanel.Get() : PortraitImage.Get())
		Frame->SetVisibility(Portrait ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	ApplyStyle();
	if (LineBox)
	{
		LineBox->Reset(true);
		const FLinearColor Accent = Theme().AccentOf(Data.Tone);
		for (const FNoticeLine& Line : Data.Lines)
		{
			if (UNoticeLineWidget* Entry = LineBox->CreateEntry<UNoticeLineWidget>()) Entry->Show(Line, Accent);
		}
		LineBox->SetVisibility(Data.Lines.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
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
	const UNoticeStyle& Look = Theme();
	const float Fade = FMath::Max(0.01f, Look.FadeSeconds);
	const float Duration = FMath::Max(0.1f, Look.HoldSeconds) + 2.f * Fade;
	Age += DeltaSeconds;
	if (Age >= Duration)
	{
		Stop();
		OnFinished.ExecuteIfBound();
		return;
	}
	const float In = FMath::Clamp(Age / Fade, 0.f, 1.f);
	const float Opacity = FMath::Min(In, FMath::Clamp((Duration - Age) / Fade, 0.f, 1.f));
	SetRenderOpacity(Opacity);
	// It eases in from SlideFrom and ScaleFrom, and only fades on the way out.
	const float Rest = 1.f - FMath::Square(1.f - In);
	UWidget* Moving = Card ? Card.Get() : this;
	Moving->SetRenderTranslation(Look.SlideFrom * (1.f - Rest));
	Moving->SetRenderScale(FVector2D(FMath::Lerp(Look.ScaleFrom, 1.f, Rest)));
	if (LifetimeBar) LifetimeBar->SetPercent(FMath::Clamp((Duration - Age) / Duration, 0.f, 1.f));
	if (Sweep)
	{
		const float T = FMath::Clamp(Age / FMath::Max(0.05f, SweepSeconds), 0.f, 1.f);
		const float Width = Moving->GetCachedGeometry().GetLocalSize().X;
		Sweep->SetRenderTranslation(FVector2D(FMath::Lerp(-0.5f, 0.5f, T) * Width, 0.f));
		Sweep->SetRenderOpacity(FMath::Sin(T * PI));
	}
}

void UNoticeFeedWidget::ShowFrom(const FName Channel, const FNoticeData& Data)
{
	SetVisibility(ESlateVisibility::HitTestInvisible);
	SetRenderOpacity(1.f);
	TArray<UUserWidget*> Rows = RowBox->GetAllEntries();
	for (int32 Index = 0; Index <= Rows.Num() - FMath::Max(1, MaxShown); ++Index) RowBox->RemoveEntry(Rows[Index]);
	UNoticeWidget* Row = RowBox->CreateEntry<UNoticeWidget>();
	if (!Row) return;
	Row->Style = Style;
	Row->ShowFrom(Channel, Data);
}

void UNoticeFeedWidget::StopChannel(const FName Channel)
{
	for (UUserWidget* Entry : RowBox->GetAllEntries())
	{
		if (const UNoticeWidget* Row = Cast<UNoticeWidget>(Entry); Row && Row->ShowingChannel == Channel) RowBox->RemoveEntry(Entry);
	}
}

void UNoticeFeedWidget::NativeTick(const FGeometry& Geometry, float DeltaSeconds)
{
	UUserWidget::NativeTick(Geometry, DeltaSeconds);
	// A row that has played out leaves the column, and the rows under it close the gap.
	for (UUserWidget* Entry : RowBox->GetAllEntries())
	{
		if (!Entry->IsVisible()) RowBox->RemoveEntry(Entry);
	}
}
