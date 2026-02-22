#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "TimerManager.h"

#include "Sonheim/UI/System/UIStackData.h"

#include "UIStackSubsystem.generated.h"

USTRUCT()
struct FUIStackEntry
{
	GENERATED_BODY()

	UPROPERTY()
	FName UIId = NAME_None;

	UPROPERTY()
	EUIStackLayer Layer = EUIStackLayer::Screen;

	UPROPERTY()
	int32 ZOrder = 0;

	UPROPERTY()
	TObjectPtr<class UUserWidget> Widget = nullptr;

	UPROPERTY()
	EUIStackInputMode InputMode = EUIStackInputMode::GameOnly;

	UPROPERTY()
	bool bShowMouse = false;

	UPROPERTY()
	bool bBlockGameInput = false;

	UPROPERTY()
	bool bPersistAcrossMaps = false;

	UPROPERTY()
	bool bCloseOnMapLoad = true;

	// Toast auto-close timer (used only for toast entries)
	FTimerHandle AutoCloseHandle;
};

UCLASS()
class SONHEIM_API UUIStackSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category="UI")
	UUserWidget* PushScreen(FName UIId);

	// Migration-friendly path: open a screen without requiring a DataTable row.
	UUserWidget* PushScreenClass(FName UIId, TSubclassOf<UUserWidget> WidgetClass, int32 ZOrder,
	                            EUIStackInputMode InputMode, bool bShowMouse, bool bBlockGameInput,
	                            EUIStackPolicy StackPolicy = EUIStackPolicy::SingleInstance,
	                            bool bCloseOnMapLoad = true, bool bPersistAcrossMaps = false);

	UFUNCTION(BlueprintCallable, Category="UI")
	void PopScreen(FName UIId);

	UFUNCTION(BlueprintCallable, Category="UI")
	void PopTopScreen();

	UFUNCTION(BlueprintCallable, Category="UI")
	UUserWidget* ShowModal(FName UIId);

	UUserWidget* ShowModalClass(FName UIId, TSubclassOf<UUserWidget> WidgetClass, int32 ZOrder,
	                           EUIStackInputMode InputMode, bool bShowMouse, bool bBlockGameInput,
	                           EUIStackPolicy StackPolicy = EUIStackPolicy::SingleInstance,
	                           bool bCloseOnMapLoad = true, bool bPersistAcrossMaps = false);

	UFUNCTION(BlueprintCallable, Category="UI")
	void CloseTopModal();

	UFUNCTION(BlueprintCallable, Category="UI")
	void CloseModalById(FName UIId);

	UFUNCTION(BlueprintCallable, Category="UI")
	void CloseModalWidget(UUserWidget* Widget);

	UFUNCTION(BlueprintCallable, Category="UI")
	void CloseScreenWidget(UUserWidget* Widget);

	UFUNCTION(BlueprintCallable, Category="UI")
	UUserWidget* ShowToastClass(FName UIId, TSubclassOf<UUserWidget> WidgetClass, int32 ZOrder,
	                            float DurationSeconds, bool bCloseOnMapLoad = true,
	                            bool bPersistAcrossMaps = false);

	UFUNCTION(BlueprintPure, Category="UI")
	bool IsScreenOpen(FName UIId) const;

	UFUNCTION(BlueprintPure, Category="UI")
	UUserWidget* GetScreenWidget(FName UIId) const;

	UFUNCTION(BlueprintPure, Category="UI")
	bool IsGameplayInputBlocked() const;

	UFUNCTION(BlueprintPure, Category="UI")
	bool IsWidgetManaged(const UUserWidget* Widget) const;

private:
	const FUIWidgetDefRow* FindWidgetDef(FName UIId) const;
	UUserWidget* CreateWidgetFromDef(const FUIWidgetDefRow& Def);
	UUserWidget* CreateWidgetFromClass(TSubclassOf<UUserWidget> WidgetClass) const;
	UUserWidget* HandleExistingPolicy(TArray<FUIStackEntry>& Stack, FName UIId, EUIStackPolicy Policy);
	UUserWidget* AddEntry(TArray<FUIStackEntry>& Stack, const FUIStackEntry& Entry);
	bool RemoveEntryByWidget(TArray<FUIStackEntry>& Stack, UUserWidget* Widget);
	bool IsStaleEntry(const FUIStackEntry& Entry) const;
	void ApplyInputState();
	void HandlePreLoadMap(const FString& MapName);
	void CloseToastWidget(UUserWidget* Widget);

private:
	UPROPERTY()
	TObjectPtr<UDataTable> WidgetDefTable = nullptr;
	UPROPERTY()
	TObjectPtr<UDataTable> PresetTable = nullptr;

	UPROPERTY()
	TArray<FUIStackEntry> ScreenStack;
	UPROPERTY()
	TArray<FUIStackEntry> ModalStack;
	UPROPERTY()
	TArray<FUIStackEntry> ToastStack;

	FDelegateHandle PreLoadMapHandle;
};
