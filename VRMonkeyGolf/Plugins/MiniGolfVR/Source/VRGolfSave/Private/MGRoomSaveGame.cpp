#include "MGRoomSaveGame.h"

FString UMGRoomSaveGame::MakeSlotName(const FString& InRoomName, const FString& InCourseName)
{
    // Sanitize — strip spaces to keep slot names filesystem-safe
    FString SafeRoom   = InRoomName.Replace(TEXT(" "), TEXT("_"));
    FString SafeCourse = InCourseName.Replace(TEXT(" "), TEXT("_"));
    return FString::Printf(TEXT("Room_%s_%s"), *SafeRoom, *SafeCourse);
}

int32 UMGRoomSaveGame::GetScore(const FGuid& PlayerID, int32 HoleIndex) const
{
    for (const FMGPlayerRoundData& Round : PlayerRounds)
    {
        if (Round.ProfileID == PlayerID)
        {
            if (Round.HoleScores.IsValidIndex(HoleIndex))
            {
                return Round.HoleScores[HoleIndex];
            }
            return -1;
        }
    }
    return -1;
}

void UMGRoomSaveGame::RecordScore(const FGuid& PlayerID, const FString& PlayerName,
                                   int32 HoleIndex, int32 Strokes)
{
    // Find or create player entry
    FMGPlayerRoundData* Round = PlayerRounds.FindByPredicate(
        [&PlayerID](const FMGPlayerRoundData& R){ return R.ProfileID == PlayerID; });

    if (!Round)
    {
        FMGPlayerRoundData NewRound;
        NewRound.ProfileID   = PlayerID;
        NewRound.ProfileName = PlayerName;
        NewRound.HoleScores.Init(-1, TotalHoles);
        PlayerRounds.Add(NewRound);
        Round = &PlayerRounds.Last();
    }

    if (!Round->HoleScores.IsValidIndex(HoleIndex))
    {
        Round->HoleScores.SetNum(HoleIndex + 1, false);
    }

    Round->HoleScores[HoleIndex] = Strokes;
    LastSaved = FDateTime::UtcNow();
}
