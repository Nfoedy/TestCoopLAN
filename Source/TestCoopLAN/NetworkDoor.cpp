// Fill out your copyright notice in the Description page of Project Settings.

#include "NetworkDoor.h"
#include "Net/UnrealNetwork.h"

// Sets default values
ANetworkDoor::ANetworkDoor()
{
	// La porta non deve aggiornarsi ogni frame.
	// Cambierà posizione solo quando cambia il suo stato.
	PrimaryActorTick.bCanEverTick = false;

	// Questo actor deve essere replicato dal server ai client
	bReplicates = true;

	SetReplicateMovement(false);

}

// Called when the game starts or when spawned
void ANetworkDoor::BeginPlay()
{
	Super::BeginPlay();

	// Salvo la posizione iniziale della porta. Questa sarà la posizione chiusa
	ClosedLocation = GetActorLocation();

	// Applico lo stato iniziale della porta
	ApplyDoorState();
}


// Soltanto il server può cambiare lo stato autorevole
void ANetworkDoor::SetDoorOpen(bool bNewIsOpen)
{
	if (!HasAuthority())
	{
		return;
	}

	if (bIsOpen == bNewIsOpen)
	{
		return;
	}

	// Il server modifica lo stato autorevole
	bIsOpen = bNewIsOpen;

	// OnRep non viene chiamata direttamente sul server
	ApplyDoorState();

	// Chiedo ad Unreal di aggiornare presto questo Actor nella rete
	ForceNetUpdate();
}


void ANetworkDoor::OnRep_IsOpen()
{
	ApplyDoorState();
}



// Muove la porta
void ANetworkDoor::ApplyDoorState()
{
	const FVector TargetLocation = bIsOpen ? ClosedLocation + FVector(0.0f, 0.0f, OpenHeight) : ClosedLocation;

	SetActorLocation(TargetLocation);
}


// Registra bIsOpen dal server ai client
void ANetworkDoor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const 
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANetworkDoor, bIsOpen);
}