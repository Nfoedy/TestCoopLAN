// Fill out your copyright notice in the Description page of Project Settings.


#include "NetworkSessionSubsystem.h"

#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"


// Costruttore: inizializza i delegate delle operazioni online e reupera SessionInteface dal subsystem attivo
UNetworkSessionSubsystem::UNetworkSessionSubsystem()
	: CreateSessionCompleteDelegate(FOnCreateSessionCompleteDelegate::CreateUObject(this, &UNetworkSessionSubsystem::OnCreateSessionComplete))
	, FindSessionsCompleteDelegate(FOnFindSessionsCompleteDelegate::CreateUObject(this, &UNetworkSessionSubsystem::OnFindSessionsComplete))
	, JoinSessionCompleteDelegate(FOnJoinSessionCompleteDelegate::CreateUObject(this, &UNetworkSessionSubsystem::OnJoinSessionComplete))
	, DestroySessionCompleteDelegate(FOnDestroySessionCompleteDelegate::CreateUObject(this, &UNetworkSessionSubsystem::OnDestroySessionComplete))
{
	 // Recupera l'Online Subsystem attivo, nel mio caso Steam, configurato in DefaultEngine.ini
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();

	if (!OnlineSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("NETWORK_SESSION: OnlineSubsystem is NULL."));
		return;
	}

	// Recupera l'interfaccia delle sessioni. Questo permette di creare, cercare, joinare e distruggere sessioni online
	SessionInterface = OnlineSubsystem->GetSessionInterface();

	const FString SubsystemName = OnlineSubsystem->GetSubsystemName().ToString();
	const bool bHasValidSessionInterface = SessionInterface.IsValid();

	// Recupera in Nickname del Player
	FString PlayerName = TEXT("Unknown");

	IOnlineIdentityPtr IdentityInterface = OnlineSubsystem->GetIdentityInterface();

	if (IdentityInterface.IsValid())
	{
		PlayerName = IdentityInterface->GetPlayerNickname(0);
	}

	// Log iniziale del subsystem. Solo nel log, non a schermo, perché è informazione tecnica
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("NETWORK_SESSION: OnlineSubsystem=%s | SessionInterface=%s | Player=%s"),
		*SubsystemName,
		bHasValidSessionInterface ? TEXT("Valid") : TEXT("Invalid"),
		*PlayerName
	);
}



// Crea una sessione Steam come Host e configura le impostazioni necessarie per renderla trovabile dal client
void UNetworkSessionSubsystem::CreateSession(int32 NumPublicConnections, FString MatchType)
{
	//Se la SessionInterface non è valida, può succedere se l'OnlineSubsystem non è stato inizializzato correttamente
	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("NETWORK_SESSION: CreateSession failed. SessionInterface is invalid."));

		NetworkOnCreateSessionComplete.Broadcast(false);
		return;
	}

	// Recupera il World e il player locale che sta creando la sessione. Se uno dei due non validi, restituisce errore
	UWorld* World = GetWorld();

	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("NETWORK_SESSION: CreateSession failed. World is NULL."));

		NetworkOnCreateSessionComplete.Broadcast(false);
		return;
	}

	const ULocalPlayer* LocalPlayer = World->GetFirstLocalPlayerFromController();

	if (!LocalPlayer || !LocalPlayer->GetPreferredUniqueNetId().IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("NETWORK_SESSION: CreateSession failed. LocalPlayer or NetId is invalid."));

		NetworkOnCreateSessionComplete.Broadcast(false);
		return;
	}

	// Registra il delegate interno. Quando Unreal/Steam termina la creazione della sessione verrà chiamata OnCreateSessionComplete
	CreateSessionCompleteDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegate);

	// Crea le impostazioni della sessione. 
	// Uso TSharedPtr perché l'operazione è asincrona: queste impostazioni devono restare valide anche dopo la fine di questa funzione
	LastSessionSettings = MakeShareable(new FOnlineSessionSettings());

	LastSessionSettings->bIsLANMatch = false;
	LastSessionSettings->NumPublicConnections = NumPublicConnections;

	// Impostazioni Steam/Online. La sessione deve essere visibile, joinabile e basata su Presence/Lobby.
	LastSessionSettings->bAllowJoinInProgress = true;
	LastSessionSettings->bAllowJoinViaPresence = true;
	LastSessionSettings->bShouldAdvertise = true;
	LastSessionSettings->bUsesPresence = true;
	LastSessionSettings->bUseLobbiesIfAvailable = true;

	// BuildUniqueId deve combaciare con il BuildIdOverride nel DefaultEngine.ini.
	// Serve a far vedere tra loro solo client con la stessa build.
	LastSessionSettings->BuildUniqueId = 666;

	// Dato custom della sessione. Usato in FindSessions per filtrare solo le sessioni del nostro gioco
	LastSessionSettings->Set(
		FName("MatchType"),
		MatchType,
		EOnlineDataAdvertisementType::ViaOnlineServiceAndPing
	);

	
	// Richiede all'OnlineSubsystem di creare la sessione.
	// Se ritorna true, la richiesta è partita.
	// Il risultato finale arriverà in OnCreateSessionComplete.
	 
	const bool bCreateSessionStarted = SessionInterface->CreateSession(
		*LocalPlayer->GetPreferredUniqueNetId(),
		NAME_GameSession,
		*LastSessionSettings
	);

	UE_LOG(LogTemp, Warning, TEXT("NETWORK_SESSION: CreateSession started=%s | MatchType=%s | PublicConnections=%d"),
		bCreateSessionStarted ? TEXT("true") : TEXT("false"),
		*MatchType,
		NumPublicConnections
	);

	// Se CreateSession ritorna false, la richiesta non è nemmeno partita.
	if (!bCreateSessionStarted)
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);

		NetworkOnCreateSessionComplete.Broadcast(false);
	}
}



