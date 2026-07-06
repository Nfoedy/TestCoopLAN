// Fill out your copyright notice in the Description page of Project Settings.


#include "NetworkSessionSubsystem.h"

#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"


UNetworkSessionSubsystem::UNetworkSessionSubsystem()
	: CreateSessionCompleteDelegate(FOnCreateSessionCompleteDelegate::CreateUObject(this, &UNetworkSessionSubsystem::OnCreateSessionComplete))
	, FindSessionsCompleteDelegate(FOnFindSessionsCompleteDelegate::CreateUObject(this, &UNetworkSessionSubsystem::OnFindSessionsComplete))
	, JoinSessionCompleteDelegate(FOnJoinSessionCompleteDelegate::CreateUObject(this, &UNetworkSessionSubsystem::OnJoinSessionComplete))
	, DestroySessionCompleteDelegate(FOnDestroySessionCompleteDelegate::CreateUObject(this, &UNetworkSessionSubsystem::OnDestroySessionComplete))
{
	// Prende il subsystem attivo, cioè Steam se il .ini è giusto
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();

	if (OnlineSubsystem)
	{
		SessionInterface = OnlineSubsystem->GetSessionInterface();

		const FString SubsystemName = OnlineSubsystem->GetSubsystemName().ToString();

		const bool bHasSessionInterface = SessionInterface.IsValid();

		FString PlayerName = TEXT("Unknown");

		IOnlineIdentityPtr IdentityInterface = OnlineSubsystem->GetIdentityInterface();

		if (IdentityInterface.IsValid())
		{
			PlayerName = IdentityInterface->GetPlayerNickname(0);
		}

		const FString DebugMessage = FString::Printf(
			TEXT("OnlineSubsystem: %s | SessionInterface: %s | Player: %s"),
			*SubsystemName,
			bHasSessionInterface ? TEXT("Valid") : TEXT("Invalid"),
			*PlayerName
		);

		UE_LOG(LogTemp, Warning, TEXT("NETWORK_DEBUG: %s"), *DebugMessage);

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				15.f,
				FColor::Cyan,
				DebugMessage
			);
		}
	}

	else
	{
		UE_LOG(LogTemp, Error, TEXT("NETWORK_DEBUG: OnlineSubsystem is NULL"));

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				15.f,
				FColor::Red,
				TEXT("OnlineSubsystem: NULL")
			);
		}
	}

}


// 
void UNetworkSessionSubsystem::CreateSession(int32 NumPublicConnections, FString MatchType)
{
	if (!SessionInterface.IsValid())
	{
		NetworkOnCreateSessionComplete.Broadcast(false);
		return;
	}

	// Registro il delegate interno di Unreal. Quando CreateSession finirà, Unreal chiamerà OnCreateSessionComplete
	CreateSessionCompleteDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegate);

	// Creo le impostazioni della sessione
	// Uso TsharedPtr perchè queste impostazioni devono restare vive anche dopo la fine di questa funzione
	LastSessionSettings = MakeShareable(new FOnlineSessionSettings());

	LastSessionSettings->bIsLANMatch = false;
	LastSessionSettings->NumPublicConnections = NumPublicConnections;
	LastSessionSettings->bAllowJoinInProgress = true;
	LastSessionSettings->bAllowJoinViaPresence = true;		// permette di accedere tramite presenza online/Steam
	LastSessionSettings->bShouldAdvertise = true;			// rende la sessione visibile nelle ricerche
	LastSessionSettings->bUsesPresence = true;				// Usa il sistema di Presence di Steam
	LastSessionSettings->bUseLobbiesIfAvailable = true;		// Dice a Steam di usare lobby se disponibili. è importante per trovare/joinare sessioni in modo moderno
	LastSessionSettings->BuildUniqueId = 1;
	// Salvo un dato custom dentro la sessione, in futuro serve per cercare solo sessioni con MatchType uguale a quello che vogliamo.
	LastSessionSettings->Set(FName("MatchType"), MatchType,	EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	// Prendo il Local player, ovvero il player locale che sta creando la sessione
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();

	// Chiedo all'Online Subsystem di creare davvero la sessione
	const bool bCreateSessionStarted = SessionInterface->CreateSession(
		*LocalPlayer->GetPreferredUniqueNetId(),
		NAME_GameSession,		// NAME_GameSession è il nome standard della sessione principale di gioco
		*LastSessionSettings
	);

	// Se Craete Session ritorna false, vuol dire che la richiesta non è nemmeno partita
	if (!bCreateSessionStarted)
	{
		// Stacco il delegate perchè non riceve nessuna callback
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);

		// Avviso il resto del gioco che la creazione è fallita
		NetworkOnCreateSessionComplete.Broadcast(false);
	}

}


