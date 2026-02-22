#include "UIStackSubsystem.h"

#include "Blueprint/UserWidget.h"
#include "Engine/DataTable.h"
#include "GameFramework/PlayerController.h"

void UUIStackSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// DataTable is optional in v1; if missing, the subsystem still works for code-driven widgets.
	WidgetDefTable = LoadObject<UDataTable>(
		nullptr, TEXT("/Script/Engine.DataTable'/Game/_BluePrint/_DataTable/dt_UIWidgetDef.dt_UIWidgetDef'"));
	PresetTable = LoadObject<UDataTable>(
		nullptr, TEXT("/Script/Engine.DataTable'/Game/_BluePrint/_DataTable/dt_UIPreset.dt_UIPreset'"));

	PreLoadMapHandle = FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &UUIStackSubsystem::HandlePreLoadMap);
}

void UUIStackSubsystem::Deinitialize()
{
	if (PreLoadMapHandle.IsValid())
	{
		FCoreUObjectDelegates::PreLoadMap.Remove(PreLoadMapHandle);
		PreLoadMapHandle.Reset();
	}

	if (UWorld* World = GetWorld())
	{
		for (FUIStackEntry& E : ToastStack)
		{
			if (E.Widget) E.Widget->RemoveFromParent();
			if (E.AutoCloseHandle.IsValid())
			{
				World->GetTimerManager().ClearTimer(E.AutoCloseHandle);
			}
		}
	}

	ScreenStack.Empty();
	ModalStack.Empty();
	ToastStack.Empty();
	WidgetDefTable = nullptr;
	PresetTable = nullptr;

	Super::Deinitialize();
}

const FUIWidgetDefRow* UUIStackSubsystem::FindWidgetDef(FName UIId) const
{
	if (!WidgetDefTable) return nullptr;
	return WidgetDefTable->FindRow<FUIWidgetDefRow>(UIId, TEXT("UIStackSubsystem"));
}

UUserWidget* UUIStackSubsystem::CreateWidgetFromDef(const FUIWidgetDefRow& Def)
{
	UWorld* World = GetWorld();
	if (!World) return nullptr;
	APlayerController* PC = GetLocalPlayer() ? GetLocalPlayer()->GetPlayerController(World) : nullptr;
	if (!PC) return nullptr;

	UClass* WidgetClass = Def.WidgetClass.LoadSynchronous();
	if (!WidgetClass) return nullptr;

	return CreateWidget<UUserWidget>(PC, WidgetClass);
}

UUserWidget* UUIStackSubsystem::CreateWidgetFromClass(TSubclassOf<UUserWidget> WidgetClass) const
{
	UWorld* World = GetWorld();
	if (!World) return nullptr;
	APlayerController* PC = GetLocalPlayer() ? GetLocalPlayer()->GetPlayerController(World) : nullptr;
	if (!PC) return nullptr;
	if (!WidgetClass) return nullptr;
	return CreateWidget<UUserWidget>(PC, WidgetClass);
}

bool UUIStackSubsystem::IsStaleEntry(const FUIStackEntry& Entry) const
{
	return !Entry.Widget || !IsValid(Entry.Widget) || !Entry.Widget->IsInViewport();
}

bool UUIStackSubsystem::RemoveEntryByWidget(TArray<FUIStackEntry>& Stack, UUserWidget* Widget)
{
	if (!Widget) return false;

	for (int32 i = Stack.Num() - 1; i >= 0; --i)
	{
		if (Stack[i].Widget == Widget)
		{
			if (Stack[i].Widget)
			{
				Stack[i].Widget->RemoveFromParent();
			}
			Stack.RemoveAt(i);
			return true;
		}
	}

	return false;
}

