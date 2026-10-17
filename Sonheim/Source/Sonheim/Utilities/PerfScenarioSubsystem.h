#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "PerfScenarioSubsystem.generated.h"

/**
 * Development builds: plays fixed UI scenes for measuring, when the game is started with -PerfScenes. Each scene sets the player in
 * one place and view, opens what it measures (the HUD alone, a stream of notices, the inventory, the monster book, the world map
 * being panned and zoomed), lets it settle for -PerfWarmup seconds, records a CSV profile for -PerfSeconds into
 * Saved/Perf/<-PerfLabel>/csv, closes it and goes on; after the last scene the game quits. Scripts/Perf/measure_ui.py drives it.
 *
 *   -PerfScenes=HUD,Notices,Inventory,Dex,Map -PerfWarmup=3 -PerfSeconds=10 -PerfLabel=baseline -PerfRun=1
 */
UCLASS()
class SONHEIM_API UPerfScenarioSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	enum class EPhase : uint8 { WaitingForPlayer, Settling, Recording, Writing, Done };

	bool Tick(float DeltaSeconds);
	/** Puts the player in the scene's place and opens what the scene measures. */
	void SetUp(const FString& Scene);
	/** What a scene does every frame while it runs (the notices it pushes, the map view it moves). */
	void Drive(const FString& Scene, float Time);
	void TearDown();
	class ASonheimPlayerController* Controller() const;

	TArray<FString> Scenes;
	FString Label;
	int32 Run = 1;
	float WarmupSeconds = 3.f;
	float RecordSeconds = 10.f;
	int32 SceneIndex = 0;
	EPhase Phase = EPhase::WaitingForPlayer;
	float PhaseTime = 0.f;
	float NextNotice = 0.f;
	FTSTicker::FDelegateHandle TickHandle;
	/** The capture's file, written on another thread after the frame that ends it. */
	TSharedFuture<FString> Written;
};
