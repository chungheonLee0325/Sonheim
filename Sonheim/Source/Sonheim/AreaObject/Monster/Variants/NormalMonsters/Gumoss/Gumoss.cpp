// Fill out your copyright notice in the Description page of Project Settings.


#include "Gumoss.h"

#include "Components/CapsuleComponent.h"
#include "Sonheim/AreaObject/Monster/AI/Derived/AiMonster/Gumoss/GumossFSM.h"


// Sets default values
AGumoss::AGumoss()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	m_AreaObjectID = 100;

	// FSM Setting
	m_AiFSM = AGumoss::CreateFSM();
	// Skill Setting
	m_SkillRoulette = ABaseMonster::CreateSkillRoulette();
	
	ConstructorHelpers::FObjectFinder<USkeletalMesh> TempMesh
	(TEXT("/Script/Engine.SkeletalMesh'/Game/_Resource/Monster/Gumoss/SK_PlantSlime_LOD0.SK_PlantSlime_LOD0'"));
	if (TempMesh.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(TempMesh.Object);
		GetMesh()->SetRelativeLocationAndRotation(FVector(0, 0, -70), FRotator(0, -90, 0));
		GetMesh()->SetRelativeScale3D(FVector(0.7f));
	}
	
	GetCapsuleComponent()->SetCapsuleRadius(70.f);
	GetCapsuleComponent()->SetCapsuleHalfHeight(70.f);
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("Monster"));
	
	//PickaxeMesh->SetupAttachment(GetMesh(), "Pickaxe");

}

// Called when the game starts or when spawned
void AGumoss::BeginPlay()
{
	Super::BeginPlay();

	// Eye Material
	EyeMat = GetMesh()->CreateDynamicMaterialInstance(0);
}

// Called every frame
void AGumoss::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void AGumoss::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

UBaseAiFSM* AGumoss::CreateFSM()
{
	return CreateDefaultSubobject<UGumossFSM>(TEXT("FSM"));
}