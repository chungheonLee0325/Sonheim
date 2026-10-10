#include "UIStackSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Sonheim/Utilities/LogMacro.h"

TArray<UUserWidget*> UUIStackSubsystem::Open(const FName Id, const TArray<TSubclassOf<UUserWidget>>& Classes)
{
	Prune();
	TArray<UUserWidget*> Result;
	const int32 Existing = Stack.IndexOfByPredicate([Id](const FUIStackEntry& Entry) { return Entry.Id == Id; });
	if (Existing != INDEX_NONE)
	{
		FUIStackEntry Entry = Stack[Existing];
		if (Existing != Stack.Num() - 1)
		{
			Stack.RemoveAt(Existing);
			Stack.Add(Entry);
			// Re-adding puts the screen above the others of its Z order.
			for (UUserWidget* Widget : Entry.Widgets)
			{
				Widget->RemoveFromParent();
				Widget->AddToViewport(RuleOf(Id).ZOrder);
			}
		}
		Result.Append(Entry.Widgets);
		ApplyInputState();
		return Result;
	}

	APlayerController* PC = GetController();
	if (!PC) return Result;
	FUIStackEntry Entry;
	Entry.Id = Id;
	for (const TSubclassOf<UUserWidget>& Class : Classes)
	{
		UUserWidget* Widget = Class ? CreateWidget<UUserWidget>(PC, Class) : nullptr;
		if (!Widget)
		{
			UE_LOG(SONHEIM, Error, TEXT("[UIStack] Screen %s could not make a widget of %s"), *Id.ToString(), *GetNameSafe(Class));
			for (UUserWidget* Made : Entry.Widgets) Made->RemoveFromParent();
			return {};
		}
		Widget->AddToViewport(RuleOf(Id).ZOrder);
		Entry.Widgets.Add(Widget);
	}
	Stack.Add(Entry);
	Result.Append(Entry.Widgets);
	ApplyInputState();
	return Result;
}

UUserWidget* UUIStackSubsystem::Open(const FName Id, const TSubclassOf<UUserWidget> Class)
{
	const TArray<UUserWidget*> Widgets = Open(Id, TArray<TSubclassOf<UUserWidget>>{Class});
	return Widgets.Num() ? Widgets[0] : nullptr;
}

void UUIStackSubsystem::Close(const FName Id)
{
	const int32 Index = Stack.IndexOfByPredicate([Id](const FUIStackEntry& Entry) { return Entry.Id == Id; });
	if (Index != INDEX_NONE) RemoveAt(Index);
	Prune();
	ApplyInputState();
}

bool UUIStackSubsystem::CloseTop()
{
	Prune();
	if (Stack.IsEmpty()) return false;
	RemoveAt(Stack.Num() - 1);
	ApplyInputState();
	return true;
}

void UUIStackSubsystem::CloseWidget(UUserWidget* Widget)
{
	if (!Widget) return;
	const ULocalPlayer* Player = Widget->GetOwningLocalPlayer();
	if (UUIStackSubsystem* UIStack = Player ? Player->GetSubsystem<UUIStackSubsystem>() : nullptr)
	{
		const int32 Index = UIStack->Stack.IndexOfByPredicate([Widget](const FUIStackEntry& Entry) { return Entry.Widgets.Contains(Widget); });
		if (Index != INDEX_NONE)
		{
			UIStack->RemoveAt(Index);
			UIStack->Prune();
			UIStack->ApplyInputState();
			return;
		}
	}
	Widget->RemoveFromParent();
}

bool UUIStackSubsystem::IsOpen(const FName Id) const
{
	return GetWidget(Id) != nullptr;
}

bool UUIStackSubsystem::HasOpenScreen() const
{
	return Stack.ContainsByPredicate(&UUIStackSubsystem::IsLive);
}

UUserWidget* UUIStackSubsystem::GetWidget(const FName Id) const
{
	const FUIStackEntry* Entry = Stack.FindByPredicate([Id](const FUIStackEntry& E) { return E.Id == Id && IsLive(E); });
	return Entry ? Entry->Widgets[0].Get() : nullptr;
}

bool UUIStackSubsystem::IsGameplayBlocked() const
{
	for (int32 i = Stack.Num() - 1; i >= 0; --i)
	{
		if (IsLive(Stack[i])) return RuleOf(Stack[i].Id).bBlockGameInput;
	}
	return false;
}

const FUIScreenRule& UUIStackSubsystem::RuleOf(const FName Id) const
{
	if (const FUIScreenRule* Rule = Screens.FindByPredicate([Id](const FUIScreenRule& R) { return R.Id == Id; }))
	{
		return *Rule;
	}
	ensureMsgf(false, TEXT("[UIStack] No rule for screen %s in [/Script/Sonheim.UIStackSubsystem]"), *Id.ToString());
	static const FUIScreenRule Fallback;
	return Fallback;
}

void UUIStackSubsystem::RemoveAt(const int32 Index)
{
	// Taken off the stack first: a widget's NativeDestruct may close other screens through this stack.
	const FUIStackEntry Entry = Stack[Index];
	Stack.RemoveAt(Index);
	for (UUserWidget* Widget : Entry.Widgets)
	{
		if (Widget) Widget->RemoveFromParent();
	}
}

bool UUIStackSubsystem::IsLive(const FUIStackEntry& Entry)
{
	return Entry.Widgets.Num() && Entry.Widgets.ContainsByPredicate([](const UUserWidget* Widget) { return IsValid(Widget) && Widget->IsInViewport(); });
}

void UUIStackSubsystem::Prune()
{
	for (int32 i = Stack.Num() - 1; i >= 0; --i)
	{
		// A closing widget can close other screens from its NativeDestruct, so the stack may shrink under this loop.
		if (i < Stack.Num() && !IsLive(Stack[i])) RemoveAt(i);
	}
}

void UUIStackSubsystem::ApplyInputState()
{
	APlayerController* PC = GetController();
	if (!PC) return;
	// Callers prune first, so the last screen is the one showing on top.
	const FUIScreenRule Rule = Stack.Num() ? RuleOf(Stack.Last().Id) : FUIScreenRule{NAME_None, 0, EUIStackInputMode::GameOnly, false, false};
	PC->SetShowMouseCursor(Rule.bShowMouse);
	switch (Rule.InputMode)
	{
	case EUIStackInputMode::UIOnly:
		PC->SetInputMode(FInputModeUIOnly());
		break;
	case EUIStackInputMode::GameAndUI:
		{
			FInputModeGameAndUI Mode;
			Mode.SetHideCursorDuringCapture(false);
			PC->SetInputMode(Mode);
			break;
		}
	default:
		PC->SetInputMode(FInputModeGameOnly());
		break;
	}
}

APlayerController* UUIStackSubsystem::GetController() const
{
	const ULocalPlayer* Player = GetLocalPlayer();
	return Player ? Player->GetPlayerController(Player->GetWorld()) : nullptr;
}
