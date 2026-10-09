#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectileTrajectoryPreviewActor.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class USceneComponent;

// 궤적 점선 모양. 기본값은 팰스피어 조준용으로 맞춘 값이다.
USTRUCT(BlueprintType)
struct FProjectileTrajectoryPreviewConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.1"))
	float MaxSimTime = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1.0"))
	float SimFrequency = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0"))
	float CollisionRadius = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1.0"))
	float DashSpacing = 48.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1.0"))
	float DashLength = 16.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1.0"))
	float DashThickness = 4.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1", ClampMax="256"))
	int32 MaxDashCount = 96;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1", ClampMax="64"))
	int32 MinDashCount = 10;

	// 끝점 앞에서 점선을 비우는 거리
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0"))
	float EndGapDistance = 45.0f;

	// 시작점이 몸 안이라 앞부분은 비운다
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0"))
	float StartGapDistance = 40.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor DashColor = FLinearColor(0.04f, 0.47f, 1.0f, 0.92f);

	// 머티리얼이 2배를 곱하므로 0.5면 DashColor 그대로. 1을 넘는 성분은 화면에서 흰색으로 날아간다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0"))
	float EmissiveStrength = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UMaterialInterface> DashMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(
		TEXT("/Game/_Resource/FX/Projectile/Trajectory/M_ProjectileTrajectoryPreview.M_ProjectileTrajectoryPreview")));
};

// 로컬 전용 궤적 점선. 시작점과 초기 속도로 포물선을 예측해 점을 찍는다.
UCLASS()
class SONHEIM_API AProjectileTrajectoryPreviewActor : public AActor
{
	GENERATED_BODY()

public:
	AProjectileTrajectoryPreviewActor();

	void UpdatePreview(
		const FVector& LaunchStart,
		const FVector& LaunchVelocity,
		AActor* IgnoredActor,
		const FProjectileTrajectoryPreviewConfig& InConfig);

private:
	void ApplyDashMaterial();
	void AddDashInstance(const FVector& Location, const FVector& Tangent);
	void TrimUnusedInstances(int32 UsedInstanceCount);

	UPROPERTY(VisibleAnywhere, Category="Trajectory Preview")
	TObjectPtr<USceneComponent> SceneRoot = nullptr;

	UPROPERTY(VisibleAnywhere, Category="Trajectory Preview")
	TObjectPtr<UInstancedStaticMeshComponent> DashInstances = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicDashMaterial = nullptr;

	FProjectileTrajectoryPreviewConfig Config;
	FVector LastLaunchStart = FVector::ZeroVector;
	FVector LastLaunchVelocity = FVector::ZeroVector;
	bool bHasRendered = false;
	int32 DashBuildInstanceIndex = 0;
};
