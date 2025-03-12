// Fill out your copyright notice in the Description page of Project Settings.


#include "Foxparks.h"
#include "Components/CapsuleComponent.h"


// Sets default values
AFoxparks::AFoxparks()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->SetCollisionProfileName(TEXT("Monster"));
	
	m_AreaObjectID = 106;
}

// Called when the game starts or when spawned
void AFoxparks::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AFoxparks::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void AFoxparks::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

