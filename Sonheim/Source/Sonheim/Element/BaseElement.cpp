// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseElement.h"

#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sonheim/AreaObject/Base/AreaObject.h"


// Sets default values
ABaseElement::ABaseElement()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USphereComponent>(TEXT("Root"));
	RootComponent = Root;
	Root->SetSphereRadius(50.f);
	Root->SetSimulatePhysics(true);
	Root->SetLinearDamping(0.f);
	Root->SetMassOverrideInKg(NAME_None, 1.f, true);
	Root->SetCollisionProfileName(TEXT("IgnoreActor"));


	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	static ConstructorHelpers::FObjectFinder<UStaticMesh> BaseMeshObject
		(TEXT("/Script/Engine.StaticMesh'/Game/_BluePrint/Element/Base/Model/SM_BaseElement.SM_BaseElement'"));
	if (BaseMeshObject.Succeeded()) {
		Mesh->SetStaticMesh(BaseMeshObject.Object);
	}
	
}

void ABaseElement::InitElement(AAreaObject* Caster, AAreaObject* Target, const FVector& TargetLocation,
	FAttackData* AttackData)
{
	m_Caster = Caster;
	m_Target = Target;
	m_TargetLocation = TargetLocation;
	m_AttackData = AttackData;
	
	// Collision
	Root->SetCollisionProfileName(TEXT("MonsterProjectile"));
}

// Called when the game starts or when spawned
void ABaseElement::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ABaseElement::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

FVector ABaseElement::Fire(AAreaObject* Caster, AAreaObject* Target, FVector TargetLocation, float ArcValue)
{
	// Todo : 가까우면 너무 느림, 속도 최소값 정하긴 해야할듯
	FVector StartLoc{Caster->GetActorLocation()};
	FVector TargetLoc{StartLoc + GetActorForwardVector() * (GetActorLocation() - TargetLocation).Length()};
	FVector OutVelocity{FVector::ZeroVector};
	if (UGameplayStatics::SuggestProjectileVelocity_CustomArc(this, OutVelocity, StartLoc, TargetLoc,
															  GetWorld()->GetGravityZ(), ArcValue))
	{
		if (m_Caster->bShowDebug)
		{
			FPredictProjectilePathParams PredictParams(5.f, StartLoc, OutVelocity, 15.f);
			PredictParams.DrawDebugTime = 2.f;
			PredictParams.DrawDebugType = EDrawDebugTrace::Type::ForDuration;
			PredictParams.OverrideGravityZ = GetWorld()->GetGravityZ();
			FPredictProjectilePathResult Result;
			UGameplayStatics::PredictProjectilePath(this, PredictParams, Result);
		}
	}

	return OutVelocity;
}

void ABaseElement::OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                  UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{}

void ABaseElement::DestroySelf()
{
	Destroy();
}