// 
void UNetworkSessionSubsystem::FindSessions(int32 MaxSearchResults)
{
	// Se la SessionInterface non è valida, non posso cercare sessioni
	if (!SessionInterface.IsValid())
	{
		NetworkOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
		return;
	}

	// Registro il delegate interno. Quando la ricerca finisce, Unreal chiamerà OnFindSessionsComplete
	FindSessionsCompleteDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegate);

	// Creo l'oggetto che contiene i paramentri della ricerca. Deve restare vivo fino a quando la ricerca non finisce
	LastSessionSearch = MakeShareable(new FOnlineSessionSearch());

	LastSessionSearch->MaxSearchResults = MaxSearchResults;			// Numero massimo di sessioni che vogliamo trovare
	LastSessionSearch->bIsLanQuery = false;	

	// Cerco sessioni basate su Presence/Lobby. Deve combaciare con bUsesPresence = true in CreateSession.
	LastSessionSearch->QuerySettings.Set(
		FName(TEXT("LOBBYSEARCH")),
		true,
		EOnlineComparisonOp::Equals
	);


	LastSessionSearch->QuerySettings.Set(
		FName(TEXT("MINSLOTSAVAILABLE")),
		1,
		EOnlineComparisonOp::GreaterThanEquals
	);



	// Prendo il player locale che sta facendo la ricerca.
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();

	if (!LocalPlayer)
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
		NetworkOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
		return;
	}


	// Chiedo all'Online subsystem di cercare le sessione
	const bool bFindSessionsStarted = SessionInterface->FindSessions(
		*LocalPlayer->GetPreferredUniqueNetId(),
		LastSessionSearch.ToSharedRef()
	);


	// Se ritorna false, la ricerca non è nemmeno partita
	if (!bFindSessionsStarted)
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
		NetworkOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
	}


}


// 
void UNetworkSessionSubsystem::JoinSession(const FOnlineSessionSearchResult& SessionResult)
{
	// Se la sessione non è valida non fa entrare
	if (!SessionInterface.IsValid())
	{
		NetworkOnJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
		return;
	}

	// Registro il delegate interno. Quando il tentativo di Join finisce Unreal chiamera la OnJoinSessionComplete
	JoinSessionCompleteDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegate);

	// Prendo il Player Locale che vuole entrare nella sessione
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();

	if (!LocalPlayer)
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
		NetworkOnJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
		return;
	}

	// Chiedo all'Online Subsystem di entrare nella sessione trovata
	const bool bJoinSessionStarted = SessionInterface->JoinSession(
		*LocalPlayer->GetPreferredUniqueNetId(),
		NAME_GameSession,
		SessionResult
	);

	// Se ritorna false, il tentativo di join non è nemmeno partito
	if (!bJoinSessionStarted)
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
		NetworkOnJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
	}

}


//
void UNetworkSessionSubsystem::JoinSessionByIndex(int32 SessionIndex)
{
	// Controllo che esista una ricerca valida
	if (!LastSessionSearch.IsValid())
	{
		NetworkOnJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
		return;
	}

	// Controllo che l'indice scelto sia valido nella lista delle sessioni trovate
	if (!LastSessionSearch->SearchResults.IsValidIndex(SessionIndex))
	{
		NetworkOnJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
		return;
	}

	// Uso la funzione JoinSession normale, passando la sessione trovata a quell'indice
	JoinSession(LastSessionSearch->SearchResults[SessionIndex]);
}


//
void UNetworkSessionSubsystem::DestroySession()
{
	// Se la sessione non è valida non posso distruggere nessuna sessione
	if (!SessionInterface.IsValid())
	{
		NetworkOnDestroySessionComplete.Broadcast(false);
		return;
	}
	
	// Registro il delegate interno. Quando la distruzione della sessione finisce, Unreal chiamerà OnDestroySessionComplete
	DestroySessionCompleteDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegate);

	// Chiedo all'Online Subsystem di distruggere la sessione principale di gioco 
	const bool bDestroySessionStarted = SessionInterface->DestroySession(NAME_GameSession);

	// Se ritorna false la richiesta non è nemmeno partita
	if (!bDestroySessionStarted)
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
		NetworkOnDestroySessionComplete.Broadcast(false);
	}

}


// Restituisce il numero di sessioni trovate
int32 UNetworkSessionSubsystem::GetSessionSearchResultsCount() const
{
	// Se non fa una ricerca valida, non ci sono risultati
	if (!LastSessionSearch.IsValid()) {
		return 0;
	}

	return LastSessionSearch->SearchResults.Num();
}


