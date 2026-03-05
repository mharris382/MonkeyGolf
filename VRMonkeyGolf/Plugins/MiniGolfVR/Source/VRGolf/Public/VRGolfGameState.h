// VRGolfGameState.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "VRGolfGameState.generated.h"

/**
 * Game state for VR Golf - tracks overall game progress
 * Replicated to all clients so everyone sees the same game state
 */
UCLASS()
class VRGOLF_API AVRGolfGameState : public AGameState
{
    GENERATED_BODY()

public:
    AVRGolfGameState();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // Current game state
    UPROPERTY(ReplicatedUsing = OnRep_CurrentHole, BlueprintReadOnly, Category = "Golf")
    int32 CurrentHoleNumber;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Golf")
    int32 TotalHoles;

    UPROPERTY(ReplicatedUsing = OnRep_CurrentTurn, BlueprintReadOnly, Category = "Golf")
    class AVRGolfPlayerState* CurrentTurnPlayer;

    // Queries
    UFUNCTION(BlueprintPure, Category = "Golf")
    TArray<class AVRGolfPlayerState*> GetSortedPlayersByScore() const;

    // Replication callbacks
    UFUNCTION()
    void OnRep_CurrentHole();

    UFUNCTION()
    void OnRep_CurrentTurn();
};
