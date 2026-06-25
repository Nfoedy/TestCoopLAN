// Fill out your copyright notice in the Description page of Project Settings.


#include "NetworkDoor.h"

// Sets default values
ANetworkDoor::ANetworkDoor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ANetworkDoor::BeginPlay()
{
	Super::BeginPlay();
	
}


// Soltanto il server può cambiare lo stato autorevole
void ANetworkDoor::SetDoorOpen(bool bNewIsOpen)
{
	if (!HasAuthority())
	{
		return;
	}

	ApplyDoorState;
}


void ANetworkDoor::OnRep_IsOpen()
{
	ApplyDoorState();
}




void ANetworkDoor::ApplyDoorState()
{

}