UUserWidget* UUIStackSubsystem::HandleExistingPolicy(TArray<FUIStackEntry>& Stack, FName UIId, EUIStackPolicy Policy)
{
	if (Policy == EUIStackPolicy::AllowMultiple)
	{
		return nullptr;
	}

	for (int32 i = Stack.Num() - 1; i >= 0; --i)
	{
		if (IsStaleEntry(Stack[i]))
		{
			Stack.RemoveAt(i);
		}
	}

	for (int32 i = 0; i < Stack.Num(); ++i)
	{
		if (Stack[i].UIId == UIId)
		{
			if (Policy == EUIStackPolicy::SingleInstance)
			{
				if (IsStaleEntry(Stack[i]))
				{
					Stack.RemoveAt(i);
					ApplyInputState();
					return nullptr;
				}

				FUIStackEntry Existing = Stack[i];
				const bool bWasTop = (i == Stack.Num() - 1);
				if (!bWasTop)
				{
					Stack.RemoveAt(i);
					Stack.Add(Existing);
				}

				if (Existing.Widget && !bWasTop)
				{
					Existing.Widget->RemoveFromParent();
					Existing.Widget->AddToViewport(Existing.ZOrder);
				}

				ApplyInputState();
				return Existing.Widget;
			}
			if (Policy == EUIStackPolicy::ReplaceSameId)
			{
				if (Stack[i].Widget)
				{
					Stack[i].Widget->RemoveFromParent();
				}
				Stack.RemoveAt(i);
				break;
			}
		}
	}
	return nullptr;
}

UUserWidget* UUIStackSubsystem::AddEntry(TArray<FUIStackEntry>& Stack, const FUIStackEntry& Entry)
{
	Stack.Add(Entry);
	ApplyInputState();
	return Entry.Widget;
}

UUserWidget* UUIStackSubsystem::PushScreen(FName UIId)
{
	const FUIWidgetDefRow* Def = FindWidgetDef(UIId);
	if (!Def) return nullptr;

	if (UUserWidget* Existing = HandleExistingPolicy(ScreenStack, UIId, Def->StackPolicy))
	{
		return Existing;
	}

	UUserWidget* W = CreateWidgetFromDef(*Def);
	if (!W) return nullptr;

	W->AddToViewport(Def->ZOrder);

	FUIStackEntry E;
	E.UIId = Def->UIId;
	E.Layer = Def->Layer;
	E.ZOrder = Def->ZOrder;
	E.Widget = W;
	E.InputMode = Def->InputMode;
	E.bShowMouse = Def->bShowMouse;
	E.bBlockGameInput = Def->bBlockGameInput;
	E.bPersistAcrossMaps = Def->bPersistAcrossMaps;
	E.bCloseOnMapLoad = Def->bCloseOnMapLoad;
	return AddEntry(ScreenStack, E);
}

UUserWidget* UUIStackSubsystem::PushScreenClass(FName UIId, TSubclassOf<UUserWidget> WidgetClass, int32 ZOrder,
	EUIStackInputMode InputMode, bool bShowMouse, bool bBlockGameInput, EUIStackPolicy StackPolicy,
	bool bCloseOnMapLoad, bool bPersistAcrossMaps)
{
	if (UUserWidget* Existing = HandleExistingPolicy(ScreenStack, UIId, StackPolicy))
	{
		return Existing;
	}

	UUserWidget* W = CreateWidgetFromClass(WidgetClass);
	if (!W) return nullptr;
	W->AddToViewport(ZOrder);

	FUIStackEntry E;
	E.UIId = UIId;
	E.Layer = EUIStackLayer::Screen;
	E.ZOrder = ZOrder;
	E.Widget = W;
	E.InputMode = InputMode;
	E.bShowMouse = bShowMouse;
	E.bBlockGameInput = bBlockGameInput;
	E.bPersistAcrossMaps = bPersistAcrossMaps;
	E.bCloseOnMapLoad = bCloseOnMapLoad;

	return AddEntry(ScreenStack, E);
}

