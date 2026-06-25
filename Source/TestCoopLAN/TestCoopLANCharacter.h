// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "TestCoopLANCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  A simple player-controllable third person character
 *  Implements a controllable orbiting camera
 */
UCLASS(abstract)
class ATestCoopLANCharacter : public ACharacter
{
	GENERATED_BODY()

public:

	/** Constructor */
	ATestCoopLANCharacter();

	// GetLifetimeReplicatedProps è una funzione prevista dal sistema di replication di Unreal.
	// Unreal la usa per costruire l'elenco delle propietà che devono essere replicate durante la vita delle istanze di questa
	// classe
	// TArray<FLifetimeProperty>& OutLifetiimeProps è l'array nel quale devo registrare le propietà replicate
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;



	// Public input interface

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles look inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoLook(float Yaw, float Pitch);

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoJumpStart();

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoJumpEnd();

	

	// Component Access

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }


protected:

	// Input setup & callbacks

	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	// Chiamata da locale per iniziare lo sprinting 
	void StartSprint();

	// Chiamata dal locale per fermare lo sprinting
	void StopSprint();


	// Sprint Networking
	
	/*
		UFUNCTION = registra una funzione. Permette di esplorla a Blueprint, RPC, delegate e altri sistemi Unreal,
					in base agli specifier inseriti tra parentesi

		UPROPERTY = registra una variabile. Permette ad unreal di gestirla nell'Editor, nei BP, nella serializzazione
					nel Garbage Collector, e nella replication, in base agli specifier utilizzati.
	*/

	// Richiede al server autorevole di cambiare lo stato dello sprint
	// Server = dice che questa è una funzione RPC. Se viene chiamata dal Client che possiede questo Char, Unreal invia
	//			la richiesta attraverso la rete ed esegue ServerSetSprinting_Implementation() sul server.
	// Reliable = Unreal garantisce che l'RPC venga consegnato ed eseguito, purchè la connessione rimanga attiva. Gli RPC 
	//			  Reliable mantengono anche il loro ordine rispetto agli altri RPC Reliable sullo stesso Actor. 
	//			  Vanno usati per eventi importanti e poco frequenti, non per ogni frame.
	// bNewSprinting = stato richiesto dal client. True per iniziare lo sprint, false per termianarlo.
	UFUNCTION(Server, Reliable)
	void ServerSetSprinting(bool bNewSprinting);


	// Chiamata ai clients quanto l'autoritative server cambia lo stato
	UFUNCTION()
	void OnRep_IsSprinting();


	// Spring implementation
	
	// Cambia la velocità del CharacterMovement
	void ApplySprintSpeed();

	// Cambia lo sprint state
	void SetSprinting(bool bNewSprinting);


	// Input assets

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* MouseLookAction;

	// Sprint Input Action aggiunto
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* SprintAction;


	// Spring properties
	// Aggiungo le proprietà

	// Camminata quando lo stato di sprint è disattivato 
	// EditDefaultsOnly = possibile modificarla solo dal BP del Character
	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float WalkSpeed = 500.0f;

	// Velocità quando lo stato di spint è attivo
	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float SprintSpeed = 1200.0f;

	// Stato dello sprint autorevole, replicato dal server ai client
	// ReplicatedUsing = sincronizza il valore e richiama una funzione sul ricevente. Usare DOREPLIFETIME per completare la config.
	// OnRep_IsSprinting = è una RepNotify. Usata per reagire all'aggiornamento
	UPROPERTY(ReplicatedUsing = OnRep_IsSprinting)
	bool bIsSprinting = false;


private:

	// Components

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;


};

