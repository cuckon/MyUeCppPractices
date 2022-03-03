#include "DropSystem.h"
#include "Drop.h"
#include "Common.h"

#include <Kismet/KismetRenderingLibrary.h>
#include <Engine/Canvas.h>

PRAGMA_OPTION

const float kRadiusAnimationExp = 14.0f;
const float kSplitChance = 0.1f;

DropSystem::DropSystem():m_World(nullptr), m_NextID(0)
{
}

DropSystem::~DropSystem()
{
    for (auto& CurrentDrop : m_Drops) {
        delete CurrentDrop.Value;
    }
}

void DropSystem::Kill(int ID)
{
    if (!m_Drops.Contains(ID)) {
        UE_LOG(
            LogProcess, Error, TEXT("ID %i is not found."), ID
        );
        return;
    }
    delete m_Drops[ID];
    m_Drops.Remove(ID);
}

TSet<int> DropSystem::Simulate(float TimeDeltaSeconds)
{
    float Mass;
    Drop* CurrentDrop;
    float ForceDownward;
    float Friction;
    TSet<int> MovedIDs;
    for (auto& Iter : m_Drops)
    {
        CurrentDrop = Iter.Value;

        // Calc Force
        Mass = CurrentDrop->Radius * CurrentDrop->Radius * m_Density;
        Friction = CurrentDrop->Velocity.Y > 0 ? m_DynamicFriction : m_StaticFriction;
        ForceDownward = Mass * m_Gravity - Friction;

        // Calc Velocity
        CurrentDrop->Velocity.Y += ForceDownward * TimeDeltaSeconds;
        CurrentDrop->Velocity.Y = FMath::Max(CurrentDrop->Velocity.Y, 0.0f);

        // Calc Position
        CurrentDrop->Position += CurrentDrop->Velocity * TimeDeltaSeconds * m_VelocityScale;

        // Collect Moved
        if (CurrentDrop->Velocity.Y > 0)
            MovedIDs.Add(Iter.Key);
    }
    return MovedIDs;
}

void DropSystem::Kill(const FVector2D& Center, float Radius)
{
    // Store keys before hands to avoid removal during iteration
    TArray<int> IDs;
    m_Drops.GetKeys(IDs);

    Drop* CurrentDrop;
    float Distance, Threshold;
    for (auto& ID : IDs)
    {
        CurrentDrop = m_Drops[ID];
        if (!CurrentDrop->IsActive())
            continue;

        Distance = (Center - CurrentDrop->Position).Size();
        Threshold = Radius + CurrentDrop->Radius;
        if (Distance > Threshold)
            continue;
        Kill(ID);
    }
}
void DropSystem::SplitTrailDrops(float DeltaSeconds)
{
    TSet<int> IDs;
    m_Drops.GetKeys(IDs);

    Drop* CurrentDropPtr;
    for (auto ID : IDs) {
        CurrentDropPtr = m_Drops[ID];
        if (CurrentDropPtr->Velocity.Size() < m_SplitTrailVelocityThreshold)
            continue;
        if (!CurrentDropPtr->IsActive())
            continue;
        if (FMath::RandRange(0.0f, 1.0f) > kSplitChance / DeltaSeconds)
            continue;

        float Radius = FMath::GetMappedRangeValueUnclamped(
            FVector2D(0.0f, 1.0f),
            FVector2D(kDropEmitRadiusMinDefault, kDropEmitRadiusMaxDefault),
            FMath::Pow(FMath::RandRange(0.0f, 1.0f), kDropEmitRadiusExpDefault)
        );
        Emit(
            CurrentDropPtr->Position,
            FVector2D(0.0, 0.0),
            FVector2D(1.0, 2.0),
            Radius
        );

        // Make area conservative
        CurrentDropPtr->Radius = FMath::Sqrt(
            CurrentDropPtr->Radius * CurrentDropPtr->Radius - Radius * Radius
        );
    }
}
/* 
* @param OutMovedIDs - Will be iterated and changed.
*/
TSet<int> DropSystem::Clip(const FVector2D& Size, const TSet<int>& MovedIDs)
{
    TArray<int> IDs;
    m_Drops.GetKeys(IDs);

    TSet<int> RemainingIDs;
    FVector2D Position;
    float Radius;

    for (auto& ID : IDs)
    {
        Position = m_Drops[ID]->Position;
        Radius = m_Drops[ID]->Radius;
        if (Position.X + Radius < 0 || Position.Y + Radius <0 ||
            Position.X - Radius > Size.X ||
            Position.Y - Radius > Size.Y
            )
        {
            Kill(ID);
        }
        else if (MovedIDs.Contains(ID)) {
            RemainingIDs.Add(ID);
        }
    }
    return RemainingIDs;
}

