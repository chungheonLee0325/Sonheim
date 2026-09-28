#include "BossPatternDataAsset.h"

#include "Animation/AnimMontage.h"
#include "Sonheim/Element/BaseElement.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

const FName UBossPatternDataAsset::SleepSection(TEXT("Sleep"));
const FName UBossPatternDataAsset::WakeSection(TEXT("Wake"));
const FName UBossPatternDataAsset::RoarSection(TEXT("Roar"));
const FName UBossPatternDataAsset::FallSection(TEXT("Fall"));
const FName UBossPatternDataAsset::DownSection(TEXT("Down"));
const FName UBossPatternDataAsset::GetUpSection(TEXT("GetUp"));

namespace
{
	bool HasSection(const UAnimMontage* Montage, const FName Section)
	{
		return Montage && Montage->GetSectionIndex(Section) != INDEX_NONE;
	}

	void ValidateStrike(const FBossStrike& Strike, const float PatternSeconds, const FString& At, TArray<FString>& Problems)
	{
		if (Strike.MarkSeconds < 0.f || Strike.MarkSeconds > Strike.StrikeSeconds || Strike.StrikeSeconds > PatternSeconds)
			Problems.Add(At + TEXT(": the mark must come before the strike, and the strike within the pattern"));
		const bool bProjectiles = Strike.ProjectileCount > 0;
		if (bProjectiles != (Strike.Projectile != nullptr))
			Problems.Add(At + TEXT(": a projectile strike needs both a projectile and a count"));
		if (Strike.Shape == EBossAreaShape::None)
		{
			if (!bProjectiles) Problems.Add(At + TEXT(": a strike without a shape does nothing unless it fires projectiles"));
			else if (Strike.Aim == EBossProjectileAim::Location) Problems.Add(At + TEXT(": projectiles thrown at spots mark every spot, so the strike needs a shape"));
			return;
		}
		if (Strike.Radius <= 0.f) Problems.Add(At + TEXT(": the area has no size"));
		if (Strike.StrikeSeconds - Strike.MarkSeconds < UBossPatternDataAsset::MinWarningSeconds)
			Problems.Add(FString::Printf(TEXT("%s: the mark shows for less than %.1f s, too short to step out"), *At, UBossPatternDataAsset::MinWarningSeconds));
		if (Strike.Shape == EBossAreaShape::Ring && (Strike.InnerRadius <= 0.f || Strike.InnerRadius >= Strike.Radius))
			Problems.Add(At + TEXT(": the ring's inner radius must lie between 0 and its radius"));
		if (Strike.Shape == EBossAreaShape::Cone && (Strike.HalfAngle <= 0.f || Strike.HalfAngle > 180.f))
			Problems.Add(At + TEXT(": the cone's half angle must lie between 0 and 180 degrees"));
		if (Strike.Shape == EBossAreaShape::Line && Strike.HalfWidth <= 0.f)
			Problems.Add(At + TEXT(": the line has no width"));
	}
}

TArray<FString> UBossPatternDataAsset::Validate() const
{
	TArray<FString> Problems;
	if (Patterns.IsEmpty()) Problems.Add(TEXT("no patterns"));
	TSet<FGameplayTag> Seen;
	for (int32 Index = 0; Index < Patterns.Num(); ++Index)
	{
		const FBossPattern& Pattern = Patterns[Index];
		const FString Name = Pattern.PatternId.IsValid() ? Pattern.PatternId.ToString() : FString::Printf(TEXT("Patterns[%d]"), Index);
		bool bRepeated = false;
		Seen.Add(Pattern.PatternId, &bRepeated);
		if (!Pattern.PatternId.IsValid() || bRepeated) Problems.Add(Name + TEXT(": every pattern needs an id of its own"));
		if (!Pattern.Montage) Problems.Add(Name + TEXT(": no montage"));
		if (Pattern.MinRange >= Pattern.MaxRange) Problems.Add(Name + TEXT(": MinRange is not below MaxRange"));
		if (Pattern.Strikes.IsEmpty()) Problems.Add(Name + TEXT(": no strikes"));
		for (const FBossSectionCue& Cue : Pattern.Cues)
		{
			if (!HasSection(Pattern.Montage, Cue.Section))
				Problems.Add(FString::Printf(TEXT("%s: the montage has no section %s"), *Name, *Cue.Section.ToString()));
			if (Cue.Seconds < 0.f || Cue.Seconds > Pattern.Seconds)
				Problems.Add(FString::Printf(TEXT("%s: the cue at %.2f s lies outside the pattern"), *Name, Cue.Seconds));
		}
		if (Pattern.LeapEndSeconds > 0.f && (Pattern.LeapStartSeconds < 0.f || Pattern.LeapStartSeconds >= Pattern.LeapEndSeconds || Pattern.LeapEndSeconds > Pattern.Seconds))
			Problems.Add(Name + TEXT(": the leap does not fit in the pattern"));
		for (int32 StrikeIndex = 0; StrikeIndex < Pattern.Strikes.Num(); ++StrikeIndex)
			ValidateStrike(Pattern.Strikes[StrikeIndex], Pattern.Seconds, FString::Printf(TEXT("%s strike %d"), *Name, StrikeIndex), Problems);
	}
	if (!HasSection(WakeMontage, SleepSection) || !HasSection(WakeMontage, WakeSection) || !HasSection(WakeMontage, RoarSection))
		Problems.Add(TEXT("the wake montage needs the sections Sleep, Wake and Roar"));
	if (!RestMontage) Problems.Add(TEXT("no rest montage"));
	if (!HasSection(DownMontage, FallSection) || !HasSection(DownMontage, DownSection) || !HasSection(DownMontage, GetUpSection))
		Problems.Add(TEXT("the down montage needs the sections Fall, Down and GetUp"));
	if (GetUpSeconds >= DownSeconds) Problems.Add(TEXT("GetUpSeconds must be shorter than DownSeconds"));
	return Problems;
}

const FBossPattern* UBossPatternDataAsset::FindPattern(const FGameplayTag& PatternId) const
{
	return Patterns.FindByPredicate([&PatternId](const FBossPattern& Pattern) { return Pattern.PatternId == PatternId; });
}

#if WITH_EDITOR
EDataValidationResult UBossPatternDataAsset::IsDataValid(FDataValidationContext& Context) const
{
	Super::IsDataValid(Context);
	const TArray<FString> Problems = Validate();
	for (const FString& Problem : Problems) Context.AddError(FText::FromString(Problem));
	return Problems.IsEmpty() ? EDataValidationResult::Valid : EDataValidationResult::Invalid;
}
#endif
