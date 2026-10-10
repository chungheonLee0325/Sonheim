#include "MenuTabWidget.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/DynamicEntryBox.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerController.h"

void UMenuTabEntryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	TabButton->OnClicked.AddDynamic(this, &UMenuTabEntryWidget::Clicked);
}

void UMenuTabEntryWidget::Show(const FMenuTab& InTab, const bool bActive)
{
	Tab = InTab;
	const bool bEnabled = !Tab.Screen.IsNone();
	const FLinearColor Color = !bEnabled ? Dimmed : bActive ? TextOnActive : Text;
	LabelText->SetText(Tab.Label);
	LabelText->SetColorAndOpacity(Color);
	UTexture2D* Icon = Tab.Icon.LoadSynchronous();
	if (Icon) IconImage->SetBrushFromTexture(Icon);
	IconImage->SetColorAndOpacity(Color);
	ActivePlate->SetBrushColor(bActive ? Accent : FLinearColor::Transparent);
	if (ActiveMark) ActiveMark->SetVisibility(bActive ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
}

void UMenuTabEntryWidget::Clicked()
{
	// A disabled button would grey its label, so a dimmed or active tab keeps its button and the click does nothing.
	auto* Controller = GetOwningPlayer<ASonheimPlayerController>();
	if (Controller && !Tab.Screen.IsNone()) Controller->OpenTab(Tab.Screen);
}

void UMenuTabBarWidget::ShowActive(const FName Screen)
{
	TabBox->Reset(true);
	for (const FMenuTab& Tab : Tabs) TabBox->CreateEntry<UMenuTabEntryWidget>()->Show(Tab, Tab.Screen == Screen);
}
