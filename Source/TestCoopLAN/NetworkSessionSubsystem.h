// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"		// Serve per usare il sistema Sessioni di Unreal
#include "Engine/EngineBaseTypes.h"
#include "Delegates/DelegateCombinations.h"
#include "NetworkSessionSubsystem.generated.h"


class UWorld;
class UNetDriver;


/* -- Delegate Custom -- 
	Sono delegate custom pubblici per avvisare il resto del gioco quando qualcosa finisce.
	Servono per menu/UI/altre classi per essere avvisati dal nostro subsytem
*/

DECLARE_MULTICAST_DELEGATE_OneParam(FNetworkOnCreateSessionComplete, bool /*bWasSuccessful*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FNetworkOnFindSessionsComplete, const TArray<FOnlineSessionSearchResult>& /*SessionResults*/, bool /*bWasSuccessful*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FNetworkOnJoinSessionComplete, EOnJoinSessionCompleteResult::Type /*Result*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FNetworkOnDestroySessionComplete, bool /*bWasSuccessful*/);

// Delegate esposto ai Blueprint per aggiornare la UI quando termina la ricerca delle sessioni.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FNetworkOnFindSessionsCompleteBP, int32, ResultsCount, bool, bWasSuccessful);
// Delegate per Create Session
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNetworkOnCreateSessionCompleteBP, bool, bWasSuccesful);
// Delegate per Join Session
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FNetworkOnJoinSessionCompleteBP, bool, bWasSuccessful, int32, ResultCode);
// Delegate per Destroy Session
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNetworkOnDestroySessionCompleteBP, bool, bWasSuccesful);
// Delegate che trasmette il tipo di errore e la descrizione tecnica dell'errore
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FNetworkOnNetworkFailureBP, FString, FailureType, FString, ErrorString);


UCLASS()
class TESTCOOPLAN_API UNetworkSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	// Costruttore
	UNetworkSessionSubsystem();

	// UFUNCTION(BlueprintCallable) serve per rendere il subsystem chiamabile dai BP

	// Serve all'Host per creare una sessione.
	// NumPublicConnections = numero di player che possono entrare
	// MatchType = stringa per distinguire il tipo di partita.
	UFUNCTION(BlueprintCallable)
	void CreateSession(int32 NumPublicConnections, FString MatchType);
	
	// Serve ai client per cercare partite disponibili
	UFUNCTION(BlueprintCallable)
	void FindSessions(int32 MaxSearchResults);
	
	// Serve per entrare in una sessione trovata
	// FOnlineSessionSearchResult rappresenta una partita trovata dalla ricerca	
	void JoinSession(const FOnlineSessionSearchResult& SessionResult);

	// Versione Blueprint friendly di JoinSession
	// Passiamo solo l'indice della sessione trovata nella ricerca
	UFUNCTION(BlueprintCallable)
	void JoinSessionByIndex(int32 SessionIndex);
	
	// Chiude la sessione corrente
	UFUNCTION(BlueprintCallable)
	void DestroySession();

	// Delegate Blueprint per notificare la UI quando FindSessions termina.
	// BlueprintAssignable : visibile nel BP
	UPROPERTY(BlueprintAssignable, Category = "Network Sessions")
	FNetworkOnFindSessionsCompleteBP OnFindSessionsCompleteBP;

	UPROPERTY(BlueprintAssignable, Category = "Network Sessions")
	FNetworkOnCreateSessionCompleteBP OnCreateSessionCompleteBP;

	UPROPERTY(BlueprintAssignable, Category = "Network Sessions")
	FNetworkOnJoinSessionCompleteBP OnJoinSessionCompleteBP;

	UPROPERTY(BlueprintAssignable, Category = "Network Sessions")
	FNetworkOnDestroySessionCompleteBP OnDestroySessionCompleteBP;

	UPROPERTY(BlueprintAssignable, Category = "Network Sessions")
	FNetworkOnNetworkFailureBP OnNetworkFailureBP;


	/* Funzioni per i BP */

	// Restistuisce quante sessioni sono state trovate dall'ultima FindSessions.
	UFUNCTION(BlueprintCallable)
	int32 GetSessionSearchResultsCount() const;

	// Restituisce il nome leggibile per una sessione trovata. Serve al Widget per mostrare una lista delle sessioni disponibili
	UFUNCTION(BlueprintCallable)
	FString GetSessionSearchResultName(int32 SessionIndex) const;

	// Funzioni per creare la lista nel WBP
	UFUNCTION(BlueprintCallable, Category = "Network Sessions")
	FString GetSessionDisplayName(int32 SessionIndex) const;

	UFUNCTION(BlueprintCallable, Category = "Network Sessions")
	FString GetSessionHostName(int32 SessionIndex) const;

	UFUNCTION(BlueprintCallable, Category = "Network Sessions")
	int32 GetSessionPing(int32 SessionIndex) const;


	/* Variabili pubbliche per i Delegate Custom */
	FNetworkOnCreateSessionComplete NetworkOnCreateSessionComplete;
	FNetworkOnFindSessionsComplete NetworkOnFindSessionsComplete;
	FNetworkOnJoinSessionComplete NetworkOnJoinSessionComplete;
	FNetworkOnDestroySessionComplete NetworkOnDestroySessionComplete;


	// Funzioni per leggere il tipo di errore
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;