void UUIStackSubsystem::PopScreen(FName UIId)
{
	for (int32 i = ScreenStack.Num() - 1; i >= 0; --i)
	{
		if (ScreenStack[i].UIId == UIId)
		{
			if (ScreenStack[i].Widget) ScreenStack[i].Widget->RemoveFromParent();
			ScreenStack.RemoveAt(i);
			break;
		}
	}
	ApplyInputState();
}

void UUIStackSubsystem::PopTopScreen()
{
	if (ScreenStack.Num() <= 0) return;
	FUIStackEntry& Top = ScreenStack.Last();
	if (Top.Widget) Top.Widget->RemoveFromParent();
	ScreenStack.Pop();
	ApplyInputState();
}

UUserWidget* UUIStackSubsystem::ShowModal(FName UIId)
{
	const FUIWidgetDefRow* Def = FindWidgetDef(UIId);
	if (!Def) return nullptr;

	if (UUserWidget* Existing = HandleExistingPolicy(ModalStack, UIId, Def->StackPolicy))
	{
		return Existing;
	}

	UUserWidget* W = CreateWidgetFromDef(*Def);
	if (!W) return nullptr;

	W->AddToViewport(Def->ZOrder);

	FUIStackEntry E;
	E.UIId = Def->UIId;
	E.Layer = Def->Layer;
	E.ZOrder = Def->ZOrder;
	E.Widget = W;
	E.InputMode = Def->InputMode;
	E.bShowMouse = Def->bShowMouse;
	E.bBlockGameInput = Def->bBlockGameInput;
	E.bPersistAcrossMaps = Def->bPersistAcrossMaps;
	E.bCloseOnMapLoad = Def->bCloseOnMapLoad;
	return AddEntry(ModalStack, E);
}

UUserWidget* UUIStackSubsystem::ShowModalClass(FName UIId, TSubclassOf<UUserWidget> WidgetClass, int32 ZOrder,
	EUIStackInputMode InputMode, bool bShowMouse, bool bBlockGameInput, EUIStackPolicy StackPolicy,
	bool bCloseOnMapLoad, bool bPersistAcrossMaps)
{
	if (UUserWidget* Existing = HandleExistingPolicy(ModalStack, UIId, StackPolicy))
	{
		return Existing;
	}

	UUserWidget* W = CreateWidgetFromClass(WidgetClass);
	if (!W) return nullptr;
	W->AddToViewport(ZOrder);

	FUIStackEntry E;
	E.UIId = UIId;
	E.Layer = EUIStackLayer::Modal;
	E.ZOrder = ZOrder;
	E.Widget = W;
	E.InputMode = InputMode;
	E.bShowMouse = bShowMouse;
	E.bBlockGameInput = bBlockGameInput;
	E.bPersistAcrossMaps = bPersistAcrossMaps;
	E.bCloseOnMapLoad = bCloseOnMapLoad;

	return AddEntry(ModalStack, E);
}

void UUIStackSubsystem::CloseTopModal()
{
	if (ModalStack.Num() <= 0) return;
	FUIStackEntry& Top = ModalStack.Last();
	if (Top.Widget) Top.Widget->RemoveFromParent();
	ModalStack.Pop();
	ApplyInputState();
}

void UUIStackSubsystem::CloseModalById(FName UIId)
{
	for (int32 i = ModalStack.Num() - 1; i >= 0; --i)
	{
		if (ModalStack[i].UIId == UIId)
		{
			if (ModalStack[i].Widget)
			{
				ModalStack[i].Widget->RemoveFromParent();
			}
			ModalStack.RemoveAt(i);
			ApplyInputState();
			return;
		}
	}
}

void UUIStackSubsystem::CloseModalWidget(UUserWidget* Widget)
{
	const bool bRemovedFromStack = RemoveEntryByWidget(ModalStack, Widget);
	if (!bRemovedFromStack && Widget)
	{
		Widget->RemoveFromParent();
	}

	if (bRemovedFromStack)
	{
		ApplyInputState();
	}
}

