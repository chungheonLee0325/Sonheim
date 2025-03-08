// Fill out your copyright notice in the Description page of Project Settings.


#include "SwordSkeleton.h"

#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Sonheim/AreaObject/Monster/AI/Derived/AiMonster/MonsterFSM.h"


// Sets default values
ASwordSkeleton::ASwordSkeleton()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// AreaObject ID Setting
	m_AreaObjectID = 10;

	// FSM Setting
	m_AiFSM = ASwordSkeleton::CreateFSM();
	// Skill Setting
	m_SkillRoulette = ABaseMonster::CreateSkillRoulette();

	// CapsuleComponent Setting
	GetCapsuleComponent()->SetCapsuleRadius(40.f);
	GetCapsuleComponent()->SetCapsuleHalfHeight(105.f);

	// 최대 걷기 속도
	GetCharacterMovement()->MaxWalkSpeed = 350.0f;

	// 시야 설정
	SightRadius = 1100.f;
	LoseSightRadius = 1100.f;

	// SkeletalMesh Setting
	ConstructorHelpers::FObjectFinder<USkeletalMesh> TempMesh
		(TEXT("'/Game/_Resource/SkeletonSword/C_M_SkeletonSword.C_M_SkeletonSword'"));
	if (TempMesh.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(TempMesh.Object);
		GetMesh()->SetRelativeLocationAndRotation(FVector(0, 0, -90), FRotator(0, -90, 0));
		GetMesh()->SetRelativeScale3D(FVector(0.45f));
	}
	ConstructorHelpers::FObjectFinder<UMaterial> TempMesh_Mt(
		TEXT("'/Game/_Resource/SkeletonSword/CM_M_Skeleton.CM_M_Skeleton'"));
	if (TempMesh_Mt.Succeeded())
	{
		GetMesh()->SetMaterial(0, TempMesh_Mt.Object);
	}

	// WeaponMesh Setting	
	WeaponComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponComponent->SetupAttachment(GetMesh(),TEXT("Weapon_R"));

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> tempWeaponSkeletalMesh(
		TEXT("'/Game/_Resource/Sword/SKM_Sword.SKM_Sword'"));
	if (tempWeaponSkeletalMesh.Succeeded())
	{
		WeaponComponent->SetSkeletalMesh(tempWeaponSkeletalMesh.Object);
		WeaponComponent->SetRelativeScale3D(FVector(0.4f));
		WeaponComponent->ComponentTags.Add(TEXT("WeaponMesh"));
		static ConstructorHelpers::FObjectFinder<UMaterial> tempWeaponMaterial(
			TEXT("'/Game/_Resource/SkeletonSword/CM_I_SkeletonSword.CM_I_SkeletonSword'"));
		if (tempWeaponMaterial.Succeeded())
		{
			WeaponComponent->SetMaterial(0, tempWeaponMaterial.Object);
		}
	}

	// Set Animation Blueprint
	//애님 생성
	ConstructorHelpers::FClassFinder<UAnimInstance> TempAnim(TEXT(
		"/Script/Engine.AnimBlueprint'/Game/_BluePrints/AreaObject/Monster/SwordSkeleton/ABP_SwordSkeleton.ABP_SwordSkeleton_C'"));
	if (TempAnim.Succeeded())
	{
		GetMesh()->SetAnimInstanceClass(TempAnim.Class);
	}
}

// Called when the game starts or when spawned
void ASwordSkeleton::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ASwordSkeleton::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void ASwordSkeleton::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

UBaseAiFSM* ASwordSkeleton::CreateFSM()
{
	return CreateDefaultSubobject<UMonsterFSM>(TEXT("FSM"));
}
