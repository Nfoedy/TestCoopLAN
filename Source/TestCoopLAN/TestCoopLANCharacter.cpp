// Copyright Epic Games, Inc. All Rights Reserved.

#include "TestCoopLANCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "TestCoopLAN.h"
#include "Net/UnrealNetwork.h"

ATestCoopLANCharacter::ATestCoopLANCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

void ATestCoopLANCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ATestCoopLANCharacter::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ATestCoopLANCharacter::Look);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ATestCoopLANCharacter::Look);

		// Sprinting
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &ATestCoopLANCharacter::StartSprint);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Canceled, this, &ATestCoopLANCharacter::StopSprint);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &ATestCoopLANCharacter::StopSprint);
	}
	else
	{
		UE_LOG(LogTestCoopLAN, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ATestCoopLANCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// route the input
	DoMove(MovementVector.X, MovementVector.Y);
}

void ATestCoopLANCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route the input
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void ATestCoopLANCharacter::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void ATestCoopLANCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void ATestCoopLANCharacter::DoJumpStart()
{
	// signal the character to jump
	Jump();
}

void ATestCoopLANCharacter::DoJumpEnd()
{
	// signal the character to stop jumping
	StopJumping();
}


// Funzioni di Sprinting
// HasAuthority() = controlla se la copia locale di questo Character possiede ROLE_Authority
void ATestCoopLANCharacter::StartSprint()
{
	if (HasAuthority()) 
	{ 
		// Premuto dall'Host, il Listen Server possiede l'authority quindi viene modificato subito lo stato.
		SetSprinting(true); 
	}
	else
	{
		// Premuto dal Client remoto, quindi il client proprietario richiede il cambiamenta al server.
		ServerSetSprinting(true);
	}
}

void ATestCoopLANCharacter::StopSprint()
{
	if (HasAuthority())
	{
		// Stessa cosa dello StartSprint
		SetSprinting(false);
	}
	else
	{
		// Stessa cosa dello StartSprint
		ServerSetSprinting(false);
	}
}

// Questa è l'implementazione reale del Server RPC, l'ho dichiarata nell'header. Nel cpp appunto devo aggiungere _Implementation
// Contiene il codice che viene realmente eseguito sul server dopo aver ricevuto la richiesta.
void ATestCoopLANCharacter::ServerSetSprinting_Implementation(bool bNewSprinting)
{
	// Questa implementazione viene eseguita sul server 
	SetSprinting(bNewSprinting);
}

// Soltanto il server può cambiare lo stato autorevole
void ATestCoopLANCharacter::SetSprinting(bool bNewSprinting)
{
	if (!HasAuthority())
	{
		return;
	}

	if (bIsSprinting == bNewSprinting)
	{
		return;
	}

	bIsSprinting = bNewSprinting;

	// Aggiorna manualmente la velocità della copia del Char presente sul server, perchè OnRep non lo fa per lui. 
	ApplySprintSpeed();

	// Chiede al sistema di networking di valutare presto la replica
	ForceNetUpdate();

}

// Funzione per i client
void ATestCoopLANCharacter::OnRep_IsSprinting()
{
	// Eseguita quando un client riceve il nuovo stato del server
	ApplySprintSpeed();
}

// Funzione che modifica il valore di camminata del Char
void ATestCoopLANCharacter::ApplySprintSpeed()
{
	GetCharacterMovement()->MaxWalkSpeed = bIsSprinting ? SprintSpeed : WalkSpeed;
}


//
void ATestCoopLANCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{

}