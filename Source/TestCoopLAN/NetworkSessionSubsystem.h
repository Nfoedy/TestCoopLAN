// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"		// Serve per usare il sistema Sessioni di Unreal
#include "Delegates/DelegateCombinations.h"
#include "NetworkSessionSubsystem.generated.h"

/**
 * 
 */


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
	UPROPERTY(BlueprintAssignable, Category = "Network Sessions")
	FNetworkOnFindSessionsCompleteBP OnFindSessionsCompleteBP;


	/* Funzioni per i BP */

	// Restistuisce quante sessioni sono state trovate dall'ultima FindSessions.
	UFUNCTION(BlueprintCallable)
	int32 GetSessionSearchResultsCount() const;

	// Restituisce il nome leggibile per una sessione trovata. Serve al Widget per mostrare una lista delle sessioni disponibili
	UFUNCTION(BlueprintCallable)
	FString GetSessionSearchResultName(int32 SessionIndex) const;


	/* Variabili pubbliche per i Delegate Custom */
	FNetworkOnCreateSessionComplete NetworkOnCreateSessionComplete;
	FNetworkOnFindSessionsComplete NetworkOnFindSessionsComplete;
	FNetworkOnJoinSessionComplete NetworkOnJoinSessionComplete;
	FNetworkOnDestroySessionComplete NetworkOnDestroySessionComplete;


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

};
