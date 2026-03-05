// VRGolfSettings.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "VRGolfSettings.generated.h"

/**
 * Project-wide VR Golf settings
 * Access in C++: GetDefault<UVRGolfSettings>()
 * Access in BP: Get VR Golf Settings node
 */
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "VR Golf Settings"))
class VRGOLF_API UVRGolfSettings : public UDeveloperSettings
{
    GENERATED_BODY()
    
public:
    UVRGolfSettings();

    // ========================================
    // SURFACE DATABASE
    // ========================================
    
    /**
     * DataTable mapping EPhysicalSurface to golf gameplay behavior
     * Row type: FGolfingSurfaceMappingEntry
     */
    UPROPERTY(config, EditAnywhere, Category = "Surfaces", 
              meta = (RequiredAssetDataTags = "RowStructure=/Script/VRGolf.GolfingSurfaceMappingEntry"))
    TSoftObjectPtr<UDataTable> SurfaceDatabase;

    // ========================================
    // BALL PHYSICS - STOPPING BEHAVIOR
    // ========================================
    
    /**
     * Minimum linear velocity (cm/s) for ball to be considered "stopped"
     * Lower = more sensitive, ball stops faster
     * Walkabout estimate: ~1.0
     */
    UPROPERTY(config, EditAnywhere, Category = "Ball Physics|Stopping", 
              meta = (UIMin = "0.1", UIMax = "10.0", ClampMin = "0.01"))
    float StoppingVelocityThreshold = 1.0f;

    /**
     * Minimum angular velocity (rad/s) for ball to be considered "stopped"
     * Controls when rolling motion ends
     * Walkabout estimate: ~0.1
     */
    UPROPERTY(config, EditAnywhere, Category = "Ball Physics|Stopping", 
              meta = (UIMin = "0.01", UIMax = "1.0", ClampMin = "0.001"))
    float StoppingAngularVelocityThreshold = 0.1f;

    // ========================================
    // BALL PHYSICS - RESET CONDITIONS
    // ========================================
    
    /**
     * Maximum time ball can be airborne before auto-reset (seconds)
     * Prevents infinitely falling balls
     * Can be disabled per-shot by holding trigger (trick shot mode)
     */
    UPROPERTY(config, EditAnywhere, Category = "Ball Physics|Reset", 
              meta = (UIMin = "5.0", UIMax = "30.0"))
    float MaxAirborneTime = 10.0f;

    /**
     * How often to check if ball is stuck (seconds)
     * Stuck = has velocity but not moving
     */
    UPROPERTY(config, EditAnywhere, Category = "Ball Physics|Reset", 
              meta = (UIMin = "0.5", UIMax = "5.0"))
    float StuckCheckInterval = 2.0f;

    /**
     * Minimum distance ball must move to not be considered stuck (cm)
     * Checked every StuckCheckInterval
     */
    UPROPERTY(config, EditAnywhere, Category = "Ball Physics|Reset", 
              meta = (UIMin = "1.0", UIMax = "20.0"))
    float StuckMovementThreshold = 5.0f;

    // ========================================
    // BALL PHYSICS - PHYSICAL PROPERTIES
    // ========================================
    
    /**
     * Ball radius in centimeters
     * Real golf ball: 2.135 cm (4.27 cm diameter)
     */
    UPROPERTY(config, EditAnywhere, Category = "Ball Physics|Dimensions", 
              meta = (UIMin = "1.0", UIMax = "5.0"))
    float BallRadius = 2.135f;

    /**
     * Ball mass in kilograms
     * Real golf ball: 0.04593 kg (45.93 grams)
     */
    UPROPERTY(config, EditAnywhere, Category = "Ball Physics|Dimensions", 
              meta = (UIMin = "0.01", UIMax = "0.1"))
    float BallMass = 0.04593f;

    /**
     * Linear damping - how quickly ball slows in air
     * Higher = more air resistance
     */
    UPROPERTY(config, EditAnywhere, Category = "Ball Physics|Damping", 
              meta = (UIMin = "0.0", UIMax = "1.0"))
    float BallLinearDamping = 0.2f;

    /**
     * Angular damping - how quickly ball stops spinning
     * Higher = spin decays faster
     */
    UPROPERTY(config, EditAnywhere, Category = "Ball Physics|Damping", 
              meta = (UIMin = "0.0", UIMax = "1.0"))
    float BallAngularDamping = 0.3f;

    // ========================================
    // GHOST BALL SETTINGS
    // ========================================
    
    /**
     * Enable ghost balls (practice shots) globally
     */
    UPROPERTY(config, EditAnywhere, Category = "Ghost Ball")
    bool bAllowGhostBalls = true;

