#include "ProjectileTrajectoryPreviewActor.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AProjectileTrajectoryPreviewActor::AProjectileTrajectoryPreviewActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	DashInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("TrajectoryDashInstances"));
	DashInstances->SetupAttachment(SceneRoot);
	DashInstances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DashInstances->SetGenerateOverlapEvents(false);
	DashInstances->SetCastShadow(false);
	DashInstances->bReceivesDecals = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMeshFinder.Succeeded())
	{
		DashInstances->SetStaticMesh(SphereMeshFinder.Object);
	}
}

void AProjectileTrajectoryPreviewActor::UpdatePreview(
	const FVector& LaunchStart,
	const FVector& LaunchVelocity,
	AActor* IgnoredActor,
	const FProjectileTrajectoryPreviewConfig& InConfig)
{
	// 같은 궤적이면 다시 그리지 않는다
	constexpr float RebuildTolerance = 0.5f;
	if (bHasRendered
		&& LastLaunchStart.Equals(LaunchStart, RebuildTolerance)
		&& LastLaunchVelocity.Equals(LaunchVelocity, RebuildTolerance))
	{
		return;
	}

	const bool bMaterialChanged = !bHasRendered || Config.DashMaterial != InConfig.DashMaterial;
	Config = InConfig;
	LastLaunchStart = LaunchStart;
	LastLaunchVelocity = LaunchVelocity;
	bHasRendered = true;
	if (bMaterialChanged)
	{
		ApplyDashMaterial();
	}

	FPredictProjectilePathParams Params(Config.CollisionRadius, LaunchStart, LaunchVelocity, Config.MaxSimTime);
	Params.bTraceWithCollision = true;
	Params.bTraceComplex = false;
	Params.TraceChannel = Config.TraceChannel;
	Params.SimFrequency = Config.SimFrequency;
	Params.OverrideGravityZ = GetWorld()->GetGravityZ();
	Params.ActorsToIgnore.Add(this);
	if (IgnoredActor)
	{
		Params.ActorsToIgnore.Add(IgnoredActor);
	}

	FPredictProjectilePathResult Result;
	UGameplayStatics::PredictProjectilePath(this, Params, Result);

	DashBuildInstanceIndex = 0;
	const TArray<FPredictProjectilePathPointData>& Path = Result.PathData;

	float TotalPathLength = 0.0f;
	for (int32 Index = 1; Index < Path.Num(); ++Index)
	{
		TotalPathLength += FVector::Distance(Path[Index - 1].Location, Path[Index].Location);
	}

	// 짧은 궤적에서도 최소 개수는 보이도록 간격을 줄인다
	const float ConfiguredSpacing = FMath::Max(1.0f, Config.DashSpacing);
	const float MinimumSpacing = FMath::Min(ConfiguredSpacing, FMath::Max(1.0f, Config.DashLength * 1.45f));
	const float DashSpacing = FMath::Clamp(
		TotalPathLength / FMath::Clamp(Config.MinDashCount, 1, 64),
		MinimumSpacing,
		ConfiguredSpacing);
	const float MaxDashDistance = TotalPathLength - FMath::Min(Config.EndGapDistance, TotalPathLength * 0.35f);
	const int32 MaxDashCount = FMath::Clamp(Config.MaxDashCount, 1, 256);

	float AccumulatedDistance = 0.0f;
	float NextDashDistance = Config.StartGapDistance;
	int32 DashCount = 0;
	for (int32 Index = 1; Index < Path.Num() && DashCount < MaxDashCount; ++Index)
	{
		const FVector SegmentStart = Path[Index - 1].Location;
		const FVector Segment = Path[Index].Location - SegmentStart;
		const float SegmentLength = Segment.Length();
		if (SegmentLength <= KINDA_SMALL_NUMBER)
		{
			continue;
		}

		while (AccumulatedDistance + SegmentLength >= NextDashDistance
			&& NextDashDistance <= MaxDashDistance
			&& DashCount < MaxDashCount)
		{
			const float Alpha = (NextDashDistance - AccumulatedDistance) / SegmentLength;
			AddDashInstance(SegmentStart + Segment * Alpha, Segment / SegmentLength);
			++DashCount;
			NextDashDistance += DashSpacing;
		}
		AccumulatedDistance += SegmentLength;
	}

	TrimUnusedInstances(DashBuildInstanceIndex);
	DashInstances->SetVisibility(DashCount > 0);
}

void AProjectileTrajectoryPreviewActor::ApplyDashMaterial()
{
	UMaterialInterface* DashMaterial = Config.DashMaterial.LoadSynchronous();
	DynamicDashMaterial = DashMaterial ? UMaterialInstanceDynamic::Create(DashMaterial, this) : nullptr;
	if (!DynamicDashMaterial)
	{
		return;
	}

	DynamicDashMaterial->SetVectorParameterValue(TEXT("PreviewColor"), Config.DashColor);
	DynamicDashMaterial->SetVectorParameterValue(TEXT("Color"), Config.DashColor);
	DynamicDashMaterial->SetVectorParameterValue(TEXT("BaseColor"), Config.DashColor);
	DynamicDashMaterial->SetVectorParameterValue(TEXT("EmissiveColor"), Config.DashColor * 2.0f);
	DynamicDashMaterial->SetScalarParameterValue(TEXT("Opacity"), Config.DashColor.A);
	DynamicDashMaterial->SetScalarParameterValue(TEXT("EmissiveStrength"), 1.15f);
	DashInstances->SetMaterial(0, DynamicDashMaterial);
}

void AProjectileTrajectoryPreviewActor::AddDashInstance(const FVector& Location, const FVector& Tangent)
{
	FVector MeshSize(100.0f);
	if (const UStaticMesh* StaticMesh = DashInstances->GetStaticMesh())
	{
		MeshSize = (StaticMesh->GetBounds().BoxExtent * 2.0f).ComponentMax(FVector(1.0f));
	}

	const FVector DashScale(
		FMath::Max(1.0f, Config.DashLength) / MeshSize.X,
		FMath::Max(1.0f, Config.DashThickness) / MeshSize.Y,
		FMath::Max(1.0f, Config.DashThickness) / MeshSize.Z);
	const FTransform DashTransform(Tangent.Rotation(), Location, DashScale);

	// 인스턴스를 지우고 다시 만들지 않고 재사용한다
	if (DashBuildInstanceIndex < DashInstances->GetInstanceCount())
	{
		DashInstances->UpdateInstanceTransform(DashBuildInstanceIndex, DashTransform, true, false, true);
	}
	else
	{
		DashInstances->AddInstance(DashTransform, true);
	}
	++DashBuildInstanceIndex;
}

void AProjectileTrajectoryPreviewActor::TrimUnusedInstances(int32 UsedInstanceCount)
{
	for (int32 Index = DashInstances->GetInstanceCount() - 1; Index >= UsedInstanceCount; --Index)
	{
		DashInstances->RemoveInstance(Index);
	}
	DashInstances->MarkRenderStateDirty();
}
