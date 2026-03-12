#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "MGRoomSaveGame.generated.h"

USTRUCT(BlueprintType)
struct VRGOLFSAVE_API FMGPlayerRoundData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Round")
    FString ProfileName;

    UPROPERTY(BlueprintReadWrite, Category = "Round")
    FGuid ProfileID;

    // Index = hole (0-based), value = strokes taken. -1 = not yet played.
    UPROPERTY(BlueprintReadWrite, Category = "Round")
    TArray<int32> HoleScores;
};

/**
 * Saved state for one room + course combination.
 * Slot: "Room_<RoomName>_<CourseName>"
 *
 * Each player saves this locally. Host arbitrates resume/restart on conflict.
 * Marked complete when all holes are finished — retained for scorecard history.
 */
UCLASS()
class VRGOLFSAVE_API UMGRoomSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, Category = "Room Save")
    FString RoomName;

    UPROPERTY(BlueprintReadWrite, Category = "Room Save")
    FString CourseName;

    UPROPERTY(BlueprintReadWrite, Category = "Room Save")
    int32 CurrentHole = 0;

    UPROPERTY(BlueprintReadWrite, Category = "Room Save")
    int32 TotalHoles = 18;

    UPROPERTY(BlueprintReadWrite, Category = "Room Save")
    bool bIsComplete = false;

    // UTC timestamp of last save — used for conflict resolution display
    UPROPERTY(BlueprintReadWrite, Category = "Room Save")
    FDateTime LastSaved;

    UPROPERTY(BlueprintReadWrite, Category = "Room Save")
    TArray<FMGPlayerRoundData> PlayerRounds;

    // --- Helpers ---

    /** Returns true if a save exists for this room + course combo. */
    UFUNCTION(BlueprintPure, Category = "Room Save")
    bool IsInProgress() const { return CurrentHole > 0 && !bIsComplete; }

    /** Slot name convention. Static so subsystem can call without an instance. */
    static FString MakeSlotName(const FString& InRoomName, const FString& InCourseName);

    /** Returns score for a player on a specific hole. -1 if not yet played. */
    UFUNCTION(BlueprintPure, Category = "Room Save")
    int32 GetScore(const FGuid& PlayerID, int32 HoleIndex) const;

    /** Update or add a player's score for a hole. Creates player entry if missing. */
    UFUNCTION(BlueprintCallable, Category = "Room Save")
    void RecordScore(const FGuid& PlayerID, const FString& PlayerName,
                     int32 HoleIndex, int32 Strokes);
};