    /**
     * Allow ghost balls in multiplayer games
     * If false, ghost balls only work in single player
     */
    UPROPERTY(config, EditAnywhere, Category = "Ghost Ball", 
              meta = (EditCondition = "bAllowGhostBalls"))
    bool bAllowGhostBallsInMultiplayer = true;

    /**
     * Maximum number of ghost balls per hole
     * -1 = unlimited
     * 0 = disabled
     * >0 = limited uses (e.g., 3 practice shots per hole)
     */
    UPROPERTY(config, EditAnywhere, Category = "Ghost Ball", 
              meta = (EditCondition = "bAllowGhostBalls", UIMin = "-1", UIMax = "10"))
    int32 MaxGhostBallsPerHole = -1;

    /**
     * How long ghost ball exists before fading out (seconds)
     * Prevents using ghost balls to fully map out routes
     */
    UPROPERTY(config, EditAnywhere, Category = "Ghost Ball", 
              meta = (EditCondition = "bAllowGhostBalls", UIMin = "1.0", UIMax = "10.0"))
    float GhostBallMaxLifetime = 5.0f;

    /**
     * When ghost ball starts fading (seconds)
     * Should be less than MaxLifetime
     */
    UPROPERTY(config, EditAnywhere, Category = "Ghost Ball", 
              meta = (EditCondition = "bAllowGhostBalls", UIMin = "0.5", UIMax = "8.0"))
    float GhostBallFadeStartTime = 3.0f;

    /**
     * Ghost ball opacity (0.0 - 1.0)
     * Lower = more transparent
     */
    UPROPERTY(config, EditAnywhere, Category = "Ghost Ball", 
              meta = (EditCondition = "bAllowGhostBalls", UIMin = "0.1", UIMax = "1.0"))
    float GhostBallOpacity = 0.5f;

    // ========================================
    // GAMEPLAY RULES
    // ========================================
    
    /**
     * Default number of holes in a round
     */
    UPROPERTY(config, EditAnywhere, Category = "Gameplay|Rules", 
              meta = (UIMin = "1", UIMax = "18"))
    int32 DefaultTotalHoles = 18;

    /**
     * Maximum strokes allowed per hole before automatic completion
     * -1 = unlimited
     * >0 = stroke limit (e.g., 10 max)
     */
    UPROPERTY(config, EditAnywhere, Category = "Gameplay|Rules", 
              meta = (UIMin = "-1", UIMax = "20"))
    int32 MaxStrokesPerHole = -1;

    /**
     * Allow simultaneous play (everyone plays at once)
     * If false, uses turn-based play
     */
    UPROPERTY(config, EditAnywhere, Category = "Gameplay|Multiplayer")
    bool bAllowSimultaneousPlay = false;

    // ========================================
    // TELEPORT SETTINGS
    // ========================================
    
    /**
     * Distance behind ball to teleport player (cm)
     * Player will be positioned this far back, facing the target
     */
    UPROPERTY(config, EditAnywhere, Category = "Teleport", 
              meta = (UIMin = "50.0", UIMax = "200.0"))
    float TeleportDistanceBehindBall = 100.0f;

    /**
     * Auto-teleport to ball after each stroke
     * If false, player must manually teleport
     */
    UPROPERTY(config, EditAnywhere, Category = "Teleport")
    bool bAutoTeleportAfterStroke = false;

    // ========================================
    // DEBUG SETTINGS
    // ========================================
    
    /**
     * Show debug visualization for ball state
     * (Velocity, state, stroke count displayed above ball)
     */
    UPROPERTY(config, EditAnywhere, Category = "Debug")
    bool bShowBallDebugInfo = false;

    /**
     * Draw debug lines for putter swing detection
     */
    UPROPERTY(config, EditAnywhere, Category = "Debug")
    bool bShowPutterDebug = false;

    /**
     * Show ground projection ray for putter
     */
    UPROPERTY(config, EditAnywhere, Category = "Debug")
    bool bShowGroundProjection = false;

#if WITH_EDITOR
    virtual FText GetSectionText() const override
    {
        return NSLOCTEXT("VRGolf", "VRGolfSettingsSection", "VR Golf");
    }

    virtual FText GetSectionDescription() const override
    {
        return NSLOCTEXT("VRGolf", "VRGolfSettingsDescription",
            "Configure global settings for VR Mini Golf gameplay, physics, and multiplayer rules.");
    }

    virtual FName GetCategoryName() const override
    {
        return TEXT("Game");
    }
#endif
};