// Cerca le sessioni Steam disponibili e prepara i risultati che verranno poi filtrati in OnFindSessionComplete 
void UNetworkSessionSubsystem::FindSessions(int32 MaxSearchResults)
{
	// Se la SessionInterface non è valida, non possiamo cercare sessioni	
	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("NETWORK_SESSION: FindSessions failed. SessionInterface is invalid."));

		NetworkOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
		OnFindSessionsCompleteBP.Broadcast(0, false);
		return;
	}

	// Recupera il World e il player locale che sta cercando sessioni. Steam deve sapere quale utente sta facendo la ricerca
	UWorld* World = GetWorld();

	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("NETWORK_SESSION: FindSessions failed. World is NULL."));

		NetworkOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
		OnFindSessionsCompleteBP.Broadcast(0, false);
		return;
	}

	const ULocalPlayer* LocalPlayer = World->GetFirstLocalPlayerFromController();

	if (!LocalPlayer || !LocalPlayer->GetPreferredUniqueNetId().IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("NETWORK_SESSION: FindSessions failed. LocalPlayer or NetId is invalid."));

		NetworkOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
		OnFindSessionsCompleteBP.Broadcast(0, false);
		return;
	}

	// Registro il delegate interno. Quando Unreal/Steam termina la ricerca, verrà chiamata OnFindSessionsComplete
	FindSessionsCompleteDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegate);


	// Creo l'oggetto che contiene i parametri della ricerca.
	// Uso TSharedPtr perché la ricerca è asincrona e l'oggetto deve restare valido anche dopo la fine di questa funzione	 
	LastSessionSearch = MakeShareable(new FOnlineSessionSearch());


	// Numero massimo di risultati da cercare, 10000 nel nostro caso
	LastSessionSearch->MaxSearchResults = FMath::Max(MaxSearchResults, 10000);


	// Non sto cercando session LAN
	LastSessionSearch->bIsLanQuery = false;


	// Cerca sessioni basate su Lobby Steam.
	// Deve combaciare con bUseLobbiesIfAvailable = true e bUsesPresence = true impostati in CreateSession
	LastSessionSearch->QuerySettings.Set(
		FName(TEXT("LOBBYSEARCH")),
		true,
		EOnlineComparisonOp::Equals
	);

	
	// Chiede all'OnlineSubsystem di iniziare la ricerca.
	// Se ritorna true, la ricerca è partita. Il risultato finale arriverà in OnFindSessionsComplete.
	const bool bFindSessionsStarted = SessionInterface->FindSessions(
		*LocalPlayer->GetPreferredUniqueNetId(),
		LastSessionSearch.ToSharedRef()
	);

	UE_LOG(LogTemp, Warning, TEXT("NETWORK_SESSION: FindSessions started=%s | MaxSearchResults=%d"),
		bFindSessionsStarted ? TEXT("true") : TEXT("false"),
		LastSessionSearch->MaxSearchResults
	);

	// Se FindSessions ritorna false, la ricerca non è nemmeno partita. In questo caso stacco il delegate che notifica il fallimento
	if (!bFindSessionsStarted)
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);

		NetworkOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
		OnFindSessionsCompleteBP.Broadcast(0, false);
	}
}



