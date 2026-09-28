#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Sonheim/ResourceManager/SonheimGameType.h"
#include "BossPatternDataAsset.generated.h"

class ABaseElement;
class UAnimMontage;

/** The ground a strike covers. Its mark on the ground has the same shape and stays until the strike lands. */
UENUM(BlueprintType)
enum class EBossAreaShape : uint8
{
	/** No mark and no area damage: the strike only fires its projectiles. */
	None,
	/** Radius around the anchor. */
	Circle,
	/** Between InnerRadius and Radius; the inside is safe. */
	Ring,
	/** Radius long, HalfAngle to either side of the facing. */
	Cone,
	/** Radius long from the anchor along the facing, HalfWidth to either side. */
	Line,
};

UENUM(BlueprintType)
enum class EBossAreaAnchor : uint8
{
	/** On the boss: the mark turns and moves with it until the strike lands. */
	Boss,
	/** Where the target stood when the mark appeared, facing away from the boss. The mark stays there. */
	Target,
};

UENUM(BlueprintType)
enum class EBossProjectileAim : uint8
{
	/** Along a direction from the boss: LightningBall flies straight, ElectricBall then follows the target. */
	Direction,
	/** At a spot on the ground: BladeWind is thrown there. Every spot gets its own mark, the way an area strike's spots do. */
	Location,
};

USTRUCT(BlueprintType)
struct FBossSectionCue
{
	GENERATED_BODY()
	/** Seconds from the start of the pattern. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss") float Seconds = 0.f;
	/** The montage section to jump to, such as the release after a charge that loops until then. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss") FName Section;
};

/** One hit of a pattern: its mark appears on the ground, and after a while the strike lands where the mark is. */
USTRUCT(BlueprintType)
struct FBossStrike
{
	GENERATED_BODY()
	/** When the mark appears and when the strike lands, in seconds from the start of the pattern. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss") float MarkSeconds = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss") float StrikeSeconds = 1.f;
	/** The phase the strike joins in, such as an outer ring that only phase 2 adds. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss", meta=(ClampMin="1", ClampMax="2")) int32 MinPhase = 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss") EBossAreaShape Shape = EBossAreaShape::Circle;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss") EBossAreaAnchor Anchor = EBossAreaAnchor::Boss;
	/** Circle and Ring: the radius. Cone and Line: the length. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss") float Radius = 400.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss") float InnerRadius = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss") float HalfAngle = 45.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss") float HalfWidth = 100.f;
	/** Moves the area ahead along the facing, such as a claw that reaches in front of the boss. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss") float ForwardOffset = 0.f;
	/** Damage to everyone inside the area as the strike lands, or of each projectile. Its FireVFX_N plays on every area as it lands. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss") FAttackData Attack;
	/** A strike with projectiles fires them in place of hitting its area; its marks show where they go. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss") TSubclassOf<ABaseElement> Projectile;
	/** Areas or projectiles the strike makes. Several areas on the target mark and hit that many spots at once: the first where the
	 * target stands, the others within Scatter of it, or across SpreadDegrees at the target's distance while Scatter is 0. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss", meta=(ClampMin="1")) int32 Count = 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss") float Scatter = 0.f;
	/** Degrees the projectiles, or the spots, spread over, centred on the facing. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss") float SpreadDegrees = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss") EBossProjectileAim Aim = EBossProjectileAim::Direction;
};

USTRUCT(BlueprintType)
struct FBossPattern
{
	GENERATED_BODY()
	/** Names the pattern on the boss's status, which the dungeon screen shows by its label. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss", meta=(Categories="Boss")) FGameplayTag PatternId;
	/** Plays from its first section; the cues jump between its sections. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss") TObjectPtr<UAnimMontage> Montage;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss") TArray<FBossSectionCue> Cues;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss") TArray<FBossStrike> Strikes;
	/** Length of the pattern with its recovery; the boss picks the next one after it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss", meta=(ClampMin="0.1")) float Seconds = 3.f;
	/** The boss turns to its target until then and holds its facing after, so a mark on the boss stops turning before it lands. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss") float TrackSeconds = 0.5f;
	/** Distance to the target at which the boss picks the pattern. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss") float MinRange = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss") float MaxRange = 1500.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss", meta=(ClampMin="0")) float Weight = 1.f;
	/** Seconds before the boss picks the pattern again. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss") float Cooldown = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss", meta=(ClampMin="1", ClampMax="2")) int32 MinPhase = 1;
	/** Share of the montage's root motion the boss follows: the claw swings were animated to lunge further than the hall is wide. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss", meta=(ClampMin="0", ClampMax="1")) float RootMotionScale = 1.f;
	/** From LeapStart to LeapEnd seconds the boss flies in a high arc onto the pattern's first mark on the target, or onto the target,
	 * and comes down fast; the montage's Land section plays as it touches down. No leap while LeapEnd is 0. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss") float LeapStartSeconds = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss") float LeapEndSeconds = 0.f;
	/** How high the leap rises over the higher of its two ends. The ceiling above the boss's head bounds it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss") float LeapHeight = 300.f;
};

/** How a boss fights: its patterns, how it wakes, the phase its health turns it to, and when it can be captured. */
UCLASS(BlueprintType)
class SONHEIM_API UBossPatternDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	// Sections the boss's own montages carry: the wake montage Sleep (looping), Wake and Roar; the down montage Fall, Down (looping) and GetUp;
	// a leaping pattern's montage and the hop montage Land, which plays as the boss touches down.
	static const FName SleepSection;
	static const FName WakeSection;
	static const FName RoarSection;
	static const FName FallSection;
	static const FName DownSection;
	static const FName GetUpSection;
	static const FName LandSection;
	/** The least time between a mark and its strike: long enough to step out of the area. */
	static constexpr float MinWarningSeconds = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Patterns", meta=(TitleProperty="PatternId")) TArray<FBossPattern> Patterns;

