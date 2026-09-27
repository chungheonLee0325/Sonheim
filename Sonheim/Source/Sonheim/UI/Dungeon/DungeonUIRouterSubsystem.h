#pragma once
#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Engine/StreamableManager.h"
#include "DungeonViewData.h"
#include "DungeonToastWidget.h"
#include "DungeonUIRouterSubsystem.generated.h"
class UDungeonUIRegistryDataAsset;
class UDungeonStagePresenter;
class UDungeonViewWidget;
UCLASS(Config=Game)
class SONHEIM_API UDungeonUIRouterSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()
public:
	void Attach(APlayerController* Controller);
	void Detach(APlayerController* Controller);
	void ApplyView(const FDungeonStageViewData& Data);
	void ShowToast(const FDungeonToastViewData& Data);
	void ClearToasts();
	/** A room's name in the middle of the screen, while the local player takes part in a running run. */
	void ShowAreaTitle(const FText& Title, const FText& Subtitle);
	virtual void Deinitialize() override;
private:
	void RefreshWidget();
	/** Hides the registry's HiddenDuringRun screens while the local player's run goes, and gives them back afterwards. */
	void RefreshHidden();
	void PresentNextToast();
	void ToastFinished();
	UPROPERTY(Config) TSoftObjectPtr<UDungeonUIRegistryDataAsset> RegistryAsset;
	UPROPERTY(Transient) TObjectPtr<UDungeonUIRegistryDataAsset> Registry;
	UPROPERTY(Transient) TObjectPtr<UDungeonStagePresenter> Presenter;
	UPROPERTY(Transient) TObjectPtr<UDungeonViewWidget> ActiveWidget;
	UPROPERTY(Transient) TObjectPtr<UDungeonToastWidget> ToastWidget;
	TWeakObjectPtr<APlayerController> Owner;
	TSharedPtr<FStreamableHandle> RegistryLoad, WidgetLoad;
	TSharedPtr<FStreamableHandle> ToastLoad;
	TArray<FDungeonToastViewData> ToastQueue;
	bool bToastPlaying = false;
	bool bToastUnavailable = false;
	int32 ToastGeneration = 0;
	FDungeonStageViewData LatestView;
	/** The screens RefreshHidden hid, with the visibility each had. */
	TMap<TWeakObjectPtr<UUserWidget>, ESlateVisibility> HiddenWidgets;
	FName ActiveId, RequestedId;
	int32 Generation = 0;
};
