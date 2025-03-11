// Fill out your copyright notice in the Description page of Project Settings.


#include "Pengullet.h"
#include "Components/CapsuleComponent.h"


// Sets default values
APengullet::APengullet()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->SetCollisionProfileName(TEXT("Monster"));
	
	m_AreaObjectID = 112;
}

// Called when the game starts or when spawned
void APengullet::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void APengullet::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void APengullet::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

