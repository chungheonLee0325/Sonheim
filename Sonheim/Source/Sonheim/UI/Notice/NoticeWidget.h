#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Engine/DataAsset.h"
#include "NoticeWidget.generated.h"

class UBorder;
class UImage;
class UTextBlock;
class UProgressBar;
class UTexture2D;

/** The color a notice is drawn in; what it means is the producer's choice. */
UENUM(BlueprintType)
enum class ENoticeTone : uint8 { Information, Highlight, Warning };

/** What a notice says. Where it shows and whose it is go with it to UNoticeSubsystem::Push. */
USTRUCT(BlueprintType)
struct FNoticeData
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Category;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Title;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Detail;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Symbol;
	/** Drawn on the plate in place of Symbol, in the tone's color, when set. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TSoftObjectPtr<UTexture2D> Icon;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ENoticeTone Tone = ENoticeTone::Information;
};

/** A slot's look and timing; editing them never needs a code build. */
UCLASS(BlueprintType)
class SONHEIM_API UNoticeStyle : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Category="Colors") FLinearColor Surface = FLinearColor(0.012f, 0.03f, 0.06f, 0.97f);
	UPROPERTY(EditAnywhere, Category="Colors") FLinearColor Text = FLinearColor(0.92f, 0.96f, 1.f);
	UPROPERTY(EditAnywhere, Category="Colors") FLinearColor Muted = FLinearColor(0.48f, 0.62f, 0.74f);
	UPROPERTY(EditAnywhere, Category="Colors") FLinearColor Information = FLinearColor(0.045f, 0.61f, 0.91f);
	UPROPERTY(EditAnywhere, Category="Colors") FLinearColor Highlight = FLinearColor(1.f, 0.65f, 0.13f);
	UPROPERTY(EditAnywhere, Category="Colors") FLinearColor Warning = FLinearColor(1.f, 0.29f, 0.12f);
	UPROPERTY(EditAnywhere, Category="Motion", meta=(ClampMin="0.1")) float HoldSeconds = 3.2f;
	UPROPERTY(EditAnywhere, Category="Motion", meta=(ClampMin="0.01")) float FadeSeconds = 0.22f;
	UPROPERTY(EditAnywhere, Category="Motion") float SlideDistance = 12.f;
};

/** A notice on the screen, the parent of every slot's Widget Blueprint. It shows what it is given, fades in, holds, fades out and
 * says when it is done; it never looks at game state. TitleText is the one binding every layout needs, and the rest are drawn where
 * the layout has them: a banner's card, accent and lifetime bar, a title's subtitle (DetailText). */
UCLASS(Abstract)
class SONHEIM_API UNoticeWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="Notice") void Show(const FNoticeData& Data);
	/** Takes the notice off at once, as when its channel is cleared. */
	void Stop();
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Style") TObjectPtr<UNoticeStyle> Style;
	UPROPERTY(Transient, BlueprintReadOnly, Category="Notice") FNoticeData ViewData;
	FSimpleDelegate OnFinished;
protected:
	virtual void NativePreConstruct() override;
	virtual void NativeTick(const FGeometry& Geometry, float DeltaSeconds) override;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> CategoryText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> DetailText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UBorder> Card;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UBorder> AccentLine;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UBorder> IconPlate;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> SymbolText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UImage> SymbolIcon;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UProgressBar> LifetimeBar;
private:
	void ApplyStyle();
	float Age = 0.f;
	bool bPlaying = false;
};
