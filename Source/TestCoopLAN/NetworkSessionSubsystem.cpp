// Fill out your copyright notice in the Description page of Project Settings.


#include "NetworkSessionSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"


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
	// Salvo un dato custom dentro la sessione, in futuro serve per cercare solo sessioni con MatchType uguale a quello che vogliamo.
	LastSessionSettings->Set(FName("MatchType"), MatchType,	EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);





}

void UNetworkSessionSubsystem::FindSessions(int32 MaxSearchResults)
{

}

void UNetworkSessionSubsystem::JoinSession(const FOnlineSessionSearchResult& SessionResult)
{

}

void UNetworkSessionSubsystem::DestroySession()
{

}


void UNetworkSessionSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{

}

void UNetworkSessionSubsystem::OnFindSessionsComplete(bool bWasSuccessful)
{

}

void UNetworkSessionSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{

}

void UNetworkSessionSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{

}