#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "InputCoreTypes.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "InputHintSubsystem.generated.h"

class UInputAction;
class UInputMappingContext;

/** The kind of device the player last used; key hints show that device's key. */
UENUM(BlueprintType)
enum class EInputHintDevice : uint8
{
	KeyboardMouse,
	Gamepad,
};

/** The plate a key is drawn on: a key cap, a wide key, a round face button, a shoulder bumper. */
UENUM(BlueprintType)
enum class EInputGlyphShape : uint8
{
	Key,
	Wide,
	Round,
	Bumper,
};

/** How one key is drawn: its label on a plate of a shape and colour. */
USTRUCT(BlueprintType)
struct FInputGlyph
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FText Label;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) EInputGlyphShape Shape = EInputGlyphShape::Key;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FLinearColor Plate = FLinearColor::White;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FLinearColor Ink = FLinearColor(0.003f, 0.008f, 0.019f);
};

/** The look of every key the hints may show, keyboard and gamepad alike; a key missing here is drawn as its display name. */
UCLASS(BlueprintType)
class SONHEIM_API UInputGlyphSet : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Category="Glyphs") TMap<FKey, FInputGlyph> Glyphs;
};

DECLARE_MULTICAST_DELEGATE(FOnInputHintsChanged);

/**
 * What the local player's key hints show. It follows the last device the player touched (a gamepad button or stick, or the keyboard
 * and mouse), asks Enhanced Input which keys an input action is mapped to, and keeps the menu's own mappings (IMC_Menu: tabs,
 * close, confirm) on while the UI manager shows a screen that holds back gameplay. Hints redraw on OnChanged.
 */
UCLASS(Config=Game)
class SONHEIM_API UInputHintSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** The glyph of the key Action is mapped to for the current device; false when no key of that device is mapped. */
	bool GlyphFor(const UInputAction* Action, FInputGlyph& OutGlyph) const;
	EInputHintDevice GetDevice() const { return Device; }
	/** Turns the menu's mappings on or off; the UI manager's OnInputStateChanged calls it as screens open and close. */
	void SetMenuInput(bool bOn);

	FOnInputHintsChanged OnChanged;

	/** Notes the device the player used; the watcher calls it on input, and a settings screen may set it. */
	UFUNCTION(BlueprintCallable, Category="Input") void NoteDevice(EInputHintDevice Used);

private:
	/** Enhanced Input rebuilds its mappings a frame after a context changes; hints redraw then, with the keys now mapped. */
	UFUNCTION() void MappingsRebuilt() { OnChanged.Broadcast(); }
	UPROPERTY(Config) TSoftObjectPtr<UInputGlyphSet> GlyphSet;
	UPROPERTY(Config) TSoftObjectPtr<UInputMappingContext> MenuContext;
	/** Where a gameplay action's key is looked up while the menu's context holds that key (F is the menu's confirm on a screen, so
	 * a screen naming the interaction key still shows F). */
	UPROPERTY(Config) TSoftObjectPtr<UInputMappingContext> GameplayContext;
	UPROPERTY(Config) int32 MenuContextPriority = 10;
	UPROPERTY(Transient) TObjectPtr<UInputGlyphSet> LoadedGlyphs;
	EInputHintDevice Device = EInputHintDevice::KeyboardMouse;
	bool bMenuInput = false;
	TSharedPtr<class FInputHintDeviceWatcher> Watcher;
};
