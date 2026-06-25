// Fill out your copyright notice in the Description page of Project Settings.

#include "NetworkDoor.h"
#include "Net/UnrealNetwork.h"

// Sets default values
ANetworkDoor::ANetworkDoor()
{
	// La porta non ha bisogno del Tick: cambia stato solo quando viene aperta/chiusa.
	PrimaryActorTick.bCanEverTick = false;

	// Questo Actor deve esistere anche sui client.
	bReplicates = true;

	// Non replichiamo direttamente il movimento dell'Actor.
	// Replichiamo invece bIsOpen e ogni client applica localmente la posizione corretta.
	SetReplicateMovement(false);
}

// Called when the game starts or when spawned
void ANetworkDoor::BeginPlay()
{
	Super::BeginPlay();

	// Salvo la posizione iniziale della porta.
	// Questa sarà la posizione "chiusa".
	ClosedLocation = GetActorLocation();

	// Applico lo stato iniziale.
	ApplyDoorState();
}

// Soltanto il server può cambiare lo stato autorevole della porta.
void ANetworkDoor::SetDoorOpen(bool bNewIsOpen)
{
	if (!HasAuthority())
	{
		return;
	}

	// Se la porta è già nello stato richiesto, non faccio nulla.
	if (bIsOpen == bNewIsOpen)
	{
		return;
	}

	// Il server modifica lo stato autorevole.
	bIsOpen = bNewIsOpen;

	// OnRep non viene chiamata automaticamente sul server,
	// quindi applichiamo subito lo stato anche sulla copia autorevole.
	ApplyDoorState();

	// Chiedo a Unreal di considerare presto questo Actor per la replica.
	ForceNetUpdate();
}

// Chiamata sui client quando ricevono un nuovo valore di bIsOpen.
void ANetworkDoor::OnRep_IsOpen()
{
	ApplyDoorState();
}

// Applica visivamente lo stato della porta.
void ANetworkDoor::ApplyDoorState()
{
	const FVector TargetLocation = bIsOpen
		? ClosedLocation + FVector(0.0f, 0.0f, OpenHeight)
		: ClosedLocation;

	SetActorLocation(TargetLocation);
}

// Registra le proprietà replicate della porta.
void ANetworkDoor::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANetworkDoor, bIsOpen);
}