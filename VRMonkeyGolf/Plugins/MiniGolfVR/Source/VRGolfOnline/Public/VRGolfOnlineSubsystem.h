#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OnlineSubsystem.h"
#include "Delegates/DelegateCombinations.h"
#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "VRGolfOnlineSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnVRGolfLoginComplete, bool, bSuccess, const FString&, ErrorMsg);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnVRGolfRoomJoined, bool, bSuccess, const FString&, ErrorMsg);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnVRGolfRoomLeft);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVRGolfPlayerJoined, const FString&, PlayerName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVRGolfPlayerLeft, const FString&, PlayerName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnVRGolfHostMigrated);

/** Session attribute key used to match rooms by name. */
#define VRGOLF_ROOM_NAME_KEY TEXT("RoomName")

/** Session name used with UE's OSS session interface. */
#define VRGOLF_SESSION_NAME NAME_GameSession

/**
 * GameInstance subsystem — single entry point for all EOS session logic.
 *
 * Core API is intentionally one call: CreateOrJoinRoom(RoomName).
 * Internally searches for an existing EOS session matching RoomName.
 * If found → joins and ClientTravels to host.
 * If not found → creates as listen server host.
 *
 * UI never touches OSS directly.
 */
UCLASS()
class VRGOLFONLINE_API UVRGolfOnlineSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    // --- Auth ---

    /**
     * Login via EOS Device ID. No Epic account required — works on Quest + PCVR.
     * Must succeed before CreateOrJoinRoom. Call after active profile is loaded.
     * @param DisplayName   Profile name shown to other players.
     */
    UFUNCTION(BlueprintCallable, Category = "VRGolf Online")
    void Login(const FString& DisplayName);

    UFUNCTION(BlueprintPure, Category = "VRGolf Online")
    bool IsLoggedIn() const { return bLoggedIn; }

    // --- Room ---

    /**
     * Primary multiplayer entry point.
     * Searches for a session with RoomName. Joins if found, creates if not.
     * Bind OnVRGolfRoomJoined before calling.
     */
    UFUNCTION(BlueprintCallable, Category = "VRGolf Online")
    void CreateOrJoinRoom(const FString& RoomName);

    /** Leave the current room. Safe to call as host or client. */
    UFUNCTION(BlueprintCallable, Category = "VRGolf Online")
    void LeaveRoom();

    UFUNCTION(BlueprintPure, Category = "VRGolf Online")
    bool IsInRoom() const { return bInRoom; }

    UFUNCTION(BlueprintPure, Category = "VRGolf Online")
    bool IsHost() const { return bIsHost; }

    UFUNCTION(BlueprintPure, Category = "VRGolf Online")
    FString GetCurrentRoomName() const { return CurrentRoomName; }

    // --- Delegates ---

    /** Fires after Login() completes. */
    UPROPERTY(BlueprintAssignable, Category = "VRGolf Online")
    FOnVRGolfLoginComplete OnLoginComplete;

    /** Fires after CreateOrJoinRoom() resolves — success or failure. */
    UPROPERTY(BlueprintAssignable, Category = "VRGolf Online")
    FOnVRGolfRoomJoined OnRoomJoined;

    /** Fires when we leave or are disconnected from a room. */
    UPROPERTY(BlueprintAssignable, Category = "VRGolf Online")
    FOnVRGolfRoomLeft OnRoomLeft;

    /** Fires on host when a new player connects. */
    UPROPERTY(BlueprintAssignable, Category = "VRGolf Online")
    FOnVRGolfPlayerJoined OnPlayerJoined;

    /** Fires when a player disconnects. */
    UPROPERTY(BlueprintAssignable, Category = "VRGolf Online")
    FOnVRGolfPlayerLeft OnPlayerLeft;

    /** Fires on clients when host migration completes. */
    UPROPERTY(BlueprintAssignable, Category = "VRGolf Online")
    FOnVRGolfHostMigrated OnHostMigrated;

    // --- Subsystem interface ---
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

private:
    // --- State ---
    bool bLoggedIn = false;
    bool bInRoom = false;
    bool bIsHost = false;
    FString CurrentRoomName;
    FString PendingRoomName;    // Stored during async search

    // --- OSS helpers ---
    IOnlineSubsystem* GetOSS() const;
    IOnlineSessionPtr       GetSessionInterface() const;
    IOnlineIdentityPtr      GetIdentityInterface() const;
    FUniqueNetIdPtr         GetLocalPlayerId() const;

    // --- Internal flow ---
    void SearchForRoom(const FString& RoomName);
    void CreateRoom(const FString& RoomName);
    void JoinFoundRoom(const FOnlineSessionSearchResult& SearchResult);

    // --- OSS delegate handles ---
    FDelegateHandle OnLoginCompleteHandle;
    FDelegateHandle OnCreateSessionCompleteHandle;
    FDelegateHandle OnFindSessionsCompleteHandle;
    FDelegateHandle OnJoinSessionCompleteHandle;
    FDelegateHandle OnDestroySessionCompleteHandle;
    FDelegateHandle OnSessionParticipantLeftHandle;

    // --- OSS callbacks ---
    void HandleLoginComplete(int32 LocalUserNum, bool bWasSuccessful,
        const FUniqueNetId& UserId, const FString& Error);
    void HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful);
    void HandleFindSessionsComplete(bool bWasSuccessful);
    void HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
    void HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful);
    void HandleSessionParticipantLeft(FName SessionName, const FUniqueNetId& UniqueId, EOnSessionParticipantLeftReason LeaveReason);

    // --- Session search ---
    TSharedPtr<FOnlineSessionSearch> SessionSearch;

    // --- Settings builder ---
    TSharedPtr<FOnlineSessionSettings> BuildHostSessionSettings(const FString& RoomName) const;
};