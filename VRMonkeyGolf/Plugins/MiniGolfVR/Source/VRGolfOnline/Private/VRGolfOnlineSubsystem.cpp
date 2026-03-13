#include "VRGolfOnlineSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

// ---------------------------------------------------------------------------
// Subsystem lifecycle
// ---------------------------------------------------------------------------

void UVRGolfOnlineSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void UVRGolfOnlineSubsystem::Deinitialize()
{
    if (bInRoom)
    {
        LeaveRoom();
    }

    // Unbind any lingering OSS delegates
    if (IOnlineSessionPtr Sessions = GetSessionInterface())
    {
        Sessions->ClearOnSessionParticipantLeftDelegate_Handle(OnSessionParticipantLeftHandle);
    }

    Super::Deinitialize();
}

// ---------------------------------------------------------------------------
// Auth
// ---------------------------------------------------------------------------

void UVRGolfOnlineSubsystem::Login(const FString& DisplayName)
{
    IOnlineIdentityPtr Identity = GetIdentityInterface();
    if (!Identity.IsValid())
    {
        OnLoginComplete.Broadcast(false, TEXT("EOS identity interface unavailable. Check OnlineSubsystemEOS plugin is enabled."));
        return;
    }

    // Already logged in — fire success immediately
    if (Identity->GetLoginStatus(0) == ELoginStatus::LoggedIn)
    {
        bLoggedIn = true;
        OnLoginComplete.Broadcast(true, TEXT(""));
        return;
    }

    // Device ID auth — no Epic account required, works on Quest + PCVR
    FOnlineAccountCredentials Creds;
    Creds.Type = TEXT("deviceid");
    Creds.Id = DisplayName;
    Creds.Token = TEXT("");

    OnLoginCompleteHandle = Identity->AddOnLoginCompleteDelegate_Handle(
        0, FOnLoginCompleteDelegate::CreateUObject(this, &UVRGolfOnlineSubsystem::HandleLoginComplete));

    Identity->Login(0, Creds);
}

void UVRGolfOnlineSubsystem::HandleLoginComplete(int32 LocalUserNum, bool bWasSuccessful,
    const FUniqueNetId& UserId, const FString& Error)
{
    if (IOnlineIdentityPtr Identity = GetIdentityInterface())
    {
        Identity->ClearOnLoginCompleteDelegate_Handle(0, OnLoginCompleteHandle);
    }

    bLoggedIn = bWasSuccessful;

    if (!bWasSuccessful)
    {
        UE_LOG(LogTemp, Error, TEXT("VRGolfOnline: EOS login failed — %s"), *Error);
    }

    OnLoginComplete.Broadcast(bWasSuccessful, Error);
}

// ---------------------------------------------------------------------------
// CreateOrJoinRoom — primary entry point
// ---------------------------------------------------------------------------

void UVRGolfOnlineSubsystem::CreateOrJoinRoom(const FString& RoomName)
{
    if (!bLoggedIn)
    {
        OnRoomJoined.Broadcast(false, TEXT("Not logged in. Call Login() first."));
        return;
    }

    if (bInRoom)
    {
        OnRoomJoined.Broadcast(false, TEXT("Already in a room. Call LeaveRoom() first."));
        return;
    }

    if (RoomName.IsEmpty())
    {
        OnRoomJoined.Broadcast(false, TEXT("Room name cannot be empty."));
        return;
    }

    PendingRoomName = RoomName;
    SearchForRoom(RoomName);
}

// ---------------------------------------------------------------------------
// Search
// ---------------------------------------------------------------------------

void UVRGolfOnlineSubsystem::SearchForRoom(const FString& RoomName)
{
    IOnlineSessionPtr Sessions = GetSessionInterface();
    if (!Sessions.IsValid())
    {
        OnRoomJoined.Broadcast(false, TEXT("Session interface unavailable."));
        return;
    }

    SessionSearch = MakeShared<FOnlineSessionSearch>();
    SessionSearch->MaxSearchResults = 10;
    SessionSearch->bIsLanQuery = false;

    // Filter by our room name attribute
    SessionSearch->QuerySettings.Set(
        FName(VRGOLF_ROOM_NAME_KEY),
        RoomName,
        EOnlineComparisonOp::Equals);

    OnFindSessionsCompleteHandle = Sessions->AddOnFindSessionsCompleteDelegate_Handle(
        FOnFindSessionsCompleteDelegate::CreateUObject(
            this, &UVRGolfOnlineSubsystem::HandleFindSessionsComplete));

    FUniqueNetIdPtr LocalId = GetLocalPlayerId();
    if (!LocalId.IsValid())
    {
        OnRoomJoined.Broadcast(false, TEXT("No valid local player ID."));
        return;
    }

    Sessions->FindSessions(*LocalId, SessionSearch.ToSharedRef());
}

