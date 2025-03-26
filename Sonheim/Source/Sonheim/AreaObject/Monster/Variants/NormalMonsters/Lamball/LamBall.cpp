// Fill out your copyright notice in the Description page of Project Settings.


#include "LamBall.h"

#include "Components/CapsuleComponent.h"
#include "Net/UnrealNetwork.h"
#include "Sonheim/AreaObject/Monster/AI/Derived/AiMonster/PalFSM/LambBallFSM.h"


// Sets default values
ALamBall::ALamBall()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	m_AreaObjectID = 103;

 	m_AiFSM = ALamBall::CreateFSM();
	m_SkillRoulette = ABaseMonster::CreateSkillRoulette();
	
	ConstructorHelpers::FObjectFinder<USkeletalMesh> TempMesh
		(TEXT("/Script/Engine.SkeletalMesh'/Game/_Resource/Monster/LambBall/SK_SheepBall_LOD0.SK_SheepBall_LOD0'"));
	if (TempMesh.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(TempMesh.Object);
		GetMesh()->SetRelativeLocationAndRotation(FVector(0, 0, -65), FRotator(0, -90, 0));
		GetMesh()->SetRelativeScale3D(FVector(0.5f));
	}
	
	PickaxeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickaxeMesh"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PickaxeMeshObject
		(TEXT("/Script/Engine.StaticMesh'/Game/SurvivalGameKitV2/Meshes/Static/SM_Pickaxe_01.SM_Pickaxe_01'"));
	if (PickaxeMeshObject.Succeeded())
	{
		PickaxeMesh->SetStaticMesh(PickaxeMeshObject.Object);
		PickaxeMesh->SetupAttachment(GetMesh(), "Pickaxe");
	}
	
	GetCapsuleComponent()->SetCapsuleRadius(65.f);
	GetCapsuleComponent()->SetCapsuleHalfHeight(65.f);
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("Monster"));
}

// Called when the game starts or when spawned
void ALamBall::BeginPlay()
{
	Super::BeginPlay();

	EyeMat = GetMesh()->CreateDynamicMaterialInstance(0);
	MouthMat = GetMesh()->CreateDynamicMaterialInstance(1);

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
	//return CreateDefaultSubobject<UTempLamballFSM>(TEXT("FSM2"));
	return CreateDefaultSubobject<ULambBallFSM>(TEXT("FSM"));
}

void ALamBall::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ALamBall, isDizzy);
}
