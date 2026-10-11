#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MonsterDexStage.generated.h"

class ACharacter;
class UPointLightComponent;
class USceneCaptureComponent2D;
class USkeletalMeshComponent;
class UTextureRenderTarget2D;

/**
 * The monster book's photo booth: a lone skeletal mesh, a key light and a scene capture that sees only that mesh, far from the
 * play area. The book's screen spawns one on the local machine only, shows the picked species on it and draws the render target;
 * the background stays transparent, so the screen can paint an undiscovered species as a silhouette.
 *
 * The species is shown by copying the mesh, its animation blueprint and its mesh offset from the species' character class
 * defaults; no monster actor is made, so no AI, replication or gameplay runs for it.
 */
UCLASS(NotPlaceable)
class SONHEIM_API AMonsterDexStage : public AActor
{
	GENERATED_BODY()
public:
	AMonsterDexStage();
	virtual void Tick(float DeltaSeconds) override;

	/** Shows the species of this character class, framed to fill the picture; no class empties the stage. */
	void Show(TSubclassOf<ACharacter> CharacterClass);
	/** Turns the shown species by this many degrees, as dragging the picture does. */
	void Turn(float Degrees) { Yaw += Degrees; IdleSeconds = 0.f; }
	UTextureRenderTarget2D* GetTarget() const { return Target; }

	/** Stops the live picture and takes one still of each class instead, into its own render target of this size, a few frames
	 * apart so each pose settles. The targets exist at once and fill as the stills are taken; a null class leaves its blank. */
	void Snap(const TArray<TSubclassOf<ACharacter>>& Classes, int32 Size);
	UTextureRenderTarget2D* GetSnap(int32 Index) const { return Snaps.IsValidIndex(Index) ? Snaps[Index].Get() : nullptr; }

	/** Where stages stand: high above any play area, so nothing of the world is near them. */
	static const FVector Spot;

private:
	void Frame();

	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Pivot;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USkeletalMeshComponent> Mesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneCaptureComponent2D> Capture;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> KeyLight;
	UPROPERTY(Transient) TObjectPtr<UTextureRenderTarget2D> Target;

	UPROPERTY(EditAnywhere, Category="Stage") int32 Resolution = 640;
	UPROPERTY(EditAnywhere, Category="Stage") float SpinDegreesPerSecond = 18.f;
	/** After a drag the species holds still this long before it spins again. */
	UPROPERTY(EditAnywhere, Category="Stage") float IdleAfterDragSeconds = 2.f;
	UPROPERTY(EditAnywhere, Category="Stage") float FieldOfView = 30.f;
	/** How much room the species leaves around it: 1 touches the picture's edges. */
	UPROPERTY(EditAnywhere, Category="Stage") float Margin = 1.08f;
	/** The camera looks down on the species by this many degrees. */
	UPROPERTY(EditAnywhere, Category="Stage") float CameraPitch = -12.f;

	/** The turn of a still: a three-quarter view reads better in a small cell than a face-on one. */
	UPROPERTY(EditAnywhere, Category="Stage") float SnapYaw = -30.f;
	/** Frames a species stands before its still is taken, so its animation has posed it. */
	UPROPERTY(EditAnywhere, Category="Stage") int32 SnapSettleFrames = 3;

	UPROPERTY(Transient) TArray<TObjectPtr<UTextureRenderTarget2D>> Snaps;
	UPROPERTY(Transient) TArray<TSubclassOf<ACharacter>> SnapQueue;
	int32 SnapNext = 0;
	int32 SnapWait = 0;

	float Yaw = 0.f;
	float IdleSeconds = 0.f;
};