// Avvia il tentativo di ingresso in una sessione trovata tramite OnlineSubsystem
void UNetworkSessionSubsystem::JoinSession(const FOnlineSessionSearchResult& SessionResult)
{
	// Se la SessionInterface non è valida
	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("NETWORK_SESSION: JoinSession failed. SessionInterface is invalid."));

		NetworkOnJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
		return;
	}

	// Recupera il World e il player locale che vuole entrare nella sessione. Steam deve sapere quale utente sta provando a fare il join.
	UWorld* World = GetWorld();

	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("NETWORK_SESSION: JoinSession failed. World is NULL."));

		NetworkOnJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
		return;
	}

	const ULocalPlayer* LocalPlayer = World->GetFirstLocalPlayerFromController();

	if (!LocalPlayer || !LocalPlayer->GetPreferredUniqueNetId().IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("NETWORK_SESSION: JoinSession failed. LocalPlayer or NetId is invalid."));

		NetworkOnJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
		return;
	}

	// Registro il delegate interno. Quando Unreal/Steam termina il tentativo di join, verrà chiamata OnJoinSessionComplete
	JoinSessionCompleteDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegate);

	/*
	 * Leggo alcune informazioni della sessione selezionata, utili per controllare nel log quale sessione sto provando a joinare
	 */
	FString FoundMatchType;
	SessionResult.Session.SessionSettings.Get(FName("MatchType"), FoundMatchType);

	UE_LOG(LogTemp, Warning, TEXT("NETWORK_SESSION: Join requested | Owner=%s | MatchType=%s | OpenConnections=%d | Ping=%d"),
		*SessionResult.Session.OwningUserName,
		*FoundMatchType,
		SessionResult.Session.NumOpenPublicConnections,
		SessionResult.PingInMs
	);

	// Chiede all'OnlineSubsystem di entrare nella sessione trovata.
	// Se ritorna true, il tentativo di join è partito ed il risultato finale arriverà in OnJoinSessionComplete
	const bool bJoinSessionStarted = SessionInterface->JoinSession(
		*LocalPlayer->GetPreferredUniqueNetId(),
		NAME_GameSession,
		SessionResult
	);

	UE_LOG(LogTemp, Warning, TEXT("NETWORK_SESSION: JoinSession started=%s"),
		bJoinSessionStarted ? TEXT("true") : TEXT("false")
	);

	// Se ritorna false, il tentativo di join non è nemmeno partito, quindi bisogna staccare il delegate e notificare il fallimento.
	if (!bJoinSessionStarted)
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);

		NetworkOnJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
	}
}



// Versione BP-friendly del join: riceve un index e prova ad entrare nella sessione corrispondente
void UNetworkSessionSubsystem::JoinSessionByIndex(int32 SessionIndex)
{
	// Controlla che esista una ricerca valida
	if (!LastSessionSearch.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("NETWORK_SESSION: JoinSessionByIndex failed. LastSessionSearch is invalid."));

		NetworkOnJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
		return;
	}

	// Controlla che l'indice ricevuto dal Blueprint sia valido
	if (!LastSessionSearch->SearchResults.IsValidIndex(SessionIndex))
	{
		UE_LOG(LogTemp, Error, TEXT("NETWORK_SESSION: JoinSessionByIndex failed. Invalid SessionIndex=%d | ResultsCount=%d"),
			SessionIndex,
			LastSessionSearch->SearchResults.Num()
		);

		NetworkOnJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
		return;
	}

	// Recupera la sessione selezionata dalla lista e la passa alla funzione C++ vera, che si occupa di chiamare JoinSession sull'OnlineSubsystem
	UE_LOG(LogTemp, Warning, TEXT("NETWORK_SESSION: JoinSessionByIndex | SessionIndex=%d"), SessionIndex);

	// Joina
	JoinSession(LastSessionSearch->SearchResults[SessionIndex]);
}



