#include "MonsterDexStage.h"
#include "Animation/AnimInstance.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GameFramework/Character.h"

const FVector AMonsterDexStage::Spot(0.f, 0.f, 200000.f);

AMonsterDexStage::AMonsterDexStage()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;
	SetCanBeDamaged(false);

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Pivot = CreateDefaultSubobject<USceneComponent>(TEXT("Pivot"));
	Pivot->SetupAttachment(RootComponent);
	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Pivot);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// Only the capture sees the mesh: a player never meets it in the world.
	Mesh->SetVisibleInSceneCaptureOnly(true);
	Mesh->SetCastShadow(false);
	Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	KeyLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("KeyLight"));
	KeyLight->SetupAttachment(RootComponent);
	KeyLight->SetIntensityUnits(ELightUnits::Candelas);
	KeyLight->SetIntensity(120.f);
	KeyLight->SetAttenuationRadius(5000.f);
	KeyLight->SetCastShadows(false);

	Capture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("Capture"));
	Capture->SetupAttachment(RootComponent);
	Capture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
	// Scene color before post processing keeps the background's alpha, which the book's material turns into transparency.
	Capture->CaptureSource = ESceneCaptureSource::SCS_SceneColorHDR;
	Capture->bCaptureEveryFrame = true;
	Capture->bCaptureOnMovement = false;
	Capture->FOVAngle = FieldOfView;
	for (const TCHAR* Flag : {TEXT("Atmosphere"), TEXT("Fog"), TEXT("VolumetricFog"), TEXT("Cloud"), TEXT("Particles")})
	{
		Capture->ShowFlagSettings.Add(FEngineShowFlagsSetting{Flag, false});
	}
}

void AMonsterDexStage::Show(const TSubclassOf<ACharacter> CharacterClass)
{
	if (!Target)
	{
		Target = NewObject<UTextureRenderTarget2D>(this);
		Target->RenderTargetFormat = RTF_RGBA16f;
		Target->ClearColor = FLinearColor::Transparent;
		Target->InitAutoFormat(Resolution, Resolution);
		Capture->TextureTarget = Target;
		Capture->ShowOnlyComponents.Add(Mesh);
		Capture->FOVAngle = FieldOfView;
	}
	const ACharacter* Defaults = CharacterClass ? CharacterClass->GetDefaultObject<ACharacter>() : nullptr;
	const USkeletalMeshComponent* Source = Defaults ? Defaults->GetMesh() : nullptr;
	Mesh->SetSkeletalMesh(Source ? Source->GetSkeletalMeshAsset() : nullptr);
	if (!Source) return;
	Mesh->SetRelativeScale3D(Source->GetRelativeScale3D());
	// A character's mesh faces along its capsule's -Y; the stage keeps that turn so species face the camera alike.
	Mesh->SetRelativeRotation(Source->GetRelativeRotation());
	Mesh->SetAnimInstanceClass(Source->AnimClass);
	for (int32 Slot = 0; Slot < Source->GetNumOverrideMaterials(); ++Slot)
	{
		if (UMaterialInterface* Material = Source->OverrideMaterials[Slot]) Mesh->SetMaterial(Slot, Material);
	}
	Yaw = 0.f;
	IdleSeconds = IdleAfterDragSeconds;
	Frame();
}

void AMonsterDexStage::Frame()
{
	// Centred on the mesh's bounds, the camera backs off until the box fits the picture as it turns: its height, or the widest
	// it gets across, whichever is larger.
	Mesh->SetRelativeLocation(FVector::ZeroVector);
	Pivot->SetRelativeRotation(FRotator::ZeroRotator);
	// The mesh asset's own bounds, not the component's: a skeletal mesh component's bounds follow its physics bodies, which reach
	// well past what is drawn.
	const USkeletalMesh* Asset = Mesh->GetSkeletalMeshAsset();
	if (!Asset) return;
	const FBox Box = Asset->GetImportedBounds().GetBox().TransformBy(Mesh->GetComponentTransform());
	Mesh->AddWorldOffset(GetActorLocation() - Box.GetCenter());
	const FVector Extent = Box.GetExtent();
	const float Across = FVector2D(Extent.X, Extent.Y).Size();
	const float Half = FMath::Max(10.f, FMath::Max(Extent.Z, Across));
	const float Distance = Half / FMath::Tan(FMath::DegreesToRadians(FieldOfView * 0.5f)) * Margin;
	// A character faces +X, so the camera stands on that side looking back at it, and the light above the camera's left.
	const FRotator Look(CameraPitch, 180.f, 0.f);
	Capture->SetWorldLocationAndRotation(GetActorLocation() - Look.Vector() * Distance, Look);
	KeyLight->SetWorldLocation(GetActorLocation() - FRotator(-35.f, 150.f, 0.f).Vector() * Distance);
}

void AMonsterDexStage::Snap(const TArray<TSubclassOf<ACharacter>>& Classes, const int32 Size)
{
	Capture->bCaptureEveryFrame = false;
	SnapQueue = Classes;
	SnapNext = 0;
	SnapWait = 0;
	Snaps.Reset();
	for (int32 i = 0; i < Classes.Num(); ++i)
	{
		UTextureRenderTarget2D* Still = NewObject<UTextureRenderTarget2D>(this);
		Still->RenderTargetFormat = RTF_RGBA16f;
		Still->ClearColor = FLinearColor::Transparent;
		Still->InitAutoFormat(Size, Size);
		Snaps.Add(Still);
	}
}

void AMonsterDexStage::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (SnapNext < SnapQueue.Num())
	{
		if (SnapWait == 0)
		{
			Show(SnapQueue[SnapNext]);
			Pivot->SetRelativeRotation(FRotator(0.f, SnapYaw, 0.f));
			SnapWait = FMath::Max(1, SnapSettleFrames);
		}
		else if (--SnapWait == 0)
		{
			if (Mesh->GetSkeletalMeshAsset())
			{
				Capture->TextureTarget = Snaps[SnapNext];
				Capture->CaptureScene();
				Capture->TextureTarget = Target;
			}
			++SnapNext;
		}
		return;
	}
	IdleSeconds += DeltaSeconds;
	if (IdleSeconds >= IdleAfterDragSeconds) Yaw += SpinDegreesPerSecond * DeltaSeconds;
	Pivot->SetRelativeRotation(FRotator(0.f, Yaw, 0.f));
}
