#include "BossTelegraph.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// Above the floor, so the mark never flickers into it.
	constexpr float Lift = 4.f;

	float ShapeParameter(const EBossAreaShape Shape)
	{
		switch (Shape)
		{
		case EBossAreaShape::Ring: return 1.f;
		case EBossAreaShape::Cone: return 2.f;
		case EBossAreaShape::Line: return 3.f;
		default: return 0.f;
		}
	}
}

ABossTelegraph::ABossTelegraph()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	// A ground mark never moves and every machine moves a mark on the boss itself, so movement is not replicated.
	SetReplicatingMovement(false);
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Plane = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Plane"));
	Plane->SetupAttachment(RootComponent);
	Plane->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Plane->SetGenerateOverlapEvents(false);
	Plane->SetCanEverAffectNavigation(false);
	Plane->SetCastShadow(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (PlaneMesh.Succeeded()) Plane->SetStaticMesh(PlaneMesh.Object);
	Material = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/CuratedDungeon/Boss/M_BossTelegraph.M_BossTelegraph")));
}

FTransform ABossTelegraph::FeetTransform(const AActor* Actor, const float Forward)
{
	if (!Actor) return FTransform::Identity;
	const FRotator Facing(0.f, Actor->GetActorRotation().Yaw, 0.f);
	FVector Feet = Actor->GetActorLocation();
	if (const ACharacter* Character = Cast<ACharacter>(Actor))
		Feet.Z -= Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	return FTransform(Facing, Feet + Facing.Vector() * Forward);
}

void ABossTelegraph::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(ABossTelegraph, Mark, COND_InitialOnly);
}

void ABossTelegraph::BeginPlay()
{
	Super::BeginPlay();
	Build();
}

void ABossTelegraph::OnRep_Mark()
{
	Build();
}

void ABossTelegraph::Build()
{
	if (Mark.Radius <= 0.f) return;
	if (!Dynamic)
		if (UMaterialInterface* Loaded = Material.LoadSynchronous()) Dynamic = Plane->CreateDynamicMaterialInstance(0, Loaded);
	// The plane is 100 units square: circles, rings and cones sit on the anchor, a line starts at it.
	const bool bLine = Mark.Shape == EBossAreaShape::Line;
	Plane->SetRelativeLocation(FVector(bLine ? Mark.Radius * 0.5f : 0.f, 0.f, Lift));
	Plane->SetRelativeScale3D(bLine ? FVector(Mark.Radius / 100.f, Mark.HalfWidth / 50.f, 1.f) : FVector(Mark.Radius / 50.f, Mark.Radius / 50.f, 1.f));
	if (Dynamic)
	{
		Dynamic->SetScalarParameterValue(TEXT("Shape"), ShapeParameter(Mark.Shape));
		Dynamic->SetScalarParameterValue(TEXT("Inner"), Mark.Shape == EBossAreaShape::Ring ? Mark.InnerRadius / Mark.Radius : 0.f);
		Dynamic->SetScalarParameterValue(TEXT("HalfAngle"), FMath::DegreesToRadians(Mark.HalfAngle));
		Dynamic->SetScalarParameterValue(TEXT("Fill"), 0.f);
	}
	if (Mark.Follow) SetActorTransform(FeetTransform(Mark.Follow, Mark.FollowForward));
}

void ABossTelegraph::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Age += DeltaSeconds;
	if (Mark.Follow) SetActorTransform(FeetTransform(Mark.Follow, Mark.FollowForward));
	if (Dynamic) Dynamic->SetScalarParameterValue(TEXT("Fill"), Mark.Seconds > 0.f ? FMath::Clamp(Age / Mark.Seconds, 0.f, 1.f) : 1.f);
}