void UUIStackSubsystem::CloseScreenWidget(UUserWidget* Widget)
{
	const bool bRemovedFromStack = RemoveEntryByWidget(ScreenStack, Widget);
	if (!bRemovedFromStack && Widget)
	{
		Widget->RemoveFromParent();
	}

	if (bRemovedFromStack)
	{
		ApplyInputState();
	}
}

UUserWidget* UUIStackSubsystem::ShowToastClass(FName UIId, TSubclassOf<UUserWidget> WidgetClass, int32 ZOrder,
	float DurationSeconds, bool bCloseOnMapLoad, bool bPersistAcrossMaps)
{
	UUserWidget* W = CreateWidgetFromClass(WidgetClass);
	if (!W) return nullptr;
	W->AddToViewport(ZOrder);

	FUIStackEntry E;
	E.UIId = UIId;
	E.Layer = EUIStackLayer::Toast;
	E.ZOrder = ZOrder;
	E.Widget = W;
	E.InputMode = EUIStackInputMode::GameOnly;
	E.bShowMouse = false;
	E.bBlockGameInput = false;
	E.bPersistAcrossMaps = bPersistAcrossMaps;
	E.bCloseOnMapLoad = bCloseOnMapLoad;
	ToastStack.Add(E);

	if (DurationSeconds > 0.f)
	{
		if (UWorld* World = GetWorld())
		{
			FTimerHandle Handle;
			TWeakObjectPtr<UUIStackSubsystem> WeakThis(this);
			TWeakObjectPtr<UUserWidget> WeakWidget(W);
			World->GetTimerManager().SetTimer(
				Handle,
				[WeakThis, WeakWidget]()
				{
					if (WeakThis.IsValid())
					{
						WeakThis->CloseToastWidget(WeakWidget.Get());
					}
				},
				DurationSeconds,
				false);

			ToastStack.Last().AutoCloseHandle = Handle;
		}
	}

	return W;
}

bool UUIStackSubsystem::IsScreenOpen(FName UIId) const
{
	for (const FUIStackEntry& E : ScreenStack)
	{
		if (E.UIId == UIId && !IsStaleEntry(E))
		{
			return true;
		}
	}
	return false;
}

UUserWidget* UUIStackSubsystem::GetScreenWidget(FName UIId) const
{
	for (int32 i = ScreenStack.Num() - 1; i >= 0; --i)
	{
		if (ScreenStack[i].UIId == UIId && !IsStaleEntry(ScreenStack[i]))
		{
			return ScreenStack[i].Widget;
		}
	}
	return nullptr;
}

bool UUIStackSubsystem::IsGameplayInputBlocked() const
{
	// If any modal blocks input, gameplay is blocked.
	for (const FUIStackEntry& E : ModalStack)
	{
		if (IsStaleEntry(E)) continue;
		if (E.bBlockGameInput) return true;
	}
	for (const FUIStackEntry& E : ScreenStack)
	{
		if (IsStaleEntry(E)) continue;
		if (E.bBlockGameInput) return true;
	}
	return false;
}

bool UUIStackSubsystem::IsWidgetManaged(const UUserWidget* Widget) const
{
	if (!Widget) return false;

	for (const FUIStackEntry& E : ModalStack)
	{
		if (E.Widget == Widget)
		{
			return true;
		}
	}

	for (const FUIStackEntry& E : ScreenStack)
	{
		if (E.Widget == Widget)
		{
			return true;
		}
	}

	for (const FUIStackEntry& E : ToastStack)
	{
		if (E.Widget == Widget)
		{
			return true;
		}
	}

	return false;
}