	/** The boss sleeps in the Sleep section until a player comes within WakeRadius or hits it, then plays Wake and Roar. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Intro") TObjectPtr<UAnimMontage> WakeMontage;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Intro") float WakeRadius = 1200.f;
	/** From waking to the first pattern. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Intro") float WakeSeconds = 3.8f;
	/** Players farther than this from where the boss spawned are out of its reach; with none within it, the boss walks back to its spot. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Intro") float ArenaRadius = 2500.f;

	/** Between two patterns the boss keeps moving for a while, facing its target: it hops back from a target closer than BackOffRange,
	 * now and then hops to the side of one further off, and otherwise walks toward it. Phase 2 shortens the while. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Patterns") float GapSecondsMin = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Patterns") float GapSecondsMax = 2.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Patterns", meta=(ClampMin="0")) float BackOffRange = 500.f;
	/** How far one hop carries the boss. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Patterns", meta=(ClampMin="0")) float HopDistance = 600.f;
	/** In the air, looping, until Land plays as the boss touches down. Without it the boss only walks between patterns. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Patterns") TObjectPtr<UAnimMontage> HopMontage;

	/** Share of health at which phase 2 begins: the boss roars (the Roar section of the wake montage) before its next pattern. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Phase", meta=(ClampMin="0", ClampMax="1")) float PhaseTwoHealth = 0.6f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Phase") float RoarSeconds = 1.8f;
	/** Phase 2 plays its patterns, their marks and their montages this much faster. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Phase", meta=(ClampMin="1", ClampMax="2")) float PhaseTwoTempo = 1.2f;
	/** Projectiles, or spots, that every strike of several adds in phase 2. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Phase", meta=(ClampMin="0")) int32 PhaseTwoExtraCount = 2;

	/** Shares of health, highest first, at which the worn-out boss sinks down to catch its breath: the only times it can be captured.
	 * Each one comes once, the moment the boss's health falls to it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exhaust") TArray<float> ExhaustHealth = {0.3f, 0.1f};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exhaust") float ExhaustSeconds = 8.f;
	/** Played over ExhaustSeconds from its start. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exhaust") TObjectPtr<UAnimMontage> ExhaustMontage;

	/** Damage, as a share of its health, that knocks the boss down for a while of free hits. Damage it takes while it is down or worn
	 * out counts for nothing, so one opening never leads straight into the next. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Down", meta=(ClampMin="0.01", ClampMax="1")) float DownAfterDamage = 0.2f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Down") float DownSeconds = 6.f;
	/** Fall, then Down until the last GetUpSeconds, then GetUp. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Down") TObjectPtr<UAnimMontage> DownMontage;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Down") float GetUpSeconds = 1.7f;

	/** Every problem with the patterns and montages; empty when the asset is sound. The save checks the same. */
	UFUNCTION(BlueprintCallable, Category="Boss")
	TArray<FString> Validate() const;
	const FBossPattern* FindPattern(const FGameplayTag& PatternId) const;
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
