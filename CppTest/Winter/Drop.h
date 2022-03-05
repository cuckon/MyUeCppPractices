#pragma once
#include <CoreMinimal.h>

const float kBirthTimeNotInitialized = -2.0f;
const float kBirthTimeOutsideOfFinger = -1.0f;   // Outside of finger tip, ready to be active

class Drop
{
public:
    FVector2D Position;
    FVector2D Velocity;
    FVector2D Stretch;
    float Radius;
    float BirthTimeSeconds;
    // bool Mergeable

    Drop(const FVector2D &Position, const FVector2D& Velocity, const FVector2D& Stretch,
        float Radius=1.0f, float BirthTimeSeconds=kBirthTimeNotInitialized)
        : Position(Position), Velocity(Velocity), Stretch(Stretch),
        Radius(Radius), BirthTimeSeconds(BirthTimeSeconds)
    {
    };

    bool AreOverlapped(Drop* Another) {
        return (Another->Position - Position).Size() <= (Another->Radius + Radius);
    }

    bool IsActive() const {
        return BirthTimeSeconds >= 0.0f;
    }

    bool IsOutsideOfFinger() const {
        return BirthTimeSeconds >= kBirthTimeOutsideOfFinger;
    }
};

