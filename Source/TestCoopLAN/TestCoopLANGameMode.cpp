// Copyright Epic Games, Inc. All Rights Reserved.

#include "TestCoopLANGameMode.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

// Per creare Log
DEFINE_LOG_CATEGORY_STATIC(LogNetworkGameMode, Log, All);



ATestCoopLANGameMode::ATestCoopLANGameMode()
{
	// stub
}


// Log a schermo per quando l'Host o il Client entrano nella partita
void ATestCoopLANGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	FString PlayerName = TEXT("UnknownPlayer");
	const FString ControllerName = GetNameSafe(NewPlayer);

	APlayerState* NewPlayerState = nullptr;

	if (NewPlayer)
	{
		NewPlayerState = NewPlayer->PlayerState.Get();
	}

	if (NewPlayerState)
	{
		PlayerName = NewPlayerState->GetPlayerName();
	}

	const int32 ConnectedPlayers = GetNumPlayers();

	if (GEngine)
	{
		const FString Message = FString::Printf(
			TEXT("PLAYER JOINED SESSION | PlayerName=%s | Controller=%s | ConnectedPlayers=%d"),
			*PlayerName,
			*ControllerName,
			ConnectedPlayers
		);

		GEngine->AddOnScreenDebugMessage(
			-1,
			10.f,
			FColor::Green,
			Message
		);
	}
}

// Log per quando l'Host o il Client escono dalla partita. Tipo per disconnessione o crash. Mi dice chi è uscito
void ATestCoopLANGameMode::Logout(AController* Exiting)
{
	FString PlayerName = TEXT("UnknownPlayer");
	const FString ControllerName = GetNameSafe(Exiting);

	APlayerState* ExitingPlayerState = nullptr;

	if (Exiting)
	{
		ExitingPlayerState = Exiting->PlayerState.Get();
	}

	if (ExitingPlayerState)
	{
		PlayerName = ExitingPlayerState->GetPlayerName();
	}

	Super::Logout(Exiting);

	const int32 RemainingPlayers = GetNumPlayers();

	if (GEngine)
	{
		const FString Message = FString::Printf(
			TEXT("PLAYER LEFT SESSION | PlayerName=%s | Controller=%s | RemainingPlayers=%d"),
			*PlayerName,
			*ControllerName,
			RemainingPlayers
		);

		GEngine->AddOnScreenDebugMessage(
			-1,
			10.f,
			FColor::Red,
			Message
		);
	}
}
