#pragma once
#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Engine/StreamableManager.h"
#include "DungeonViewData.h"
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
	/** A room's name in the notices' Title slot, while the local player takes part in a running run. */
	void ShowAreaTitle(const FText& Title, const FText& Subtitle);
	/** The channel of the run's notices, its banners and its rooms' names, which the run takes back when it ends. */
	static const FName NoticeChannel;
	virtual void Deinitialize() override;
private:
	void RefreshWidget();
	/** Hides the registry's HiddenDuringRun screens while the local player's run goes, and gives them back afterwards. */
	void RefreshHidden();
	UPROPERTY(Config) TSoftObjectPtr<UDungeonUIRegistryDataAsset> RegistryAsset;
	UPROPERTY(Transient) TObjectPtr<UDungeonUIRegistryDataAsset> Registry;
	UPROPERTY(Transient) TObjectPtr<UDungeonStagePresenter> Presenter;
	UPROPERTY(Transient) TObjectPtr<UDungeonViewWidget> ActiveWidget;
	TWeakObjectPtr<APlayerController> Owner;
	TSharedPtr<FStreamableHandle> RegistryLoad, WidgetLoad;
	FDungeonStageViewData LatestView;
	/** The screens RefreshHidden hid, with the visibility each had. */
	TMap<TWeakObjectPtr<UUserWidget>, ESlateVisibility> HiddenWidgets;
	FName ActiveId, RequestedId;
	int32 Generation = 0;
};
