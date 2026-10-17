#include "InputHintSubsystem.h"
#include "EnhancedInputSubsystems.h"
#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "InputMappingContext.h"

/** Watches Slate's input for the device the player last used. */
class FInputHintDeviceWatcher : public IInputProcessor
{
public:
	explicit FInputHintDeviceWatcher(UInputHintSubsystem* InOwner) : Owner(InOwner) {}
	virtual void Tick(const float, FSlateApplication&, TSharedRef<ICursor>) override {}
	virtual bool HandleKeyDownEvent(FSlateApplication&, const FKeyEvent& Event) override
	{
		Note(Event.GetKey().IsGamepadKey() ? EInputHintDevice::Gamepad : EInputHintDevice::KeyboardMouse);
		return false;
	}
	virtual bool HandleAnalogInputEvent(FSlateApplication&, const FAnalogInputEvent& Event) override
	{
		// A resting stick still sends small values; only a real push counts.
		if (Event.GetKey().IsGamepadKey() && FMath::Abs(Event.GetAnalogValue()) > 0.3f) Note(EInputHintDevice::Gamepad);
		return false;
	}
	virtual bool HandleMouseMoveEvent(FSlateApplication&, const FPointerEvent& Event) override
	{
		if (Event.GetCursorDelta().SizeSquared() > 4.f) Note(EInputHintDevice::KeyboardMouse);
		return false;
	}
	virtual bool HandleMouseButtonDownEvent(FSlateApplication&, const FPointerEvent&) override
	{
		Note(EInputHintDevice::KeyboardMouse);
		return false;
	}
	virtual const TCHAR* GetDebugName() const override { return TEXT("InputHintDeviceWatcher"); }
private:
	void Note(const EInputHintDevice Used) const
	{
		if (Owner.IsValid()) Owner->NoteDevice(Used);
	}
	TWeakObjectPtr<UInputHintSubsystem> Owner;
};

void UInputHintSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadedGlyphs = GlyphSet.LoadSynchronous();
	if (auto* Input = Collection.InitializeDependency<UEnhancedInputLocalPlayerSubsystem>())
	{
		Input->ControlMappingsRebuiltDelegate.AddDynamic(this, &UInputHintSubsystem::MappingsRebuilt);
	}
	if (FSlateApplication::IsInitialized())
	{
		Watcher = MakeShared<FInputHintDeviceWatcher>(this);
		FSlateApplication::Get().RegisterInputPreProcessor(Watcher);
	}
}

void UInputHintSubsystem::Deinitialize()
{
	if (Watcher && FSlateApplication::IsInitialized()) FSlateApplication::Get().UnregisterInputPreProcessor(Watcher);
	Watcher.Reset();
	Super::Deinitialize();
}

void UInputHintSubsystem::NoteDevice(const EInputHintDevice Used)
{
	if (Used == Device) return;
	Device = Used;
	OnChanged.Broadcast();
}

bool UInputHintSubsystem::GlyphFor(const UInputAction* Action, FInputGlyph& OutGlyph) const
{
	const ULocalPlayer* Player = GetLocalPlayer();
	const auto* Input = Player ? Player->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	if (!Action || !Input) return false;
	const bool bPad = Device == EInputHintDevice::Gamepad;
	TArray<FKey> Keys = Input->QueryKeysMappedToAction(Action);
	if (Keys.IsEmpty())
		if (const UInputMappingContext* Gameplay = GameplayContext.LoadSynchronous())
			for (const FEnhancedActionKeyMapping& Mapping : Gameplay->GetMappings())
				if (Mapping.Action == Action) Keys.Add(Mapping.Key);
	for (const FKey& Key : Keys)
	{
		if (Key.IsGamepadKey() != bPad) continue;
		if (const FInputGlyph* Glyph = LoadedGlyphs ? LoadedGlyphs->Glyphs.Find(Key) : nullptr) OutGlyph = *Glyph;
		else
		{
			OutGlyph = FInputGlyph();
			OutGlyph.Label = Key.GetDisplayName(false);
			OutGlyph.Shape = OutGlyph.Label.ToString().Len() > 2 ? EInputGlyphShape::Wide : EInputGlyphShape::Key;
		}
		return true;
	}
	return false;
}

void UInputHintSubsystem::SetMenuInput(const bool bOn)
{
	if (bOn == bMenuInput) return;
	const ULocalPlayer* Player = GetLocalPlayer();
	auto* Input = Player ? Player->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	UInputMappingContext* Context = MenuContext.LoadSynchronous();
	if (!Input || !Context) return;
	bMenuInput = bOn;
	if (bOn) Input->AddMappingContext(Context, MenuContextPriority);
	else Input->RemoveMappingContext(Context);
	OnChanged.Broadcast();
}
