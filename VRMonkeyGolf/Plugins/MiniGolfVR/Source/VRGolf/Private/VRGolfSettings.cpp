// VRGolfSettings.cpp

#include "VRGolfSettings.h"

UVRGolfSettings::UVRGolfSettings()
{
    // Set category for Project Settings UI
    CategoryName = TEXT("Game");
    SectionName = TEXT("VR Golf Settings");

    // Initialize default values (these match the defaults in header, but explicit is good)
    
    // Ball Physics - Stopping
    StoppingVelocityThreshold = 1.0f;
    StoppingAngularVelocityThreshold = 0.1f;
    
    // Ball Physics - Reset
    MaxAirborneTime = 10.0f;
    StuckCheckInterval = 2.0f;
    StuckMovementThreshold = 5.0f;
    
    // Ball Physics - Dimensions
    BallRadius = 2.135f;  // Real golf ball
    BallMass = 0.04593f;  // 45.93 grams
    
    // Ball Physics - Damping
    BallLinearDamping = 0.2f;
    BallAngularDamping = 0.3f;
    
    // Ghost Ball
    bAllowGhostBalls = true;
    bAllowGhostBallsInMultiplayer = true;
    MaxGhostBallsPerHole = -1;  // Unlimited by default
    GhostBallMaxLifetime = 5.0f;
    GhostBallFadeStartTime = 3.0f;
    GhostBallOpacity = 0.5f;
    
    // Gameplay Rules
    DefaultTotalHoles = 18;
    MaxStrokesPerHole = -1;  // Unlimited by default
    bAllowSimultaneousPlay = false;
    
    // Teleport
    TeleportDistanceBehindBall = 100.0f;
    bAutoTeleportAfterStroke = false;
    
    // Debug
    bShowBallDebugInfo = false;
    bShowPutterDebug = false;
    bShowGroundProjection = false;
}
