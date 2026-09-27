#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Engine/DataAsset.h"
#include "DungeonToastWidget.generated.h"

class UBorder;
class UTextBlock;
class UProgressBar;

UENUM(BlueprintType)
enum class EDungeonToastTone : uint8 { Information, Route, Warning };

USTRUCT(BlueprintType)
struct FDungeonToastViewData
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Category;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Title;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Detail;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Symbol;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) EDungeonToastTone Tone = EDungeonToastTone::Information;
};

/** Shared by every toast state; editing colors or timing never needs a code build. */
UCLASS(BlueprintType)
class SONHEIM_API UDungeonToastStyle : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Category="Colors") FLinearColor Surface = FLinearColor(0.012f, 0.03f, 0.06f, 0.97f);
	UPROPERTY(EditAnywhere, Category="Colors") FLinearColor Text = FLinearColor(0.92f, 0.96f, 1.f);
	UPROPERTY(EditAnywhere, Category="Colors") FLinearColor Muted = FLinearColor(0.48f, 0.62f, 0.74f);
	UPROPERTY(EditAnywhere, Category="Colors") FLinearColor Information = FLinearColor(0.045f, 0.61f, 0.91f);
	UPROPERTY(EditAnywhere, Category="Colors") FLinearColor Route = FLinearColor(1.f, 0.65f, 0.13f);
	UPROPERTY(EditAnywhere, Category="Colors") FLinearColor Warning = FLinearColor(1.f, 0.29f, 0.12f);
	UPROPERTY(EditAnywhere, Category="Motion", meta=(ClampMin="0.1")) float HoldSeconds = 3.2f;
	UPROPERTY(EditAnywhere, Category="Motion", meta=(ClampMin="0.01")) float FadeSeconds = 0.22f;
	UPROPERTY(EditAnywhere, Category="Motion") float SlideDistance = 12.f;
};

/** Reusable passive toast. It consumes display data, never queries dungeon state. */
UCLASS(Abstract)
class SONHEIM_API UDungeonToastWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="Dungeon|UI") void ShowToast(const FDungeonToastViewData& Data);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Style") TObjectPtr<UDungeonToastStyle> Style;
	UPROPERTY(Transient, BlueprintReadOnly, Category="Dungeon|UI") FDungeonToastViewData ViewData;
	FSimpleDelegate OnFinished;
protected:
	virtual void NativePreConstruct() override;
	virtual void NativeTick(const FGeometry& Geometry, float DeltaSeconds) override;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UBorder> Card;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UBorder> AccentLine;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UBorder> IconPlate;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> SymbolText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> CategoryText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> DetailText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UProgressBar> LifetimeBar;
private:
	void ApplyStyle();
	float Age = 0.f;
	bool bPlaying = false;
};
