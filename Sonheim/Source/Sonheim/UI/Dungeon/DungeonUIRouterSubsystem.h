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
	virtual void Deinitialize() override;
private:
	void RefreshWidget();
	UPROPERTY(Config) TSoftObjectPtr<UDungeonUIRegistryDataAsset> RegistryAsset;
	UPROPERTY(Transient) TObjectPtr<UDungeonUIRegistryDataAsset> Registry;
	UPROPERTY(Transient) TObjectPtr<UDungeonStagePresenter> Presenter;
	UPROPERTY(Transient) TObjectPtr<UDungeonViewWidget> ActiveWidget;
	TWeakObjectPtr<APlayerController> Owner;
	TSharedPtr<FStreamableHandle> RegistryLoad, WidgetLoad;
	FDungeonStageViewData LatestView;
	FName ActiveId, RequestedId;
	int32 Generation = 0;
};
