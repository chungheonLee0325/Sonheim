#include "GameUIRootWidget.h"
#include "GameUILayer.h"

void UGameUIRootWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	// The tree is walked in the order the layout draws it, so the layers come back to front.
	WidgetTree->ForEachWidget([this](UWidget* Widget)
	{
		if (UGameUILayer* Layer = Cast<UGameUILayer>(Widget)) Layers.Add(Layer);
	});
	SetLoading(false);
}

UGameUILayer* UGameUIRootWidget::FindLayer(const FGameplayTag Tag) const
{
	const TObjectPtr<UGameUILayer>* Found = Layers.FindByPredicate([Tag](const UGameUILayer* Layer) { return Layer->LayerTag == Tag; });
	return Found ? Found->Get() : nullptr;
}

void UGameUIRootWidget::SetLoading(const bool bLoading)
{
	if (LoadingIndicator) LoadingIndicator->SetVisibility(bLoading ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}
