#include "GameUIManagerSubsystem.h"
#include "Engine/AssetManager.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StreamableManager.h"
#include "GameFramework/PlayerController.h"
#include "GameUILayer.h"
#include "GameUIRootWidget.h"
#include "GameUIScreenRegistry.h"
#include "GameUISettings.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_MODULE(FDefaultModuleImpl, GameUIFramework);
DEFINE_LOG_CATEGORY(LogGameUI);

UGameUIManagerSubsystem* UGameUIManagerSubsystem::Get(const UUserWidget* Widget)
{
	const ULocalPlayer* Player = Widget ? Widget->GetOwningLocalPlayer() : nullptr;
	return Player ? Player->GetSubsystem<UGameUIManagerSubsystem>() : nullptr;
}

UGameUIManagerSubsystem* UGameUIManagerSubsystem::Get(const APlayerController* Controller)
{
	const ULocalPlayer* Player = Controller ? Controller->GetLocalPlayer() : nullptr;
	return Player ? Player->GetSubsystem<UGameUIManagerSubsystem>() : nullptr;
}

void UGameUIManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	const UGameUIScreenRegistry* Screens = GetRegistry();
	if (!Screens) return;
	for (const FGameUIScreenEntry& Entry : Screens->Screens)
	{
		if (!Entry.bPreload || Entry.Class.IsNull()) continue;
		UE_LOG(LogGameUI, Log, TEXT("[GameUI] Preloading %s"), *Entry.Tag.ToString());
		Load(Entry, [] {});
	}
}

void UGameUIManagerSubsystem::Deinitialize()
{
	Pending.Reset();
	Held.Reset();
	Super::Deinitialize();
}

const UGameUIScreenRegistry* UGameUIManagerSubsystem::GetRegistry()
{
	// ponytail: the registry only names soft classes, so loading it in place is cheap.
	if (!Registry) Registry = GetDefault<UGameUISettings>()->Registry.LoadSynchronous();
	return Registry;
}

bool UGameUIManagerSubsystem::IsLoaded(const FGameUIScreenEntry& Entry)
{
	const UClass* Class = Entry.Class.Get();
	if (!Class) return false;
	TArray<FSoftObjectPath> Paths;
	Class->GetDefaultObject<UGameUIScreenWidget>()->GatherPreloadAssets(Paths);
	return !Paths.ContainsByPredicate([](const FSoftObjectPath& Path) { return Path.IsValid() && !Path.ResolveObject(); });
}

void UGameUIManagerSubsystem::Load(const FGameUIScreenEntry& Entry, TFunction<void()> Done)
{
	// The handles are held for good, so a screen loaded once stays loaded with its preload assets.
	const TSoftClassPtr<UGameUIScreenWidget> Class = Entry.Class;
	auto AfterClass = [this, Class, Done = MoveTemp(Done)]() mutable
	{
		TArray<FSoftObjectPath> Paths;
		if (const UClass* LoadedClass = Class.Get()) LoadedClass->GetDefaultObject<UGameUIScreenWidget>()->GatherPreloadAssets(Paths);
		Paths.RemoveAll([](const FSoftObjectPath& Path) { return !Path.IsValid(); });
		if (Paths.IsEmpty())
		{
			Done();
			return;
		}
		Held.Add(UAssetManager::GetStreamableManager().RequestAsyncLoad(Paths, FStreamableDelegate::CreateWeakLambda(this, MoveTemp(Done))));
	};
	const TSharedPtr<FStreamableHandle> ClassLoad = UAssetManager::GetStreamableManager().RequestAsyncLoad(Class.ToSoftObjectPath());
	Held.Add(ClassLoad);
	if (!ClassLoad || ClassLoad->HasLoadCompleted()) AfterClass();
	else ClassLoad->BindCompleteDelegate(FStreamableDelegate::CreateWeakLambda(this, MoveTemp(AfterClass)));
}

void UGameUIManagerSubsystem::Open(const FGameplayTag Screen, FGameUIOnReady OnReady)
{
	if (!Screen.IsValid()) return;
	if (UGameUIScreenWidget* Widget = Find(Screen))
	{
		if (UGameUIStackLayer* Layer = LayerOf(Widget)) Layer->PushScreen(Widget);
		ApplyInputState();
		if (OnReady) OnReady(*Widget);
		return;
	}
	if (FPendingOpen* Waiting = Pending.Find(Screen))
	{
		if (OnReady) Waiting->Waiting.Add(MoveTemp(OnReady));
		return;
	}
	const UGameUIScreenRegistry* Screens = GetRegistry();
	const FGameUIScreenEntry* Entry = Screens ? Screens->Find(Screen) : nullptr;
	if (!Entry || Entry->Class.IsNull())
	{
		UE_LOG(LogGameUI, Error, TEXT("[GameUI] No screen class for %s in the registry %s"), *Screen.ToString(), *GetNameSafe(Screens));
		return;
	}
	FPendingOpen& Open = Pending.Add(Screen);
	Open.Id = ++LastOpenId;
	if (OnReady) Open.Waiting.Add(MoveTemp(OnReady));
	if (IsLoaded(*Entry))
	{
		FinishOpen(Screen, Entry->Class.Get());
		return;
	}
	UE_LOG(LogGameUI, Log, TEXT("[GameUI] Loading %s"), *Screen.ToString());
	if (UGameUIRootWidget* Layout = GetRoot()) Layout->SetLoading(true);
	ApplyInputState();
	const TSoftClassPtr<UGameUIScreenWidget> Class = Entry->Class;
	// An opening that Close dropped, or that a later Open replaced, is not finished.
	Load(*Entry, [this, Screen, Class, Id = Open.Id]
	{
		if (const FPendingOpen* Still = Pending.Find(Screen); Still && Still->Id == Id) FinishOpen(Screen, Class.Get());
	});
}