void DropSystem::Draw(
    UTextureRenderTarget2D* RT_Drops,
    UTextureRenderTarget2D* RT_MovedDrops, UTexture* T_Raindrop,
    float ViewPortRatio, const TSet<int>& MovedIDs
    )
{
    check(m_World);
    float CurrentTime = m_World->GetTimeSeconds();

    UCanvas* Canvas;
    FVector2D CanvasSize;
    FDrawToRenderTargetContext Context;
    TMap<int, FVector2D> CachedSizes;
    TMap<int, FVector2D> CachedPositions;

    // Draw Drops
    UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(
        m_World, RT_Drops, Canvas, CanvasSize, Context
    );

    Drop* CurrentDrop;
    float NormalLife, Radius;
    FVector2D Radius2D, Position;
    for (auto& Iter : m_Drops) {
        CurrentDrop = Iter.Value;
        if (!CurrentDrop->IsActive())
            continue;
        NormalLife = FMath::Clamp(
            CurrentTime - CurrentDrop->BirthTimeSeconds,
            0.0f, 1.0f
        );
        NormalLife = FMath::Pow(1 - NormalLife, kRadiusAnimationExp);
        Radius = (NormalLife * 0.7 + 1.0) * CurrentDrop->Radius;
        Radius *= m_RadiusRenderFactor;
        Radius2D = FVector2D(Radius, Radius * ViewPortRatio);
        Position = CurrentDrop->Position - Radius2D * 0.5;
        Canvas->K2_DrawTexture(
            T_Raindrop,
            Position,
            Radius2D,
            FVector2D::ZeroVector,  // CoordinatePosition
            FVector2D::UnitVector,  // CoordinateSize
            FLinearColor::White,    // RenderColor
            BLEND_AlphaComposite   //BlendMode;
        );
        CachedPositions.Add(Iter.Key, Position);
        CachedSizes.Add(Iter.Key, Radius2D);

    }
    UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(m_World, Context);

    // Draw Trails
    UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(
        m_World, RT_MovedDrops, Canvas, CanvasSize, Context
    );

    for (auto ID : MovedIDs) {
        if (!m_Drops[ID]->IsActive())
            continue;

        Canvas->K2_DrawTexture(
            T_Raindrop,
            CachedPositions[ID],
            CachedSizes[ID],
            FVector2D::ZeroVector,  // CoordinatePosition
            FVector2D::UnitVector,  // CoordinateSize
            FLinearColor::White,    // RenderColor
            BLEND_AlphaComposite   //BlendMode;
        );

    }
    UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(m_World, Context);
}

void DropSystem::Activate(const FVector2D& Center, float Radius)
{
    Drop* CurrentDrop;
    float Distance, Threshold;
    for (auto Iter : m_Drops)
    {
        CurrentDrop = Iter.Value;
        if (CurrentDrop->IsActive())
            continue;

        Distance = (Center - CurrentDrop->Position).Size();
        Threshold = Radius + CurrentDrop->Radius;
        if (Distance < Threshold)
            continue;
        CurrentDrop->BirthTimeSeconds = m_World->GetTimeSeconds();
    }
}
