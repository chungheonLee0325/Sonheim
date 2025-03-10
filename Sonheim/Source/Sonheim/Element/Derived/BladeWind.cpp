// Fill out your copyright notice in the Description page of Project Settings.


#include "BladeWind.h"


// Sets default values
ABladeWind::ABladeWind()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ABladeWind::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ABladeWind::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

