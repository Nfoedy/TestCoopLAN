// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NetworkDoor.generated.h"

UCLASS()
class TESTCOOPLAN_API ANetworkDoor : public AActor
{
	GENERATED_BODY()
	
public:	

	// Sets default values for this actor's properties
	ANetworkDoor();

	// Registra le proprietà replicate della porta
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Cambia lo stato autorevole della porta. Deve essere chiamata dal server.
	void SetDoorOpen(bool bNewIsOpen);



protected:

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Altezza di apertura della porta
	UPROPERTY(EditDefaultsOnly, Category = "Door")
	float OpenHeight = 300.0f;

	// Stato autorevole della porta
	// Stato modificato dal server e replicato ai client 
	UPROPERTY(ReplicatedUsing = OnRep_IsOpen)
	bool bIsOpen = false;

	// Chiamata ai client quando ricevono un nuovo valore di bIsOpen.
	UFUNCTION()
	void OnRep_IsOpen();

	// Applica il nuovo stato alla porta
	void ApplyDoorState();


private: 

	// Posizione iniziale della porta, viene salvata per dire che la porta è chiusa.
	FVector ClosedLocation;



};
