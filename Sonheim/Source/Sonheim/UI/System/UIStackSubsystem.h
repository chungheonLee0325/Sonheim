#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "UIStackSubsystem.generated.h"

class UUserWidget;
class UMenuTabBarWidget;

/** Screen Ids, each with a rule in DefaultGame.ini. */
namespace UIScreen
{
	inline const FName Menu(TEXT("Menu"));
	inline const FName Container(TEXT("Container"));
	inline const FName Crafting(TEXT("Crafting"));
	inline const FName MonsterDex(TEXT("MonsterDex"));
	inline const FName QuestDialog(TEXT("QuestDialog"));
	inline const FName QuestJournal(TEXT("QuestJournal"));
	inline const FName WorldMap(TEXT("WorldMap"));
}

UENUM()
enum class EUIStackInputMode : uint8
{
	GameOnly,
	/** Game input still arrives, so a screen's own key (Tab, K) can close it; gameplay checks IsGameplayBlocked. */
	GameAndUI,
	UIOnly,
};

/** How one kind of screen takes the player's input, from DefaultGame.ini. */
USTRUCT()
struct FUIScreenRule
{
	GENERATED_BODY()
	UPROPERTY() FName Id;
	/** The widget Open(Id) makes; screens whose caller passes its own classes leave it empty. */
	UPROPERTY() TSoftClassPtr<UUserWidget> WidgetClass;
	UPROPERTY() int32 ZOrder = 0;
	UPROPERTY() EUIStackInputMode InputMode = EUIStackInputMode::GameAndUI;
	UPROPERTY() bool bShowMouse = true;
	UPROPERTY() bool bBlockGameInput = true;
};

USTRUCT()
struct FUIStackEntry
{
	GENERATED_BODY()
	FName Id;
	/** A screen can be several widgets that open and close together, such as the inventory beside the player's stats. */
	UPROPERTY(Transient) TArray<TObjectPtr<UUserWidget>> Widgets;
};

/** The local player's open screens, newest on top. The top screen decides the input mode, the mouse cursor and whether gameplay
 * input is held back, so opening and closing screens in any order leaves the player in the state of whatever is still open.
 * Screens are named by Id; their rules are set in DefaultGame.ini. A screen closed by any other way (its own close button,
 * the world ending) is dropped the next time the stack is touched. */
UCLASS(Config=Game)
class SONHEIM_API UUIStackSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()
public:
	/** Opens a screen made of these widgets, or brings it to the top if it is open, and returns its widgets in the same order. */
	TArray<UUserWidget*> Open(FName Id, const TArray<TSubclassOf<UUserWidget>>& Classes);
	UUserWidget* Open(FName Id, TSubclassOf<UUserWidget> Class);
	/** Opens a screen of the widget its rule names. */
	UUserWidget* Open(FName Id);
	/** Closes the screen if it is open, opens it otherwise. */
	void Toggle(FName Id);
	template <typename T>
	T* Open(FName Id, TSubclassOf<T> Class) { return Cast<T>(Open(Id, TSubclassOf<UUserWidget>(Class))); }

	void Close(FName Id);
	/** Closes the top screen; returns false when nothing is open. */
	bool CloseTop();
	/** Closes the screen a widget belongs to, through its owner's stack; a widget no stack holds is just removed. */
	static void CloseWidget(UUserWidget* Widget);

	UFUNCTION(BlueprintPure, Category="UI") bool IsOpen(FName Id) const;
	UFUNCTION(BlueprintPure, Category="UI") bool HasOpenScreen() const;
	UUserWidget* GetWidget(FName Id) const;
	/** The top screen when it is one of the menu's tabs, None otherwise. */
	FName TopTab() const;
	/** The tab Dir steps away from the top one (-1 left, 1 right), past the dimmed ones and round the ends; None when no tab is on top. */
	FName StepTab(int32 Dir) const;
	/** Whether the top screen holds back moving, looking, attacking and the other gameplay input. */
	UFUNCTION(BlueprintPure, Category="UI") bool IsGameplayBlocked() const;

private:
	const FUIScreenRule& RuleOf(FName Id) const;
	void RemoveAt(int32 Index);
	/** Whether a screen is still showing; one whose widgets left the screen some other way is not. */
	static bool IsLive(const FUIStackEntry& Entry);
	/** Drops screens that are no longer live. */
	void Prune();
	void ApplyInputState();
	/** Shows the tab bar over a tab's screen, lit on that tab, and takes it away otherwise. */
	void RefreshTabBar();
	const UMenuTabBarWidget* TabBarDefaults() const;
	APlayerController* GetController() const;

	UPROPERTY(Config) TArray<FUIScreenRule> Screens;
	UPROPERTY(Transient) TArray<FUIStackEntry> Stack;
	/** The bar across the top of the menu's screens; its tabs name the screens it shows over. */
	UPROPERTY(Config) TSoftClassPtr<UMenuTabBarWidget> TabBarClass;
	UPROPERTY(Config) int32 TabBarZOrder = 50;
	UPROPERTY(Transient) TObjectPtr<UMenuTabBarWidget> TabBar;
};
