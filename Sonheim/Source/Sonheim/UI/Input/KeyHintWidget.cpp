#include "KeyHintWidget.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "InputHintSubsystem.h"

void UKeyHintWidget::SetAction(UInputAction* InAction)
{
	Action = InAction;
	Refresh();
}

void UKeyHintWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	Refresh();
}

void UKeyHintWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (UInputHintSubsystem* Hints = GetOwningLocalPlayer() ? GetOwningLocalPlayer()->GetSubsystem<UInputHintSubsystem>() : nullptr)
	{
		Changed = Hints->OnChanged.AddUObject(this, &UKeyHintWidget::Refresh);
	}
	Refresh();
}

void UKeyHintWidget::NativeDestruct()
{
	if (UInputHintSubsystem* Hints = GetOwningLocalPlayer() ? GetOwningLocalPlayer()->GetSubsystem<UInputHintSubsystem>() : nullptr)
	{
		Hints->OnChanged.Remove(Changed);
	}
	Super::NativeDestruct();
}

void UKeyHintWidget::Refresh()
{
	const UInputHintSubsystem* Hints = GetOwningLocalPlayer() ? GetOwningLocalPlayer()->GetSubsystem<UInputHintSubsystem>() : nullptr;
	FInputGlyph Glyph;
	// The designer has no player, so it keeps the cap the Widget Blueprint was authored with.
	if (!Hints) return;
	if (!Hints->GlyphFor(Action, Glyph))
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	SetVisibility(ESlateVisibility::HitTestInvisible);
	KeyText->SetText(Glyph.Label);
	KeyText->SetColorAndOpacity(Glyph.Ink);
	FSlateBrush Brush = Cap->Background;
	Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
	Brush.TintColor = FSlateColor(Glyph.Plate);
	const bool bRound = Glyph.Shape == EInputGlyphShape::Round || Glyph.Shape == EInputGlyphShape::Bumper;
	Brush.OutlineSettings.RoundingType = bRound ? ESlateBrushRoundingType::HalfHeightRadius : ESlateBrushRoundingType::FixedRadius;
	Brush.OutlineSettings.CornerRadii = FVector4(4.f, 4.f, 4.f, 4.f);
	Cap->SetBrush(Brush);
	Cap->SetBrushColor(FLinearColor::White);
	CapSize->SetHeightOverride(Size);
	// A key or face button is square; a wide key or bumper takes its label's width with some room.
	if (Glyph.Shape == EInputGlyphShape::Key || Glyph.Shape == EInputGlyphShape::Round)
	{
		CapSize->SetWidthOverride(Size);
		CapSize->ClearMinDesiredWidth();
	}
	else
	{
		CapSize->ClearWidthOverride();
		CapSize->SetMinDesiredWidth(Size * 1.6f);
	}
}
