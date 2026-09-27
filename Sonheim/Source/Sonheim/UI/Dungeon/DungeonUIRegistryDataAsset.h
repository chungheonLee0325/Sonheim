#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "DungeonUIRegistryDataAsset.generated.h"
class UDungeonViewWidget;
class UDungeonToastWidget;
class UDungeonToastStyle;
UENUM(BlueprintType)
enum class EDungeonUILayer : uint8 { Screen, Modal, HUD };
UENUM(BlueprintType)
enum class EDungeonUIInputMode : uint8 { GameOnly, GameAndUI, UIOnly };
USTRUCT(BlueprintType)
struct FDungeonUIEntry
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FGameplayTag UIId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftClassPtr<UDungeonViewWidget> WidgetClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) EDungeonUILayer Layer = EDungeonUILayer::HUD;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) EDungeonUIInputMode InputMode = EDungeonUIInputMode::GameOnly;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bShowMouse = false;
};
UCLASS(BlueprintType)
class SONHEIM_API UDungeonUIRegistryDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") TArray<FDungeonUIEntry> Entries;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") TSoftClassPtr<UDungeonToastWidget> ToastClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon") TObjectPtr<UDungeonToastStyle> ToastStyle;
};
