#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MGProfileSaveGame.h"
#include "MGRoomSaveGame.h"
#include "MGProfileSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProfileLoaded, UMGProfileSaveGame*, Profile);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProfileSaved,  UMGProfileSaveGame*, Profile);

/**
 * GameInstance subsystem — lives for the lifetime of the game session.
 * Single source of truth for the active profile and room save operations.
 *
 * Usage:
 *   UMGProfileSubsystem* PS = GameInstance->GetSubsystem<UMGProfileSubsystem>();
 *   PS->LoadOrCreateProfile("Profile_Default");
 */
UCLASS()
class VRGOLFSAVE_API UMGProfileSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    // --- Active profile access ---

    UFUNCTION(BlueprintPure, Category = "Profile")
    UMGProfileSaveGame* GetActiveProfile() const { return ActiveProfile; }

    UFUNCTION(BlueprintPure, Category = "Profile")
    bool HasActiveProfile() const { return ActiveProfile != nullptr; }

    // --- Load / Create ---

    /**
     * Load profile from slot, or create a new one if slot is empty.
     * Sets as active profile on success.
     * @param SlotName      Save slot name, e.g. "Profile_Default"
     * @param FallbackName  Display name used only when creating a new profile.
     */
    UFUNCTION(BlueprintCallable, Category = "Profile")
    void LoadOrCreateProfile(const FString& SlotName, const FString& FallbackName = TEXT("Player"));

    /** Save the active profile to its slot. No-op if no active profile. */
    UFUNCTION(BlueprintCallable, Category = "Profile")
    void SaveActiveProfile();

    /**
     * Create a brand new profile, save it, and set as active.
     * Used by the "New Profile" UI path.
     */
    UFUNCTION(BlueprintCallable, Category = "Profile")
    void CreateNewProfile(const FString& ProfileName);

    /** Returns all existing profile slot names found on disk. */
    UFUNCTION(BlueprintCallable, Category = "Profile")
    TArray<FString> GetAllProfileSlotNames() const;

    // --- Room save ---

    /** Load room save if it exists. Returns null if no save found. */
    UFUNCTION(BlueprintCallable, Category = "Room Save")
    UMGRoomSaveGame* LoadRoomSave(const FString& RoomName, const FString& CourseName);

    /** Create a fresh room save. Does not write to disk yet. */
    UFUNCTION(BlueprintCallable, Category = "Room Save")
    UMGRoomSaveGame* CreateRoomSave(const FString& RoomName, const FString& CourseName,
                                    int32 TotalHoles = 18);

    /** Write current room save to disk. Call after every hole completes. */
    UFUNCTION(BlueprintCallable, Category = "Room Save")
    void SaveRoomProgress(UMGRoomSaveGame* RoomSave);

    /** Mark complete and do a final save. Call when last hole is finished. */
    UFUNCTION(BlueprintCallable, Category = "Room Save")
    void CompleteRoomSave(UMGRoomSaveGame* RoomSave);

    // --- Delegates ---

    UPROPERTY(BlueprintAssignable, Category = "Profile")
    FOnProfileLoaded OnProfileLoaded;

    UPROPERTY(BlueprintAssignable, Category = "Profile")
    FOnProfileSaved OnProfileSaved;

    // --- Subsystem interface ---
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

private:
    UPROPERTY()
    TObjectPtr<UMGProfileSaveGame> ActiveProfile;

    static FString MakeProfileSlotName(const FGuid& ID);
    static constexpr int32 UserIndex  = 0;
    static constexpr int32 MaxProfiles = 8;
};
