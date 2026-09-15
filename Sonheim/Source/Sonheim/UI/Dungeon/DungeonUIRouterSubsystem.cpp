#include "DungeonUIRouterSubsystem.h"
#include "DungeonUIRegistryDataAsset.h"
#include "DungeonStagePresenter.h"
#include "DungeonViewWidget.h"
#include "Engine/AssetManager.h"
#include "GameFramework/PlayerController.h"
#include "Sonheim/Utilities/LogMacro.h"
void UDungeonUIRouterSubsystem::Attach(APlayerController* Controller)
{
	if (Owner.Get() == Controller && Presenter) return;
	Detach(Owner.Get());
	Owner = Controller;
	if (!Controller || !Controller->IsLocalController()) return;
	Presenter = NewObject<UDungeonStagePresenter>(this);
	Presenter->Start(Controller, this);
	const int32 Expected = Generation;
	RegistryLoad = UAssetManager::GetStreamableManager().RequestAsyncLoad(RegistryAsset.ToSoftObjectPath(), FStreamableDelegate::CreateWeakLambda(this, [this, Expected]()
	{
		if (Expected != Generation) return;
		Registry = RegistryAsset.Get();
		RefreshWidget();
	}));
}
void UDungeonUIRouterSubsystem::Detach(APlayerController* Controller)
{
	if (Owner.Get() != Controller) return;
	++Generation;
	if (Presenter) Presenter->Stop();
	if (ActiveWidget) ActiveWidget->RemoveFromParent();
	Presenter = nullptr; ActiveWidget = nullptr; Registry = nullptr;
	RegistryLoad.Reset(); WidgetLoad.Reset(); ActiveId = NAME_None; RequestedId = NAME_None;
	Owner.Reset(); LatestView = FDungeonStageViewData{};
}
void UDungeonUIRouterSubsystem::Deinitialize() { Detach(Owner.Get()); Super::Deinitialize(); }
void UDungeonUIRouterSubsystem::ApplyView(const FDungeonStageViewData& Data)
{
	LatestView = Data;
	RefreshWidget();
}
void UDungeonUIRouterSubsystem::RefreshWidget()
{
	if (!Registry || !Owner.IsValid()) return;
	if (LatestView.Status == EDungeonRunStatus::Idle)
	{
		if (ActiveWidget) ActiveWidget->RemoveFromParent();
		ActiveWidget = nullptr; ActiveId = NAME_None; RequestedId = NAME_None; return;
	}
	const FName Id = (LatestView.Status == EDungeonRunStatus::Succeeded || LatestView.Status == EDungeonRunStatus::Failed) ? FName(TEXT("Dungeon.Result")) : FName(TEXT("Dungeon.HUD"));
	if (ActiveId == Id && ActiveWidget && ActiveWidget->IsInViewport())
	{
		ActiveWidget->ApplyViewData(LatestView);
		UE_LOG(SONHEIM, Log, TEXT("[DungeonHUD] Controller=%s Revision=%d Count=%s Branch=%s"), *Owner->GetName(), LatestView.Revision, *LatestView.CountText.ToString(), *LatestView.BranchText.ToString());
		return;
	}
	const auto* Entry = Registry->Entries.FindByPredicate([Id](const auto& Value) { return Value.UIId.GetTagName() == Id; });
	if (!Entry || Entry->WidgetClass.IsNull()) return;
	// P0 HUD/result are non-modal. Refuse policies that would compete with existing inventory input.
	if (Entry->InputMode != EDungeonUIInputMode::GameOnly || Entry->bShowMouse)
	{
		UE_LOG(SONHEIM, Error, TEXT("[DungeonUI] P0 HUD/result require GameOnly and unchanged mouse policy.")); return;
	}
	if (!Entry->WidgetClass.Get())
	{
		if (RequestedId == Id) return;
		RequestedId = Id;
		const int32 Expected = Generation;
		const auto RequestedClass = Entry->WidgetClass;
		WidgetLoad = UAssetManager::GetStreamableManager().RequestAsyncLoad(Entry->WidgetClass.ToSoftObjectPath(), FStreamableDelegate::CreateWeakLambda(this, [this, Expected, RequestedClass]()
		{
			if (Expected != Generation) return;
			if (!RequestedClass.Get())
			{
				// Keep RequestedId latched until another screen or a new attachment.
				UE_LOG(SONHEIM, Error, TEXT("[DungeonUI] Widget load failed: %s"), *RequestedClass.ToSoftObjectPath().ToString());
				return;
			}
			RequestedId = NAME_None; RefreshWidget();
		}));
		return;
	}
	if (ActiveWidget) ActiveWidget->RemoveFromParent();
	ActiveWidget = CreateWidget<UDungeonViewWidget>(Owner.Get(), Entry->WidgetClass.Get());
	if (ActiveWidget)
	{
		ActiveId = Id;
		ActiveWidget->AddToPlayerScreen(Entry->Layer == EDungeonUILayer::HUD ? 1800 : 1900);
		ActiveWidget->ApplyViewData(LatestView);
		UE_LOG(SONHEIM, Log, TEXT("[DungeonHUD] Created=%s Controller=%s Revision=%d Count=%s"), *Id.ToString(), *Owner->GetName(), LatestView.Revision, *LatestView.CountText.ToString());
	}
}