// Distrugge la sessione corrente associata a NAME_GameSession
void UNetworkSessionSubsystem::DestroySession()
{
	// Se la SessionInterface non è valida
	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("NETWORK_SESSION: DestroySession failed. SessionInterface is invalid."));

		NetworkOnDestroySessionComplete.Broadcast(false);
		return;
	}

	// Registro il delegate interno. Quando Unreal/Steam termina la distruzione della sessione, verrà chiamata OnDestroySessionComplete
	DestroySessionCompleteDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegate);

	// Chiedo all'OnlineSubsystem di distruggere la sessione principale di gioco.
	// Se ritorna true, la richiesta è partita. Il risultato finale arriverà in OnDestroySessionComplete.
	const bool bDestroySessionStarted = SessionInterface->DestroySession(NAME_GameSession);

	UE_LOG(LogTemp, Warning, TEXT("NETWORK_SESSION: DestroySession started=%s"),
		bDestroySessionStarted ? TEXT("true") : TEXT("false")
	);

	// Se DestroySession ritorna false, la richiesta non è nemmeno partita. quindi devo staccare il delegate e notificare il fallimento. 
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



// Callback chiamata quando la creazione della sessione termina. Se ha successo apre la mappa come ListenServer
void UNetworkSessionSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	// La creazione della sessione è terminata, scollego il delegate per evitare chiamate duplicate in futuro.
	if (SessionInterface)
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
	}

	UE_LOG(LogTemp, Warning, TEXT("NETWORK_SESSION: CreateSessionComplete | Session=%s | Success=%s"),
		*SessionName.ToString(),
		bWasSuccessful ? TEXT("true") : TEXT("false")
	);

	// Avvisa il resto del gioco che la creazione della sessione è terminata
	NetworkOnCreateSessionComplete.Broadcast(bWasSuccessful);

	// Se la sessione non è stata creata correttamente, non posso fare ServerTravel
	if (!bWasSuccessful)
	{
		UE_LOG(LogTemp, Error, TEXT("NETWORK_SESSION: CreateSession failed. ServerTravel aborted."));
		return;
	}

	
	// Controllo tecnico: verifico che la sessione esista davvero dopo la creazione.
	// Utile per debug e per controllare MatchType / connessioni disponibili.
	if (SessionInterface)
	{
		FNamedOnlineSession* ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession);

		if (ExistingSession)
		{
			FString SavedMatchType;
			ExistingSession->SessionSettings.Get(FName("MatchType"), SavedMatchType);

			UE_LOG(LogTemp, Warning, TEXT("NETWORK_SESSION: Host session exists | MatchType=%s | OpenPublicConnections=%d"),
				*SavedMatchType,
				ExistingSession->NumOpenPublicConnections
			);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("NETWORK_SESSION: Host session not found after CreateSessionComplete."));
		}
	}

	// ServerTravel apre la mappa come Listen Server.
	// listen significa che questa istanza diventa host/server e può accettare client.
	UWorld* World = GetWorld();

	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("NETWORK_SESSION: ServerTravel failed. World is NULL."));
		return;
	}

	World->ServerTravel(TEXT("/Game/ThirdPerson/Lvl_ThirdPerson?listen"));
}



