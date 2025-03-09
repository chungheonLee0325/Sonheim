// Fill out your copyright notice in the Description page of Project Settings.


#include "Gumoss.h"


// Sets default values
AGumoss::AGumoss()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	m_AreaObjectID = 100;
}

// Called when the game starts or when spawned
void AGumoss::BeginPlay()
{
	Super::BeginPlay();

	//UpdateCurrentSkill()
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

