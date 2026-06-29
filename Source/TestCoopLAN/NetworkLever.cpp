// Fill out your copyright notice in the Description page of Project Settings.


#include "NetworkLever.h"
#include "NetworkDoor.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"

class NetworkDoor;


// Sets default values
ANetworkLever::ANetworkLever()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	// Permette all'Actor di replicare le sue proprietà ai client
	bReplicates = true;

	// Non replichiamo direttamente il movimento/rotazione dell'Actor.
	// Replichiamo solo lo stato bIsActive e ogni macchina applica la rotazione localmente.
	SetReplicateMovement(false);

}

// Called when the game starts or when spawned
void ANetworkLever::BeginPlay()
{
	Super::BeginPlay();

	// Se il BoxCollision non è stato assegnato nell'editor restituisce errore
	if (!BoxCollision)
	{
		UE_LOG(LogTemp, Error, TEXT("NetworkLever: BoxCollision non assegnato nell'Editor."));
		return;
	}
	// Se la mesh della leva non è stata assegnata nell'editor restituisce errore
	if (!LeverMesh)
	{
		UE_LOG(LogTemp, Error, TEXT("NetworkLever: LeverMesh non assegnata nell'Editor."));
		return;
	}

	// Collego gli eventi del BoxCollison alle funzioni C++
	// Da questo momento se un Actor entra nel box, Unreal chiamerà OnBoxBeginOverlap(), quando esce OnBoxEndOverlap()
	// AddDynamic = quando il box rileva un Begin/EndOverlap, chiama la funzione OnBox...Overlap dell'oggetto
	if (BoxCollision)
	{
		BoxCollision->OnComponentBeginOverlap.AddDynamic(this, &ANetworkLever::OnBoxBeginOverlap);

		BoxCollision->OnComponentEndOverlap.AddDynamic(this, &ANetworkLever::OnBoxEndOverlap);
	}

	// Salvo la rotazione iniziale della mesh della leva, questa sarà la pos da disattiava
	if (LeverMesh)
	{
		InitialLeverRotation = LeverMesh->GetRelativeRotation();
	}

	// Counter dei players iniziale
	OverlappingPlayersCount = 0;

	// Applica lo stato iniziale
	ApplyLeverState();

}


void ANetworkLever::OnBoxBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	// La logica della leva deve essere gestita solo dal server. IMPORTANTISSIMO --> Solo il server decide 
	if (!HasAuthority())
	{
		return;
	}

	// Controllo che l'actor entrato nel box sia valido
	if (!OtherActor)
	{
		return;
	}

	// Controllo che l'actor entrato sia un Pawn, quindi un player/personaggio
	APawn* OverlappingPawn = Cast<APawn>(OtherActor);
	if (!OverlappingPawn) return;

	// Un actor è entrato nell'area della leva;
	OverlappingPlayersCount++;

	// Attivo la leva
	SetLeverActivated(true);

}

void ANetworkLever::OnBoxEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	// La logica della leva deve essere gestita solo dal server
	if (!HasAuthority())
	{
		return;
	}

	// Controllo che l'actor uscito nel box sia valido
	if (!OtherActor)
	{
		return;
	}

	// Controllo che l'actor uscito sia un Pawn, quindi un player/personaggio
	APawn* OverlappingPawn = Cast<APawn>(OtherActor);
	if (!OverlappingPawn) return;


	// Evito che il contatore vada sotto zero.
	if (OverlappingPlayersCount < 0)
	{
		OverlappingPlayersCount = 0;
	}

	// Se non c'è più nessun player nel box, disattivo la leva.
	if (OverlappingPlayersCount == 0)
	{
		SetLeverActivated(false);
	}

}



void ANetworkLever::SetLeverActivated(bool bNewIsActivated)
{
	// Solo il server può cambiare lo stato della leva
	if (!HasAuthority())
	{
		return;
	}

	// Se la leva è già nello stato richiesto, non fa nulla
	if (bIsActivated == bNewIsActivated)
	{
		return;
	}

	// Cambio lo stato della leva
	bIsActivated = bNewIsActivated;

	// Applico subito la rotazione della leva sul Server.
	// I client invece lo faranno tramite OnRep_IsActivated().
	ApplyLeverState();

	// Se è stata assegnata una porta, la apro/chiudo in base allo stato della leva
	if (DoorToOpen)
	{
		DoorToOpen->SetDoorOpen(bIsActivated);             // Da capire meglio questa parte
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("NetworkLever: DoorToOpen non assegnata."));    // Warning
	}

	// Chiedo ad Unreal di aggiornare rapidamente i client
	ForceNetUpdate();

}


void ANetworkLever::OnRep_IsActivated()
{
	// Questa funzione viene chiamata sui client quando ricevono il nuovo valore di bIsActivated dal server.
	ApplyLeverState();
}


void ANetworkLever::ApplyLeverState()
{
	
}



// Registra bIsActivated dal server ai client
void ANetworkLever::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANetworkLever, bIsActivated);
}