// Callback chiamata quando il find delle sessioni termina. Filtra i risultati ed avvisa C++ e WBP
void UNetworkSessionSubsystem::OnFindSessionsComplete(bool bWasSuccessful)
{
	// La ricerca delle sessioni è terminata, scollego il delegate per evitare chiamate duplicate in futuro
	if (SessionInterface)
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
	}

	// Se LastSessionSearch non è valida, non ho risultati da leggere.
	// Avvisiamo sia il C++ sia il Blueprint che la ricerca è fallita.	 
	if (!LastSessionSearch.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("NETWORK_SESSION: FindSessionsComplete failed. LastSessionSearch is invalid."));

		NetworkOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
		OnFindSessionsCompleteBP.Broadcast(0, false);
		return;
	}

	const int32 RawResultsCount = LastSessionSearch->SearchResults.Num();

	TArray<FOnlineSessionSearchResult> FilteredResults;


	// Steam appid 480 è condiviso con tanti progetti di test. Per questo controllo ogni risultato e tengo solo le sessioni con lo stesso MatchType
	for (const FOnlineSessionSearchResult& Result : LastSessionSearch->SearchResults)
	{
		FString FoundMatchType;
		Result.Session.SessionSettings.Get(FName("MatchType"), FoundMatchType);

		UE_LOG(LogTemp, Warning, TEXT("NETWORK_SESSION: SearchResult | Owner=%s | MatchType=%s | OpenConnections=%d | Ping=%d"),
			*Result.Session.OwningUserName,
			*FoundMatchType,
			Result.Session.NumOpenPublicConnections,
			Result.PingInMs
		);

		if (FoundMatchType == FString(TEXT("TestCoop")))
		{
			FilteredResults.Add(Result);
		}
	}

	// Da questo momento LastSessionSearch contiene solo le sessioni valide
	LastSessionSearch->SearchResults = FilteredResults;

	const bool bHasValidResults = bWasSuccessful && FilteredResults.Num() > 0;

	UE_LOG(LogTemp, Warning, TEXT("NETWORK_SESSION: FindSessionsComplete | Success=%s | RawResults=%d | FilteredResults=%d"),
		bWasSuccessful ? TEXT("true") : TEXT("false"),
		RawResultsCount,
		FilteredResults.Num()
	);

	// Avvisa il codice C++ passando solo le sessioni filtrate.
	NetworkOnFindSessionsComplete.Broadcast(FilteredResults, bHasValidResults);

	// Avvisa il WBP
	OnFindSessionsCompleteBP.Broadcast(FilteredResults.Num(), bHasValidResults);
}



// Callback chiamata quando il join termina. Se ha successo, risolve la connect string ed esegue il ClientTravel
void UNetworkSessionSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	// Il tentativo di join è terminato, scollego il delegate per evitare chiamate duplicate in futuro
	if (SessionInterface)
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
	}

	UE_LOG(LogTemp, Warning, TEXT("NETWORK_SESSION: JoinSessionComplete | Session=%s | Result=%d"),
		*SessionName.ToString(),
		static_cast<int32>(Result)
	);

	// Avvisa il resto del codice che il tentativo di join è terminato
	NetworkOnJoinSessionComplete.Broadcast(Result);

	// Se il join non è riuscito, non effettua il travel
	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		UE_LOG(LogTemp, Error, TEXT("NETWORK_SESSION: Join failed. Result is not Success."));
		return;
	}

	// Controllo di sicurezza. Senza SessionInterface non posso recuperare la connect string
	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("NETWORK_SESSION: Join failed. SessionInterface is invalid."));
		return;
	}

	// Recupera l'indirizzo reale della sessione.
	// Con Steam non costruiamo l'indirizzo a mano: lo chiedo all'OnlineSubsystem.
	FString ConnectString;

	if (!SessionInterface->GetResolvedConnectString(SessionName, ConnectString))
	{
		UE_LOG(LogTemp, Error, TEXT("NETWORK_SESSION: GetResolvedConnectString failed."));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("NETWORK_SESSION: ClientTravel | ConnectString=%s"), *ConnectString);

	// Recupera il PlayerController locale. È il controller del client che deve viaggiare verso la sessione dell'host
	UWorld* World = GetWorld();

	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("NETWORK_SESSION: ClientTravel failed. World is NULL."));
		return;
	}

	APlayerController* PlayerController = World->GetFirstPlayerController();

	if (!PlayerController)
	{
		UE_LOG(LogTemp, Error, TEXT("NETWORK_SESSION: ClientTravel failed. PlayerController is NULL."));
		return;
	}

	// Sposta il client nella sessione dell'host. La ConnectString viene risolta dall'OnlineSubsystemSteam/SteamSockets
	PlayerController->ClientTravel(ConnectString, ETravelType::TRAVEL_Absolute);
}



// Callback chiamata quando la distruzione della sessione termina
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