// Restituisce il nome delle sessioni trovate
FString UNetworkSessionSubsystem::GetSessionSearchResultName(int32 SessionIndex) const
{
	// Se non ho una ricarca valida o l'indice non esiste, ritorna un testo di fallback
	if (!LastSessionSearch.IsValid() || !LastSessionSearch->SearchResults.IsValidIndex(SessionIndex))
	{
		return FString(TEXT("Invalid Session"));
	}

	const FOnlineSessionSearchResult& SearchResult = LastSessionSearch->SearchResults[SessionIndex];

	// Provo a leggere il MatchType che ho salvato in CreateSession
	FString MatchType;

	if (SearchResult.Session.SessionSettings.Get(FName("MatchType"), MatchType))
	{
		return MatchType;
	}

	// Se non trovo MatchType, mostro comunque qualcosa di leggibile
	return FString::Printf(TEXT("Session %d"), SessionIndex);

}




//
void UNetworkSessionSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	// La richiesta è finita, quinid posso rimuovere il delegate
	if (SessionInterface)
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
	}

	// Avviso UI/menu/altre classi che la creazione della sessione è terminata, bWasSuccesful indica se è andata bene oppure no
	NetworkOnCreateSessionComplete.Broadcast(bWasSuccessful);

	// Se la sessione non è stata creata correttamente, non cambio mappa.
	if (!bWasSuccessful)
	{
		return;
	}

	// SERVER TRAVEL: apre la mappa come Listen Server e porta con se tutti i Client collegati.

	// Apro la mappa come Listen Server
	// ?listen significa che questa istanza diventa host/server e può accettare client
	UWorld* World = GetWorld();

	if (World)
	{
		World->ServerTravel(TEXT("/Game/ThirdPerson/Lvl_ThirdPerson?listen"));
	}

	const FString DebugMessage = FString::Printf(
		TEXT("CreateSessionComplete: %s"),
		bWasSuccessful ? TEXT("SUCCESS") : TEXT("FAILED")
	);

	UE_LOG(LogTemp, Warning, TEXT("NETWORK_DEBUG: %s"), *DebugMessage);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			10.f,
			FColor::Green,
			DebugMessage
		);
	}
}


//
void UNetworkSessionSubsystem::OnFindSessionsComplete(bool bWasSuccessful)
{
	// La ricerca è finita, quindi rimuovo il delegate
	if (SessionInterface)
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
	}

	// Se la ricerca non esiste oppure non ha trovato risultati, avvisiamo che è fallita/vuota
	// if (!LastSessionSearch.IsValid() || LastSessionSearch->SearchResults.Num() < 0)
	// {
		//NetworkOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
		//return;
	// }

	const int32 NumResults = LastSessionSearch.IsValid() ? LastSessionSearch->SearchResults.Num() : -1;

	const FString DebugMessage = FString::Printf(
		TEXT("FindSessionsComplete: %s | Results: %d"),
		bWasSuccessful ? TEXT("SUCCESS") : TEXT("FAILED"),
		NumResults
	);

	UE_LOG(LogTemp, Warning, TEXT("NETWORK_DEBUG: %s"), *DebugMessage);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Yellow, DebugMessage);
	}

	if (!LastSessionSearch.IsValid() || LastSessionSearch->SearchResults.Num() <= 0)
	{
		NetworkOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
		return;
	}


	// Avviso UI/menu/altre classi passando la lista delle sessioni trovate
	NetworkOnFindSessionsComplete.Broadcast(LastSessionSearch->SearchResults, bWasSuccessful);

}


//
void UNetworkSessionSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	// Il tentativo di Join è finito, quindi rimuovo il delegate
	if (SessionInterface)
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
	}

	// Avviso UI/menu/altre classi del risultato del Join
	NetworkOnJoinSessionComplete.Broadcast(Result);

	// Se il join è andato bene, non faccio nessun travel
	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		return;
	}

	// Chiedo all'Online subsystem l'indirizzo raele della sessione, con Steam non devo costruirlo a mano
	FString ConnectString;

	if (!SessionInterface->GetResolvedConnectString(SessionName, ConnectString))
	{
		return;
	}

	// Prendo il PlayerController locale
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();

	if (!PlayerController)
	{
		return;
	}

	// Sposto il client nella mappa/sessione dell'Host
	PlayerController->ClientTravel(ConnectString, ETravelType::TRAVEL_Absolute);


}

// 
void UNetworkSessionSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	// La distruzione della sessione è finita quindi rimuovo il delegate
	if (SessionInterface)
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
	}

	// Avviso UI/Menu/altre classi che la sessione è stata distrutta oppure il tentativo è fallito
	NetworkOnDestroySessionComplete.Broadcast(bWasSuccessful);
}