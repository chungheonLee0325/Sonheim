// Fill out your copyright notice in the Description page of Project Settings.


#include "SandBlast.h"

#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Sonheim/Utilities/LogMacro.h"


// Sets default values
ASandBlast::ASandBlast()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	Root = CreateDefaultSubobject<USphereComponent>(TEXT("Root"));
	RootComponent = Root;
	Root->SetSphereRadius(50.f);
	Root->SetSimulatePhysics(true);
	Root->SetCollisionProfileName(TEXT("MonsterProjectile"));
	
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> BaseMeshObject
		(TEXT("/Script/Engine.StaticMesh'/Game/_BluePrint/Element/SandBlast/Model/SM_SandBlast.SM_SandBlast'"));
	if (BaseMeshObject.Succeeded()) {
		Mesh->SetStaticMesh(BaseMeshObject.Object);
	}

	SkillProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("SkillProjectileMovement"));
	SkillProjectileMovement->InitialSpeed = 500.f;
	SkillProjectileMovement->MaxSpeed = 500.f;
	SkillProjectileMovement->ProjectileGravityScale = 0.f;
}

// Called when the game starts or when spawned
void ASandBlast::BeginPlay()
{
	Super::BeginPlay();

	FLog::Log("ASandBlast::BeginPlay");
}

// Called every frame
void ASandBlast::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

