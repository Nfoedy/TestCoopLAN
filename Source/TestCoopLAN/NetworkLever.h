// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NetworkLever.generated.h"


class UStaticMeshComponent;
class UBoxComponent;
class UPrimitiveComponent;
class ANetworkDoor;


UCLASS()
class TESTCOOPLAN_API ANetworkLever : public AActor
{
	GENERATED_BODY()


public:	

	// Sets default values for this actor's properties
	ANetworkLever();

	// Registra le proprietà replicate della leva
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;



protected:

	// Chiamata quando il gioco starta
	virtual void BeginPlay() override;

	// Chiamata quando un Actor entra nel BoxCollider della leva
	// OverlappedComponent = il componente della leva che genera l'overlap (il box collision)
	// OtherActor = l'actor che è entrato nel box collision
	// OtherComp = il component specifico dell'other actor che ha toccato il BoxCollision, in questo caso la capsula del player
	// OtherBodyIndex = indice interno usato da Unreal per identificare il body fisico coinvolto nell'overlap
	// bFromSweep = true se l'overlap è stato generato da un movimento/sweep
	// SweepResult = info dettagliate sull'impatto/overlap
	UFUNCTION()
	void OnBoxBeginOverlap(
		UPrimitiveComponent* OverlappedComponent, 
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);    


	// Chiamata quando un Actor esce dal BoxCollision della leva
	// OverlappedComponent = il componente della leva che ha generato l'end overlap (il box collision)
	// OtherActor = l'actor che è uscito dal box collision
	// OtherComp = il componente specifico del player che non sta più sovrapponendo il box collision
	// OtherBodyIndex = indice interno usato da Unreal per identificare il body fisico coinvolto nell'overlap
	UFUNCTION()
	void OnBoxEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);


	// Cambia lo stato della leva, deve essere chiamata solo dal server
	void SetLeverActivated(bool bNewIsActivated);


	// Chiamata sui client quando il server cambia lo stato
	UFUNCTION()
	void OnRep_IsActivated();


	// Applica visivamente il nuovo stato della leva
	void ApplyLeverState();


protected:

	// La porta controllata da questa leva, da assegnare nell'editor
	// Perche category = Lever per ordine visivo nell'editor
	UPROPERTY(EditInstanceOnly, Category = "Lever")
	ANetworkDoor* DoorToOpen = nullptr;

	// Rotazione applicata quando la leva e' attiva
	UPROPERTY(EditDefaultsOnly, Category = "Lever")
	float ActivatedRollOffset = 45.0f;


	// Stato della leva autorevole, replicato dal server ai client
	UPROPERTY(ReplicatedUsing = OnRep_IsActivated)
	bool bIsActivated = false;



private:
	
	// Riferimento runtime al BoxCollision creato nel Blueprint.
	// Viene trovato automaticamente in BeginPlay cercando un componente chiamato "BoxCollision".
	UBoxComponent* BoxCollisionRef = nullptr;

	// Riferimento runtime alla mesh della leva creata nel Blueprint.
	// Viene trovata automaticamente in BeginPlay cercando un componente chiamato "LeverMesh".
	UStaticMeshComponent* LeverMeshRef = nullptr;

	// Rotazione iniziale della leva, per farla ritornare al punto iniziale quando l'actor esce
	FRotator InitialLeverRotation;

	// Numero di player attualmente dentro il box, se > 0 resta attiva
	int32 OverlappingPlayersCount = 0;


};
