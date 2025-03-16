// Fill out your copyright notice in the Description page of Project Settings.


#include "LamBall.h"

#include "Components/CapsuleComponent.h"
#include "Sonheim/AreaObject/Monster/AI/Derived/AiMonster/Lamball/LamballFSM.h"


// Sets default values
ALamBall::ALamBall()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	m_AreaObjectID = 103;

 	m_AiFSM = ALamBall::CreateFSM();
	m_SkillRoulette = ABaseMonster::CreateSkillRoulette();
	
	ConstructorHelpers::FObjectFinder<USkeletalMesh> TempMesh
		(TEXT("/Script/Engine.SkeletalMesh'/Game/_Resource/Monster/Lamball/SK_SheepBall_LOD0.SK_SheepBall_LOD0'"));
	if (TempMesh.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(TempMesh.Object);
		GetMesh()->SetRelativeLocationAndRotation(FVector(0, 0, -90), FRotator(0, -90, 0));
		GetMesh()->SetRelativeScale3D(FVector(0.7f));
	}

	PickaxeMesh->SetupAttachment(GetMesh(), "Pickaxe");
	
	GetCapsuleComponent()->SetCapsuleRadius(90.f);
	GetCapsuleComponent()->SetCapsuleHalfHeight(90.f);
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("Monster"));
}

// Called when the game starts or when spawned
void ALamBall::BeginPlay()
{
	Super::BeginPlay();

}

// Called every frame
void ALamBall::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void ALamBall::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

UBaseAiFSM* ALamBall::CreateFSM()
{
	return CreateDefaultSubobject<ULamballFSM>(TEXT("FSM2"));
}
