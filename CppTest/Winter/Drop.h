#pragma once
#include <CoreMinimal.h>

const float kBirthTimeNotInitialized = -2.0f;
const float kBirthTimeOutsideOfFinger = -1.0f;   // Outside of finger tip, ready to be active
const float kDensity = 25.0f;

class Drop
{
public:
    FVector2D Position;
    FVector2D Velocity;
    FVector2D Stretch;
    float Radius;
    float BirthTimeSeconds;
    float DistanceNoTrail;
    float NextTrailDistance;

    Drop(const FVector2D &Position, const FVector2D& Velocity, const FVector2D& Stretch,
        float Radius=1.0f, float BirthTimeSeconds=kBirthTimeNotInitialized)
        : Position(Position)
        , Velocity(Velocity)
        , Stretch(Stretch)
        , Radius(Radius)
        , BirthTimeSeconds(BirthTimeSeconds)
    {
        ResetTrailDistance();
    };

    bool AreOverlapped(Drop* Another) {
        return (Another->Position - Position).Size() <= (Another->Radius + Radius);
    }

    void ResetTrailDistance() {
        DistanceNoTrail = 0;
        NextTrailDistance = FMath::RandRange(20.0f, 50.0f);
    }

    bool IsActive() const {
        return BirthTimeSeconds >= 0.0f;
    }

    bool IsOutsideOfFinger() const {
        return BirthTimeSeconds >= kBirthTimeOutsideOfFinger;
    }

    float GetMass() const {
        return Radius * Radius * kDensity;
    }

    void AdjustArea(float Delta) {
        Radius = FMath::Sqrt( Radius * Radius + Delta);
    }
};

