#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "MGProfileSaveGame.generated.h"

/**
 * One save slot = one player profile.
 * Slot name convention: "Profile_<ProfileID>"
 * Intentionally minimal — just enough to give multiplayer a stable per-player identity.
 */
UCLASS()
class VRGOLFSAVE_API UMGProfileSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    /** Display name shown in UI and to other players. */
    UPROPERTY(BlueprintReadWrite, Category = "Profile")
    FString ProfileName;

    /** Stable unique identity — generated once on profile creation, never changes. */
    UPROPERTY(BlueprintReadOnly, Category = "Profile")
    FGuid ProfileID;

    /** Slot name this profile was loaded from. Set by UMGProfileSubsystem on load. */
    UPROPERTY(BlueprintReadOnly, Category = "Profile")
    FString SlotName;

    /** True if this profile has been saved at least once. */
    UPROPERTY(BlueprintReadOnly, Category = "Profile")
    bool bIsPersisted = false;

    /** Last room name entered — pre-fills the keyboard on next session. */
    UPROPERTY(BlueprintReadWrite, Category = "Profile")
    FString LastUsedRoomName;
};