void UVRGolfOnlineSubsystem::HandleFindSessionsComplete(bool bWasSuccessful)
{
    IOnlineSessionPtr Sessions = GetSessionInterface();
    if (Sessions.IsValid())
    {
        Sessions->ClearOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteHandle);
    }

    if (!bWasSuccessful || !SessionSearch.IsValid())
    {
        // Search failed — try creating instead (network may have hiccupped)
        UE_LOG(LogTemp, Warning, TEXT("VRGolfOnline: Session search failed, attempting create."));
        CreateRoom(PendingRoomName);
        return;
    }

    if (SessionSearch->SearchResults.Num() > 0)
    {
        // Room found — join the first result
        UE_LOG(LogTemp, Log, TEXT("VRGolfOnline: Found room '%s', joining."), *PendingRoomName);
        JoinFoundRoom(SessionSearch->SearchResults[0]);
    }
    else
    {
        // No room found — we are the host, create it
        UE_LOG(LogTemp, Log, TEXT("VRGolfOnline: No room '%s' found, creating as host."), *PendingRoomName);
        CreateRoom(PendingRoomName);
    }
}

// ---------------------------------------------------------------------------
// Create (host path)
// ---------------------------------------------------------------------------

void UVRGolfOnlineSubsystem::CreateRoom(const FString& RoomName)
{
    IOnlineSessionPtr Sessions = GetSessionInterface();
    if (!Sessions.IsValid())
    {
        OnRoomJoined.Broadcast(false, TEXT("Session interface unavailable."));
        return;
    }

    OnCreateSessionCompleteHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(
        FOnCreateSessionCompleteDelegate::CreateUObject(
            this, &UVRGolfOnlineSubsystem::HandleCreateSessionComplete));

    TSharedPtr<FOnlineSessionSettings> Settings = BuildHostSessionSettings(RoomName);
    FUniqueNetIdPtr LocalId = GetLocalPlayerId();
    if (!LocalId.IsValid())
    {
        OnRoomJoined.Broadcast(false, TEXT("No valid local player ID."));
        return;
    }

    Sessions->CreateSession(*LocalId, VRGOLF_SESSION_NAME, *Settings);
}

TSharedPtr<FOnlineSessionSettings> UVRGolfOnlineSubsystem::BuildHostSessionSettings(
    const FString& RoomName) const
{
    TSharedPtr<FOnlineSessionSettings> Settings = MakeShared<FOnlineSessionSettings>();

    Settings->NumPublicConnections = 4;
    Settings->NumPrivateConnections = 0;
    Settings->bAllowJoinInProgress = false;   // No late joining mid-round
    Settings->bIsLANMatch = false;
    Settings->bShouldAdvertise = true;
    Settings->bAllowJoinViaPresence = true;
    Settings->bUsesPresence = true;
    Settings->bUseLobbiesIfAvailable = true;   // Prefer EOS lobbies over sessions

    // Room name attribute — this is what clients search for
    Settings->Set(FName(VRGOLF_ROOM_NAME_KEY), RoomName,
        EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

    return Settings;
}

void UVRGolfOnlineSubsystem::HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
    IOnlineSessionPtr Sessions = GetSessionInterface();
    if (Sessions.IsValid())
    {
        Sessions->ClearOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteHandle);
    }

    if (!bWasSuccessful)
    {
        UE_LOG(LogTemp, Error, TEXT("VRGolfOnline: Failed to create session '%s'."), *PendingRoomName);
        OnRoomJoined.Broadcast(false, TEXT("Failed to create room."));
        return;
    }

    bInRoom = true;
    bIsHost = true;
    CurrentRoomName = PendingRoomName;

    // Bind participant removed — needed for host migration detection
    if (Sessions.IsValid())
    {
        OnSessionParticipantLeftHandle =
            Sessions->AddOnSessionParticipantLeftDelegate_Handle(
                FOnSessionParticipantLeftDelegate::CreateUObject(
                    this, &UVRGolfOnlineSubsystem::HandleSessionParticipantLeft));
    }

    UE_LOG(LogTemp, Log, TEXT("VRGolfOnline: Room '%s' created. Hosting as listen server."),
        *CurrentRoomName);

    // Start listen server — open menu level to network
    UWorld* World = GetGameInstance()->GetWorld();
    if (World)
    {
        // ?listen makes this a listen server. Clients will ClientTravel here.
        World->ServerTravel(TEXT("?listen"), false);
    }

    OnRoomJoined.Broadcast(true, TEXT(""));
}

// ---------------------------------------------------------------------------
// Join (client path)
// ---------------------------------------------------------------------------