void UGameUIManagerSubsystem::FinishOpen(const FGameplayTag Screen, UClass* Class)
{
	FPendingOpen Open;
	Pending.RemoveAndCopyValue(Screen, Open);
	UGameUIRootWidget* Layout = GetRoot();
	APlayerController* Controller = GetController();
	if (Layout) Layout->SetLoading(!Pending.IsEmpty());
	if (!Layout || !Controller || !Class)
	{
		UE_LOG(LogGameUI, Error, TEXT("[GameUI] %s could not open: root %s, class %s"), *Screen.ToString(), *GetNameSafe(Layout), *GetNameSafe(Class));
		ApplyInputState();
		return;
	}
	UGameUIScreenWidget* Widget = nullptr;
	const int32 KeptIndex = Kept.IndexOfByPredicate([Screen](const UGameUIScreenWidget* W) { return W && W->ScreenTag == Screen; });
	if (KeptIndex != INDEX_NONE)
	{
		Widget = Kept[KeptIndex];
		Kept.RemoveAt(KeptIndex);
		if (!IsValid(Widget) || Widget->GetOwningPlayer() != Controller || Widget->GetClass() != Class) Widget = nullptr;
	}
	if (!Widget)
	{
		Widget = CreateWidget<UGameUIScreenWidget>(Controller, Class);
		if (!Widget)
		{
			UE_LOG(LogGameUI, Error, TEXT("[GameUI] %s could not make a widget of %s"), *Screen.ToString(), *GetNameSafe(Class));
			ApplyInputState();
			return;
		}
		Widget->ScreenTag = Screen;
	}
	UGameUIStackLayer* Layer = LayerOf(Widget);
	if (!Layer)
	{
		UE_LOG(LogGameUI, Error, TEXT("[GameUI] %s names layer %s, which the root %s has no stack layer for"), *Screen.ToString(),
		       *Widget->Layer.ToString(), *GetNameSafe(Layout->GetClass()));
		ApplyInputState();
		return;
	}
	Layer->PushScreen(Widget);
	ApplyInputState();
	for (FGameUIOnReady& Ready : Open.Waiting)
	{
		// A waiting caller may close the screen again.
		if (Find(Screen) != Widget) break;
		Ready(*Widget);
	}
}

void UGameUIManagerSubsystem::Close(const FGameplayTag Screen)
{
	if (Pending.Remove(Screen))
	{
		if (UGameUIRootWidget* Layout = GetLiveRoot()) Layout->SetLoading(!Pending.IsEmpty());
		ApplyInputState();
		return;
	}
	CloseScreen(Find(Screen));
}

void UGameUIManagerSubsystem::CloseScreen(UGameUIScreenWidget* Screen)
{
	UGameUIStackLayer* Layer = Screen ? LayerOf(Screen) : nullptr;
	if (!Layer || !Layer->GetScreens().Contains(Screen)) return;
	Layer->RemoveScreen(Screen);
	if (Screen->bKeepWhenClosed)
	{
		Kept.RemoveAll([Screen](const UGameUIScreenWidget* W) { return !W || W->ScreenTag == Screen->ScreenTag; });
		Kept.Add(Screen);
	}
	ApplyInputState();
}

void UGameUIManagerSubsystem::Toggle(const FGameplayTag Screen)
{
	if (IsOpen(Screen) || IsPending(Screen)) Close(Screen);
	else Open(Screen);
}

bool UGameUIManagerSubsystem::CloseTop()
{
	UGameUIScreenWidget* Top = GetTopScreen();
	if (!Top) return false;
	CloseScreen(Top);
	return true;
}

bool UGameUIManagerSubsystem::HandleBack()
{
	UGameUIScreenWidget* Top = GetTopScreen();
	if (!Top) return false;
	switch (Top->Back)
	{
	case EGameUIBack::Close:
		CloseScreen(Top);
		return true;
	case EGameUIBack::Custom:
		return Top->NativeOnBack();
	default:
		return true;
	}
}

