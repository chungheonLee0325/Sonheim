// Fill out your copyright notice in the Description page of Project Settings.


#include "Lifmunk.h"
#include "Components/CapsuleComponent.h"


// Sets default values
ALifmunk::ALifmunk()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->SetCollisionProfileName(TEXT("Monster"));
	
	m_AreaObjectID = 109;
}

// Called when the game starts or when spawned
void ALifmunk::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ALifmunk::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void ALifmunk::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

