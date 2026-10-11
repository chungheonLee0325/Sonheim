#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Engine/DataAsset.h"
#include "NoticeWidget.generated.h"

class UBorder;
class UDynamicEntryBox;
class UImage;
class UTextBlock;
class UProgressBar;
class UTexture2D;
class UWidget;

/** The colour a notice is drawn in, by what the message is (Art/Style/ui_style.md, colour roles). */
UENUM(BlueprintType)
enum class ENoticeTone : uint8
{
	/** Cyan: news, a quest taken, a level gained, a first capture. */
	Information,
	/** Gold: a reward, a boss down, something waiting, a caution. */
	Highlight,
	/** Red: danger, a failure. */
	Warning,
	/** Green: done, a quest finished. */
	Success,
	/** Orange: changed, a quest's next step. */
	Update,
	/** Purple: a guardian's or alpha's moment. */
	Legend,
};

/** A v2 icon (Art/Requests/ui_v2.json) by its texture's name, such as TEXT("T_IconQuestNew"). */
inline TSoftObjectPtr<UTexture2D> NoticeIcon(const TCHAR* Texture)
{
	return TSoftObjectPtr<UTexture2D>(FSoftObjectPath(FString::Printf(TEXT("/Game/UI/Icons/%s.%s"), Texture, Texture)));
}

/** One line under a big notice: a stat gained or an item paid, as an icon, a name and a value. */
USTRUCT(BlueprintType)
struct FNoticeLine
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TSoftObjectPtr<UTexture2D> Icon;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Label;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Value;
};

/** What a notice says. Where it shows and whose it is go with it to UNoticeSubsystem::Push. */
USTRUCT(BlueprintType)
struct FNoticeData
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Category;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Title;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Detail;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Symbol;
	/** Drawn on the plate in place of Symbol, in the tone's colour, when set. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TSoftObjectPtr<UTexture2D> Icon;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ENoticeTone Tone = ENoticeTone::Information;
	/** A big notice's list under its title: stats gained, items paid. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FNoticeLine> Lines;
	/** A big notice's last line, set off by a rule, such as the points a level gave. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Footer;
	/** A picture beside a big notice's text, such as a species met for the first time; drawn in its own colours. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TSoftObjectPtr<UTexture2D> Portrait;
};

/** A slot's look and timing; editing them never needs a code build. */
UCLASS(BlueprintType)
class SONHEIM_API UNoticeStyle : public UDataAsset
{
	GENERATED_BODY()
public:
	FLinearColor AccentOf(ENoticeTone Tone) const;

	UPROPERTY(EditAnywhere, Category="Colors") FLinearColor Surface = FLinearColor(0.012f, 0.03f, 0.06f, 0.97f);
	UPROPERTY(EditAnywhere, Category="Colors") FLinearColor Text = FLinearColor(0.92f, 0.96f, 1.f);
	UPROPERTY(EditAnywhere, Category="Colors") FLinearColor Muted = FLinearColor(0.48f, 0.62f, 0.74f);
	UPROPERTY(EditAnywhere, Category="Colors") FLinearColor Information = FLinearColor(0.045f, 0.61f, 0.91f);
	UPROPERTY(EditAnywhere, Category="Colors") FLinearColor Highlight = FLinearColor(1.f, 0.65f, 0.13f);
	UPROPERTY(EditAnywhere, Category="Colors") FLinearColor Warning = FLinearColor(1.f, 0.29f, 0.12f);
	UPROPERTY(EditAnywhere, Category="Colors") FLinearColor Success = FLinearColor(0.035f, 0.64f, 0.2f);
	UPROPERTY(EditAnywhere, Category="Colors") FLinearColor Update = FLinearColor(1.f, 0.35f, 0.012f);
	UPROPERTY(EditAnywhere, Category="Colors") FLinearColor Legend = FLinearColor(0.33f, 0.08f, 1.f);
	UPROPERTY(EditAnywhere, Category="Motion", meta=(ClampMin="0.1")) float HoldSeconds = 3.2f;
	UPROPERTY(EditAnywhere, Category="Motion", meta=(ClampMin="0.01")) float FadeSeconds = 0.22f;
	/** Where the notice comes in from, relative to where it rests; it eases back as it fades in. */
	UPROPERTY(EditAnywhere, Category="Motion") FVector2D SlideFrom = FVector2D(0.f, -12.f);
	/** The scale the notice grows from as it fades in; 1 is none. */
	UPROPERTY(EditAnywhere, Category="Motion") float ScaleFrom = 1.f;
};