private:

	// Unreal usa questa variabile per parlare con OnlineSubsystemSteam, quindi possiamo chiamare le funzioni sopra.
	IOnlineSessionPtr SessionInterface;


	/* -- Callback interne -- */

	// Viene chiamata quando Unreal ha finito di creare la sessione
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);

	// Viene chiamata quando la ricerca della sessione è finita
	void OnFindSessionsComplete(bool bWasSuccessful);
	
	// Viene chiamata quando il client ha provato ad entrare in una sessione
	// Result indica se è il join è andato bene oppure no
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	
	// Viene chiamato quando una sessione viene distrutta
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);



	/* -- Delegate --
	 sono il collegamento tra operazione asincrona online e callback da chiamare quando finisce.
	 Servono perchè, dato che le funzioni online non finiscono subito, Steam deve parlare con i server Steam.
	 cercando lobby/sessioni, rispondere, ecc... Quindi Unreal usa un sistema asincrono:
	 Io chiedo "Create Session", Unreal/Steam lavorano, quando finisce viene chiamato il delegate
	*/

	// Quando CreateSession finisce, voglio poter collegaer una funzione da chiamare
	FOnCreateSessionCompleteDelegate CreateSessionCompleteDelegate;
	
	FOnFindSessionsCompleteDelegate FindSessionsCompleteDelegate;
	
	FOnJoinSessionCompleteDelegate JoinSessionCompleteDelegate;
	
	FOnDestroySessionCompleteDelegate DestroySessionCompleteDelegate;


	/* -- Handle -- 
		il delegate è il collegamento, l'handle è il "ticket" di quel collegamento.
		Serve per poterlo rimuovere dopo.
		è importante perchè serve per evitare che una callback venga chiamata più volte o resti agganciata quando non serve più.
		Delegate = campanello collegato alla callback
		Handle = chiave per staccare quel campanello
		Senza Handle rischio di accumulare collegamenti nel tempo
	*/

	FDelegateHandle CreateSessionCompleteDelegateHandle;
	
	FDelegateHandle FindSessionsCompleteDelegateHandle;
	
	FDelegateHandle JoinSessionCompleteDelegateHandle;
	
	FDelegateHandle DestroySessionCompleteDelegateHandle;



	/* -- Settings and Search -- */

	// Questo conterrà le impostazioni dell'ultima sessione creata
	// TSharedPtr perchè sono operazioni asincrone
	TSharedPtr<FOnlineSessionSettings> LastSessionSettings;

	// Questo conterrà i dati dell'ultima ricerca di sessioni
	TSharedPtr<FOnlineSessionSearch> LastSessionSearch;



	void HandleNetworkFailure(
		UWorld* World,
		UNetDriver* NetDriver,
		ENetworkFailure::Type FailureType,
		const FString& ErrorString
	);

	FDelegateHandle NetworkFailureDelegateHandle;

};
