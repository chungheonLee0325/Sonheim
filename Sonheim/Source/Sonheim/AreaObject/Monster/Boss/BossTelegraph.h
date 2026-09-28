#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BossPatternDataAsset.h"
#include "BossTelegraph.generated.h"

class UMaterialInstanceDynamic;
class UStaticMeshComponent;

/** What a mark on the ground draws: the strike's shape and the seconds until it lands. */
USTRUCT(BlueprintType)
struct FBossMark
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category="Boss") EBossAreaShape Shape = EBossAreaShape::Circle;
	UPROPERTY(BlueprintReadOnly, Category="Boss") float Radius = 0.f;
	UPROPERTY(BlueprintReadOnly, Category="Boss") float InnerRadius = 0.f;
	UPROPERTY(BlueprintReadOnly, Category="Boss") float HalfAngle = 0.f;
	UPROPERTY(BlueprintReadOnly, Category="Boss") float HalfWidth = 0.f;
	UPROPERTY(BlueprintReadOnly, Category="Boss") float Seconds = 1.f;
	/** A mark on the boss follows it, FollowForward ahead of its feet; a mark on the ground stays where it was placed. */
	UPROPERTY(BlueprintReadOnly, Category="Boss") TObjectPtr<AActor> Follow;
	UPROPERTY(BlueprintReadOnly, Category="Boss") float FollowForward = 0.f;
};

/** A strike's mark on the ground, drawn on every machine. It fills up until the strike lands and goes once it has. */
UCLASS()
class SONHEIM_API ABossTelegraph : public AActor
{
	GENERATED_BODY()
public:
	ABossTelegraph();
	/** Set on the server before the actor finishes spawning. */
	UPROPERTY(ReplicatedUsing=OnRep_Mark, BlueprintReadOnly, Category="Boss") FBossMark Mark;
	/** Draws the shape on a flat plane from the parameters Shape (0 circle, 1 ring, 2 cone, 3 line), Inner, HalfAngle (radians) and Fill. */
	UPROPERTY(EditDefaultsOnly, Category="Boss") TObjectPtr<UMaterialInterface> Material;
	/** The ground under a character and its facing, Forward ahead of its feet. A mark on the boss lies there, and so does its strike. */
	static FTransform FeetTransform(const AActor* Actor, float Forward);
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	UFUNCTION() void OnRep_Mark();
	void Build();
	UPROPERTY(VisibleAnywhere, Category="Boss") TObjectPtr<UStaticMeshComponent> Plane;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> Dynamic;
	float Age = 0.f;
};
