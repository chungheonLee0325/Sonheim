#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "GameplayTagContainer.h"
#include "GameUIScreenWidget.h"
#include "GameUIManagerSubsystem.generated.h"

class UGameUIRootWidget;
class UGameUIScreenRegistry;
class UGameUIStackLayer;
struct FGameUIScreenEntry;
struct FStreamableHandle;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnGameUIInputStateChanged, bool /*bBlockGameplay*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnGameUITopScreenChanged, FGameplayTag /*Screen*/);
using FGameUIOnReady = TFunction<void(UGameUIScreenWidget&)>;

/** A local player's UI: the root layout on the screen, the screens open in its stack layers, the input state their top screen asks
 * for, and the back input. Screens are opened by tag from the registry in UGameUISettings; a screen whose class is not loaded yet is
 * loaded asynchronously, with gameplay input held back and the root's loading indicator showing until it opens. */
UCLASS()
class GAMEUIFRAMEWORK_API UGameUIManagerSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()
public:
	static UGameUIManagerSubsystem* Get(const UUserWidget* Widget);
	static UGameUIManagerSubsystem* Get(const APlayerController* Controller);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Opens the screen, or brings it to the top of its layer if it is open; OnReady runs once it shows, in this frame when its class
	 * is loaded. */
	void Open(FGameplayTag Screen, FGameUIOnReady OnReady = nullptr);
	template <typename T>
	void Open(FGameplayTag Screen, TFunction<void(T&)> OnReady)
	{
		Open(Screen, [OnReady = MoveTemp(OnReady)](UGameUIScreenWidget& Widget) { if (T* Typed = Cast<T>(&Widget)) OnReady(*Typed); });
	}
	/** Closes the screen, or drops its opening while it loads. */
	void Close(FGameplayTag Screen);
	void CloseScreen(UGameUIScreenWidget* Screen);
	void Toggle(FGameplayTag Screen);
	/** Closes the top screen of the highest stack layer; returns false when no screen is open. */
	bool CloseTop();
	/** The back input, for the top screen to handle as its Back says; returns whether a screen took it. */
	bool HandleBack();

	UFUNCTION(BlueprintPure, Category="UI") bool IsOpen(FGameplayTag Screen) const { return Find(Screen) != nullptr; }
	UFUNCTION(BlueprintPure, Category="UI") bool IsPending(FGameplayTag Screen) const { return Pending.Contains(Screen); }
	UFUNCTION(BlueprintPure, Category="UI") bool HasOpenScreen() const { return GetTopScreen() != nullptr; }
	UFUNCTION(BlueprintPure, Category="UI") UGameUIScreenWidget* Find(FGameplayTag Screen) const;
	template <typename T>
	T* Find(FGameplayTag Screen) const { return Cast<T>(Find(Screen)); }
	/** The screen on top of the highest stack layer that has one. */
	UFUNCTION(BlueprintPure, Category="UI") UGameUIScreenWidget* GetTopScreen() const;
	/** Whether gameplay input waits: the top screen asks for it, or a screen is loading to open. */
	UFUNCTION(BlueprintPure, Category="UI") bool IsGameplayBlocked() const;
	/** Adds a widget that is no screen, such as a part of the HUD, to a plain layer of the root; RemoveFromParent takes it off. */
	void AddToLayer(FGameplayTag Layer, UUserWidget* Widget);
	/** The root on the local player's screen, made when there is none or its world is gone. */
	UGameUIRootWidget* GetRoot();

	FOnGameUIInputStateChanged OnInputStateChanged;
	FOnGameUITopScreenChanged OnTopScreenChanged;

private:
	struct FPendingOpen
	{
		int32 Id = 0;
		TArray<FGameUIOnReady> Waiting;
	};
	const UGameUIScreenRegistry* GetRegistry();
	/** Loads the entry's class, then the assets its default object asks for, then calls Done (later, or now when all is loaded). */
	void Load(const FGameUIScreenEntry& Entry, TFunction<void()> Done);
	static bool IsLoaded(const FGameUIScreenEntry& Entry);
	void FinishOpen(FGameplayTag Screen, UClass* Class);
	UGameUIStackLayer* LayerOf(const UGameUIScreenWidget* Screen);
	UGameUIRootWidget* GetLiveRoot() const;
	APlayerController* GetController() const;
	void ApplyInputState();

	UPROPERTY(Transient) TObjectPtr<UGameUIRootWidget> Root;
	UPROPERTY(Transient) TObjectPtr<const UGameUIScreenRegistry> Registry;
	/** Closed screens with bKeepWhenClosed, waiting to open again. */
	UPROPERTY(Transient) TArray<TObjectPtr<UGameUIScreenWidget>> Kept;
	TMap<FGameplayTag, FPendingOpen> Pending;
	/** Every load made, kept so a loaded screen's class and preload assets stay in memory. */
	TArray<TSharedPtr<FStreamableHandle>> Held;
	int32 LastOpenId = 0;
	FGameplayTag LastTop;
	bool bLastBlocked = false;
};
