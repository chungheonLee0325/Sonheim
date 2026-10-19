#include "GameUILayer.h"
#include "Components/OverlaySlot.h"
#include "GameUIScreenWidget.h"

UGameUILayer::UGameUILayer()
{
	// An empty layer lets clicks through to the layers under it.
	SetVisibilityInternal(ESlateVisibility::SelfHitTestInvisible);
}

void UGameUILayer::AddLayerWidget(UWidget* Widget)
{
	if (UOverlaySlot* Added = AddChildToOverlay(Widget))
	{
		Added->SetHorizontalAlignment(HAlign_Fill);
		Added->SetVerticalAlignment(VAlign_Fill);
	}
}

void UGameUIStackLayer::PushScreen(UGameUIScreenWidget* Screen)
{
	if (!Screen) return;
	UGameUIScreenWidget* Top = GetTop();
	if (Top == Screen) return;
	if (Screens.Contains(Screen))
	{
		Screens.Remove(Screen);
		Cover(Top);
		Screens.Add(Screen);
		Uncover(Screen);
		return;
	}
	Cover(Top);
	Screens.Add(Screen);
	AddLayerWidget(Screen);
	Screen->NativeOnOpened();
}

void UGameUIStackLayer::RemoveScreen(UGameUIScreenWidget* Screen)
{
	const int32 Index = Screens.IndexOfByKey(Screen);
	if (Index == INDEX_NONE) return;
	const bool bWasTop = Index == Screens.Num() - 1;
	// Out of the stack first: a closing screen may close others through the manager.
	Screens.RemoveAt(Index);
	if (!bWasTop) Screen->SetVisibility(Screen->UncoveredVisibility);
	Screen->NativeOnClosed();
	Screen->RemoveFromParent();
	if (bWasTop) Uncover(GetTop());
}

void UGameUIStackLayer::Cover(UGameUIScreenWidget* Screen)
{
	if (!Screen) return;
	Screen->UncoveredVisibility = Screen->GetVisibility();
	Screen->SetVisibility(ESlateVisibility::Collapsed);
	Screen->NativeOnCovered();
}

void UGameUIStackLayer::Uncover(UGameUIScreenWidget* Screen)
{
	if (!Screen) return;
	Screen->SetVisibility(Screen->UncoveredVisibility);
	Screen->NativeOnUncovered();
}
