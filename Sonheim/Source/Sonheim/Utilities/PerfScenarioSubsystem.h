#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "PerfScenarioSubsystem.generated.h"

/**
 * Development builds: plays fixed UI scenes for measuring, when the game is started with -PerfScenes. Each scene sets the player in
 * one place and view, opens what it measures (the HUD alone, a stream of notices, the inventory, the monster book, the world map
 * being panned and zoomed; OpenClose opens and closes the inventory, the book and the map in turn; InvSwap fills the inventory
 * and swaps two of its items over and over), lets it settle for -PerfWarmup seconds, records a CSV profile for -PerfSeconds into
 * Saved/Perf/<-PerfLabel>/csv, closes it and goes on; after the last scene the game quits. Scripts/Perf/measure_ui.py drives it.
 *
 *   -PerfScenes=HUD,Notices,Inventory,Dex,Map -PerfWarmup=3 -PerfSeconds=10 -PerfLabel=baseline -PerfRun=1
 * Each open, close and swap is marked as a CSV event (PerfUI/Open Menu, ...), so the frames it costs can be found in the capture.
 *
 *   -PerfShots           also saves a screenshot of each scene as it starts recording, into Saved/Perf/<label>/shots
 *   -PerfProfileGPU=Dex  also writes one frame's GPU passes (ProfileGPU) into the log as that scene starts recording
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
	FString ProfileGpuScene;
	bool bShots = false;
	int32 Run = 1;
	float WarmupSeconds = 3.f;
	float RecordSeconds = 10.f;
	int32 SceneIndex = 0;
	EPhase Phase = EPhase::WaitingForPlayer;
	float PhaseTime = 0.f;
	float NextNotice = 0.f;
	/** The scene's next timed step (a notice, an open or close, a swap) and how many it took. */
	int32 Steps = 0;
	FTSTicker::FDelegateHandle TickHandle;
	/** The capture's file, written on another thread after the frame that ends it. */
	TSharedFuture<FString> Written;
};