void UUIStackSubsystem::ApplyInputState()
{
	for (int32 i = ModalStack.Num() - 1; i >= 0; --i)
	{
		if (IsStaleEntry(ModalStack[i]))
		{
			ModalStack.RemoveAt(i);
		}
	}
	for (int32 i = ScreenStack.Num() - 1; i >= 0; --i)
	{
		if (IsStaleEntry(ScreenStack[i]))
		{
			ScreenStack.RemoveAt(i);
		}
	}

	UWorld* World = GetWorld();
	if (!World) return;
	APlayerController* PC = GetLocalPlayer() ? GetLocalPlayer()->GetPlayerController(World) : nullptr;
	if (!PC) return;

	// Decide the effective state: modal has priority.
	EUIStackInputMode Mode = EUIStackInputMode::GameOnly;
	bool bShowMouse = false;
	bool bBlockGameInput = false;

	if (ModalStack.Num() > 0)
	{
		const FUIStackEntry& Top = ModalStack.Last();
		Mode = Top.InputMode;
		bShowMouse = Top.bShowMouse;
		bBlockGameInput = Top.bBlockGameInput;
	}
	else if (ScreenStack.Num() > 0)
	{
		const FUIStackEntry& Top = ScreenStack.Last();
		Mode = Top.InputMode;
		bShowMouse = Top.bShowMouse;
		bBlockGameInput = Top.bBlockGameInput;
	}

	PC->bShowMouseCursor = bShowMouse;

	switch (Mode)
	{
	case EUIStackInputMode::UIOnly:
		PC->SetInputMode(FInputModeUIOnly());
		break;
	case EUIStackInputMode::GameAndUI:
		PC->SetInputMode(FInputModeGameAndUI());
		break;
	case EUIStackInputMode::GameOnly:
	default:
		PC->SetInputMode(FInputModeGameOnly());
		break;
	}

	// NOTE: gameplay input blocking is enforced by callers using IsGameplayInputBlocked().
	// This subsystem intentionally does not modify EnhancedInput mappings.
	(void)bBlockGameInput;
}

void UUIStackSubsystem::HandlePreLoadMap(const FString& MapName)
{
	// Close transient widgets before travel.
	for (int32 i = ScreenStack.Num() - 1; i >= 0; --i)
	{
		FUIStackEntry& E = ScreenStack[i];
		if (E.Widget) E.Widget->RemoveFromParent();
		if (E.bCloseOnMapLoad || !E.bPersistAcrossMaps)
		{
			ScreenStack.RemoveAt(i);
		}
		else
		{
			E.Widget = nullptr;
		}
	}
	for (int32 i = ModalStack.Num() - 1; i >= 0; --i)
	{
		FUIStackEntry& E = ModalStack[i];
		if (E.Widget) E.Widget->RemoveFromParent();
		if (E.bCloseOnMapLoad || !E.bPersistAcrossMaps)
		{
			ModalStack.RemoveAt(i);
		}
		else
		{
			E.Widget = nullptr;
		}
	}
	for (int32 i = ToastStack.Num() - 1; i >= 0; --i)
	{
		FUIStackEntry& E = ToastStack[i];
		if (E.Widget) E.Widget->RemoveFromParent();
		if (UWorld* World = GetWorld())
		{
			if (E.AutoCloseHandle.IsValid())
			{
				World->GetTimerManager().ClearTimer(E.AutoCloseHandle);
			}
		}
		if (E.bCloseOnMapLoad || !E.bPersistAcrossMaps)
		{
			ToastStack.RemoveAt(i);
		}
		else
		{
			E.Widget = nullptr;
		}
	}
	ApplyInputState();
}

void UUIStackSubsystem::CloseToastWidget(UUserWidget* Widget)
{
	if (!Widget) return;

	for (int32 i = ToastStack.Num() - 1; i >= 0; --i)
	{
		if (ToastStack[i].Widget == Widget)
		{
			if (ToastStack[i].Widget) ToastStack[i].Widget->RemoveFromParent();
			if (UWorld* World = GetWorld())
			{
				if (ToastStack[i].AutoCloseHandle.IsValid())
				{
					World->GetTimerManager().ClearTimer(ToastStack[i].AutoCloseHandle);
				}
			}
			ToastStack.RemoveAt(i);
			return;
		}
	}
}