UGameUIScreenWidget* UGameUIManagerSubsystem::Find(const FGameplayTag Screen) const
{
	const UGameUIRootWidget* Layout = GetLiveRoot();
	if (!Layout) return nullptr;
	for (const UGameUILayer* Layer : Layout->GetLayers())
	{
		const UGameUIStackLayer* Stack = Cast<UGameUIStackLayer>(Layer);
		if (!Stack) continue;
		for (UGameUIScreenWidget* Widget : Stack->GetScreens())
		{
			if (Widget && Widget->ScreenTag == Screen) return Widget;
		}
	}
	return nullptr;
}

UGameUIScreenWidget* UGameUIManagerSubsystem::GetTopScreen() const
{
	const UGameUIRootWidget* Layout = GetLiveRoot();
	if (!Layout) return nullptr;
	const TArray<TObjectPtr<UGameUILayer>>& Layers = Layout->GetLayers();
	for (int32 i = Layers.Num() - 1; i >= 0; --i)
	{
		const UGameUIStackLayer* Stack = Cast<UGameUIStackLayer>(Layers[i]);
		if (UGameUIScreenWidget* Top = Stack ? Stack->GetTop() : nullptr) return Top;
	}
	return nullptr;
}

bool UGameUIManagerSubsystem::IsGameplayBlocked() const
{
	const UGameUIScreenWidget* Top = GetTopScreen();
	return Top ? Top->bBlockGameInput : !Pending.IsEmpty();
}

UGameUIRootWidget* UGameUIManagerSubsystem::GetLiveRoot() const
{
	return IsValid(Root) && Root->IsInViewport() ? Root.Get() : nullptr;
}

UGameUIRootWidget* UGameUIManagerSubsystem::GetRoot()
{
	APlayerController* Controller = GetController();
	if (UGameUIRootWidget* Live = GetLiveRoot(); Live && Live->GetOwningPlayer() == Controller) return Live;
	if (!Controller) return nullptr;
	// The last root left with its world, and its screens with it; the kept ones belonged to that world's controller.
	Kept.Reset();
	const UGameUISettings* Settings = GetDefault<UGameUISettings>();
	// ponytail: the root is a small layout needed at once, so its class loads in place.
	UClass* Class = Settings->RootClass.LoadSynchronous();
	Root = Class ? CreateWidget<UGameUIRootWidget>(Controller, Class) : nullptr;
	if (!Root)
	{
		UE_LOG(LogGameUI, Error, TEXT("[GameUI] No root widget: %s"), *Settings->RootClass.ToString());
		return nullptr;
	}
	// ponytail: on the viewport, among the widgets the game still adds there itself (the HUD); a split screen needs
	// AddToPlayerScreen, once those widgets are in the root's layers.
	Root->AddToViewport(Settings->RootZOrder);
	// Nothing of the last world is open any more.
	LastTop = FGameplayTag();
	if (bLastBlocked)
	{
		bLastBlocked = false;
		OnInputStateChanged.Broadcast(false);
	}
	return Root;
}

UGameUIStackLayer* UGameUIManagerSubsystem::LayerOf(const UGameUIScreenWidget* Screen)
{
	UGameUIRootWidget* Layout = GetRoot();
	if (!Layout || !Screen) return nullptr;
	const FGameplayTag Tag = Screen->Layer.IsValid() ? Screen->Layer : Screen->GetDefaultLayer();
	return Cast<UGameUIStackLayer>(Layout->FindLayer(Tag));
}

APlayerController* UGameUIManagerSubsystem::GetController() const
{
	const ULocalPlayer* Player = GetLocalPlayer();
	return Player ? Player->GetPlayerController(Player->GetWorld()) : nullptr;
}

void UGameUIManagerSubsystem::ApplyInputState()
{
	const UGameUIScreenWidget* Top = GetTopScreen();
	const bool bBlocked = IsGameplayBlocked();
	if (APlayerController* Controller = GetController())
	{
		const EGameUIInputMode Mode = Top ? Top->InputMode : EGameUIInputMode::GameOnly;
		Controller->SetShowMouseCursor(Top && Top->bShowMouse);
		switch (Mode)
		{
		case EGameUIInputMode::UIOnly:
			Controller->SetInputMode(FInputModeUIOnly());
			break;
		case EGameUIInputMode::GameAndUI:
			{
				FInputModeGameAndUI GameAndUI;
				GameAndUI.SetHideCursorDuringCapture(false);
				Controller->SetInputMode(GameAndUI);
				break;
			}
		default:
			Controller->SetInputMode(FInputModeGameOnly());
			break;
		}
	}
	const FGameplayTag TopTag = Top ? Top->ScreenTag : FGameplayTag();
	if (bBlocked != bLastBlocked)
	{
		bLastBlocked = bBlocked;
		OnInputStateChanged.Broadcast(bBlocked);
	}
	if (TopTag != LastTop)
	{
		LastTop = TopTag;
		OnTopScreenChanged.Broadcast(TopTag);
	}
}