void UVRGolfOnlineSubsystem::JoinFoundRoom(const FOnlineSessionSearchResult& SearchResult)
{
    IOnlineSessionPtr Sessions = GetSessionInterface();
    if (!Sessions.IsValid())
    {
        OnRoomJoined.Broadcast(false, TEXT("Session interface unavailable."));
        return;
    }

    OnJoinSessionCompleteHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(
        FOnJoinSessionCompleteDelegate::CreateUObject(
            this, &UVRGolfOnlineSubsystem::HandleJoinSessionComplete));

    FUniqueNetIdPtr LocalId = GetLocalPlayerId();
    if (!LocalId.IsValid())
    {
        OnRoomJoined.Broadcast(false, TEXT("No valid local player ID."));
        return;
    }

    Sessions->JoinSession(*LocalId, VRGOLF_SESSION_NAME, SearchResult);
}

void UVRGolfOnlineSubsystem::HandleJoinSessionComplete(FName SessionName,
    EOnJoinSessionCompleteResult::Type Result)
{
    IOnlineSessionPtr Sessions = GetSessionInterface();
    if (Sessions.IsValid())
    {
        Sessions->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteHandle);
    }

    if (Result != EOnJoinSessionCompleteResult::Success)
    {
        FString ErrorMsg = FString::Printf(TEXT("Failed to join room. Code: %d"), (int32)Result);
        UE_LOG(LogTemp, Error, TEXT("VRGolfOnline: %s"), *ErrorMsg);
        OnRoomJoined.Broadcast(false, ErrorMsg);
        return;
    }

    // Get connect URL from EOS (includes P2P relay address)
    FString ConnectURL;
    if (!Sessions.IsValid() || !Sessions->GetResolvedConnectString(VRGOLF_SESSION_NAME, ConnectURL))
    {
        OnRoomJoined.Broadcast(false, TEXT("Could not resolve host address."));
        return;
    }

    bInRoom = true;
    bIsHost = false;
    CurrentRoomName = PendingRoomName;

    UE_LOG(LogTemp, Log, TEXT("VRGolfOnline: Joining host at %s"), *ConnectURL);

    // Travel to host — EOS P2P relay handles NAT traversal transparently
    APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController();
    if (PC)
    {
        PC->ClientTravel(ConnectURL, TRAVEL_Absolute);
    }

    OnRoomJoined.Broadcast(true, TEXT(""));
}

// ---------------------------------------------------------------------------
// Leave
// ---------------------------------------------------------------------------

void UVRGolfOnlineSubsystem::LeaveRoom()
{
    if (!bInRoom) return;

    IOnlineSessionPtr Sessions = GetSessionInterface();
    if (!Sessions.IsValid())
    {
        bInRoom = false;
        bIsHost = false;
        CurrentRoomName.Empty();
        OnRoomLeft.Broadcast();
        return;
    }

    Sessions->ClearOnSessionParticipantLeftDelegate_Handle(OnSessionParticipantLeftHandle);

    OnDestroySessionCompleteHandle = Sessions->AddOnDestroySessionCompleteDelegate_Handle(
        FOnDestroySessionCompleteDelegate::CreateUObject(
            this, &UVRGolfOnlineSubsystem::HandleDestroySessionComplete));

    Sessions->DestroySession(VRGOLF_SESSION_NAME);
}

void UVRGolfOnlineSubsystem::HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
    IOnlineSessionPtr Sessions = GetSessionInterface();
    if (Sessions.IsValid())
    {
        Sessions->ClearOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteHandle);
    }

    bInRoom = false;
    bIsHost = false;
    CurrentRoomName.Empty();

    OnRoomLeft.Broadcast();
}

// ---------------------------------------------------------------------------
// Host migration
// ---------------------------------------------------------------------------

void UVRGolfOnlineSubsystem::HandleSessionParticipantLeft(FName SessionName,
    const FUniqueNetId& UniqueId,
    EOnSessionParticipantLeftReason LeaveReason)
{
    UE_LOG(LogTemp, Log, TEXT("VRGolfOnline: Participant left session '%s'. Reason: %s"),
        *SessionName.ToString(), ToLogString(LeaveReason));

    // Fire PlayerLeft delegate for UI and GameMode cleanup
    OnPlayerLeft.Broadcast(UniqueId.ToString());
}

// ---------------------------------------------------------------------------
// OSS helpers
// ---------------------------------------------------------------------------

IOnlineSubsystem* UVRGolfOnlineSubsystem::GetOSS() const
{
    return IOnlineSubsystem::Get(TEXT("EOS"));
}

IOnlineSessionPtr UVRGolfOnlineSubsystem::GetSessionInterface() const
{
    IOnlineSubsystem* OSS = GetOSS();
    return OSS ? OSS->GetSessionInterface() : nullptr;
}

IOnlineIdentityPtr UVRGolfOnlineSubsystem::GetIdentityInterface() const
{
    IOnlineSubsystem* OSS = GetOSS();
    return OSS ? OSS->GetIdentityInterface() : nullptr;
}

FUniqueNetIdPtr UVRGolfOnlineSubsystem::GetLocalPlayerId() const
{
    IOnlineIdentityPtr Identity = GetIdentityInterface();
    if (!Identity.IsValid()) return nullptr;
    return Identity->GetUniquePlayerId(0);
}