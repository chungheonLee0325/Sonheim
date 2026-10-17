#include "MenuTabWidget.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/DynamicEntryBox.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "GameUIManagerSubsystem.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerController.h"

void UMenuTabEntryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	TabButton->OnClicked.AddDynamic(this, &UMenuTabEntryWidget::Clicked);
}

void UMenuTabEntryWidget::Show(const FMenuTab& InTab, const bool bActive)
{
	Tab = InTab;
	const bool bEnabled = Tab.Screen.IsValid();
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
	if (Controller && Tab.Screen.IsValid()) Controller->OpenTab(Tab.Screen);
}

void UMenuTabBarWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SetVisibility(ESlateVisibility::Collapsed);
	if (UGameUIManagerSubsystem* Manager = UGameUIManagerSubsystem::Get(this))
	{
		Manager->OnTopScreenChanged.AddWeakLambda(this, [this](const FGameplayTag Top)
		{
			const bool bTab = HasTab(Top);
			SetVisibility(bTab ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
			if (bTab) ShowActive(Top);
		});
	}
}

bool UMenuTabBarWidget::HasTab(const FGameplayTag Screen) const
{
	return Screen.IsValid() && Tabs.ContainsByPredicate([Screen](const FMenuTab& Tab) { return Tab.Screen == Screen; });
}

FGameplayTag UMenuTabBarWidget::Step(const FGameplayTag From, const int32 Dir) const
{
	const int32 Count = Tabs.Num();
	const int32 Index = Tabs.IndexOfByPredicate([From](const FMenuTab& Tab) { return Tab.Screen == From; });
	if (!From.IsValid() || Index == INDEX_NONE || Dir == 0) return FGameplayTag();
	for (int32 Offset = 1; Offset < Count; ++Offset)
	{
		const FMenuTab& Tab = Tabs[((Index + Offset * FMath::Sign(Dir)) % Count + Count) % Count];
		if (Tab.Screen.IsValid()) return Tab.Screen;
	}
	return FGameplayTag();
}

void UMenuTabBarWidget::ShowActive(const FGameplayTag Screen)
{
	// The entries go back to the box's pool and come out again for the next tab, instead of being made anew.
	TabBox->Reset();
	for (const FMenuTab& Tab : Tabs) TabBox->CreateEntry<UMenuTabEntryWidget>()->Show(Tab, Tab.Screen == Screen);
}