/** A line of a big notice: its icon, name and value, the value in the notice's colour. */
UCLASS(Abstract)
class SONHEIM_API UNoticeLineWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void Show(const FNoticeLine& Line, const FLinearColor& Accent);
protected:
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UImage> IconImage;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> LabelText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> ValueText;
};

/** A notice on the screen, the parent of every slot's Widget Blueprint. It shows what it is given, fades in, holds, fades out and
 * says when it is done; it never looks at game state. Every binding is optional and drawn where the layout has it: a feed row's icon
 * plate and accent, a headline's band, emblem, lines and footer, a title's subtitle. */
UCLASS(Abstract)
class SONHEIM_API UNoticeWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="Notice") void Show(const FNoticeData& Data) { ShowFrom(NAME_None, Data); }
	/** Shows a notice that came on a channel; a slot that stacks keeps each one's channel so it can take them back. */
	virtual void ShowFrom(FName Channel, const FNoticeData& Data);
	/** Takes the notice off at once, as when its channel is cleared. */
	void Stop();
	/** Takes off what came on a channel, for a widget that shows several at once; one that shows a single notice leaves it to its
	 * slot, which stops it. */
	virtual void StopChannel(FName Channel) {}
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Style") TObjectPtr<UNoticeStyle> Style;
	UPROPERTY(Transient, BlueprintReadOnly, Category="Notice") FNoticeData ViewData;
	FName ShowingChannel;
	FSimpleDelegate OnFinished;
protected:
	virtual void NativePreConstruct() override;
	virtual void NativeTick(const FGeometry& Geometry, float DeltaSeconds) override;
	const UNoticeStyle& Theme() const { return Style ? *Style : *GetDefault<UNoticeStyle>(); }
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> CategoryText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> DetailText;
	/** What moves as the notice comes in; the whole widget when the layout has no card. */
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UWidget> Card;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UBorder> AccentLine;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UBorder> IconPlate;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> SymbolText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UImage> SymbolIcon;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UProgressBar> LifetimeBar;
	/** A headline's band and its two edge lines, and the emblem behind its symbol, all tinted with the tone's colour. */
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UImage> BandImage;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UImage> BandEdgeTop;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UImage> BandEdgeBottom;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UBorder> EmblemPlate;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UDynamicEntryBox> LineBox;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> FooterText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UWidget> FooterRow;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UImage> PortraitImage;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UWidget> PortraitPanel;
	/** A light that crosses the card once as the notice comes in, left to right over SweepSeconds. */
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UWidget> Sweep;
	UPROPERTY(EditAnywhere, Category="Notice") float SweepSeconds = 0.7f;
private:
	void ApplyStyle();
	float Age = 0.f;
	bool bPlaying = false;
};

/** The feed: a column of notices, each a row of its own that comes in, holds and leaves on its own time. A new one goes at the
 * bottom; past MaxShown the oldest leaves at once. */
UCLASS(Abstract)
class SONHEIM_API UNoticeFeedWidget : public UNoticeWidget
{
	GENERATED_BODY()
public:
	virtual void ShowFrom(FName Channel, const FNoticeData& Data) override;
	virtual void StopChannel(FName Channel) override;
protected:
	virtual void NativeTick(const FGeometry& Geometry, float DeltaSeconds) override;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UDynamicEntryBox> RowBox;
	UPROPERTY(EditAnywhere, Category="Notice") int32 MaxShown = 3;
};
