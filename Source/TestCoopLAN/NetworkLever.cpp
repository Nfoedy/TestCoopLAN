// Fill out your copyright notice in the Description page of Project Settings.


#include "NetworkLever.h"
#include "NetworkDoor.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"


// Sets default values
ANetworkLever::ANetworkLever()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void ANetworkLever::BeginPlay()
{
	Super::BeginPlay();

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

void OnBoxBeginOverlap()
{

}

void OnBoxEndOverlap()
{

}


void ApplyLeverState()
{
	bool bIsActivated = true;
}




void ANetworkLever::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANetworkLever, bIsActivated);
}

