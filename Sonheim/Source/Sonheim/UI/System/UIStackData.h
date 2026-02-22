#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"

#include "UIStackData.generated.h"

UENUM(BlueprintType)
enum class EUIStackLayer : uint8
{
	Screen,
	Modal,
	Popup,
	Toast,
};

UENUM(BlueprintType)
enum class EUIStackInputMode : uint8
{
	GameOnly,
	GameAndUI,
	UIOnly,
};

UENUM(BlueprintType)
enum class EUIStackPolicy : uint8
{
	SingleInstance,
	ReplaceSameId,
	AllowMultiple,
};

USTRUCT(BlueprintType)
struct FUIWidgetDefRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName UIId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftClassPtr<class UUserWidget> WidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EUIStackLayer Layer = EUIStackLayer::Screen;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 ZOrder = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EUIStackInputMode InputMode = EUIStackInputMode::GameOnly;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bShowMouse = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bBlockGameInput = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EUIStackPolicy StackPolicy = EUIStackPolicy::SingleInstance;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bPersistAcrossMaps = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bCloseOnMapLoad = true;
};

USTRUCT(BlueprintType)
struct FUIWidgetPresetRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName PresetId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName TemplateUIId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DefaultTitle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DefaultBody;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText PrimaryButtonText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText SecondaryButtonText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.0", ClampMax="30.0"))
	float AutoCloseSeconds = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName StyleId = NAME_None;
};
