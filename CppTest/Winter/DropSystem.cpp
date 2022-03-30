#include "DropSystem.h"
#include "Drop.h"
#include "Common.h"

#include <utility>

#include <Kismet/KismetRenderingLibrary.h>
#include <Engine/Canvas.h>


PRAGMA_OPTION

const float kRadiusAnimationExp = 10.0f;
const float kAreaLossFactor = 0.35f;     // Bigger value causes more area loss when splitting
const float kAreaGainFactor = 0.5;      // Bigger value make drops grow faster when merging with others
const float kVelocityLossFactor = 0.85f;
const float kAreaIncreaseFactorMin = 0.015f;    // Bigger value increase the growing speed of marching drops.
const float kAreaIncreaseFactorMax = 0.35f;
const float kAreaIncreaseFactorExp = 6.0f;
const float kStretchVelocityFactor = 0.1f;


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
    Drop* CurrentDropPtr;
    float ForceDownward;
    float Friction;
    float AreaGrowed;
    float MarchedDistance;
    TSet<int> MovedIDs;
    FVector2D MoveVector;
    for (auto& Iter : m_Drops)
    {
        CurrentDropPtr = Iter.Value;

        // Calc Force
        Friction = CurrentDropPtr->Velocity.Y > 0 ? m_DynamicFriction : m_StaticFriction;
        ForceDownward = CurrentDropPtr->GetMass() * m_Gravity - Friction;

        // Calc Velocity
        CurrentDropPtr->Velocity.Y += ForceDownward * TimeDeltaSeconds;
        CurrentDropPtr->Velocity.Y = FMath::Max(CurrentDropPtr->Velocity.Y, 0.0f);

        // Calc Position
        MoveVector = CurrentDropPtr->Velocity * TimeDeltaSeconds * m_VelocityScale;
        CurrentDropPtr->Position += MoveVector;
        CurrentDropPtr->DistanceNoTrail += (MarchedDistance = MoveVector.Size());

        // Increase Radius while marching downward
        AreaGrowed = MarchedDistance * FMath::GetMappedRangeValueUnclamped(
            FVector2D(0.0f, 1.0f), FVector2D(kAreaIncreaseFactorMin, kAreaIncreaseFactorMax),
            FMath::Pow(FMath::RandRange(0.0f, 1.0f), kAreaIncreaseFactorExp)
        );
        CurrentDropPtr->AdjustArea(AreaGrowed);

        // Collect Moved
        if (CurrentDropPtr->Velocity.Y > 0)
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

void DropSystem::SplitTrailDrops(float DeltaSeconds, const TSet<int>& MovedIDs)
{
    Drop* CurrentDropPtr;
    float Speed;
    FVector2D Position;
    for (auto ID : MovedIDs) {
        CurrentDropPtr = m_Drops[ID];
        Speed = CurrentDropPtr->Velocity.Size() * m_VelocityScale;
        if (Speed < m_SplitTrailVelocityThreshold)
            continue;
        if (!CurrentDropPtr->IsActive())
            continue;

        if (CurrentDropPtr->DistanceNoTrail < CurrentDropPtr->NextTrailDistance)
            continue;

        CurrentDropPtr->ResetTrailDistance();

        float Radius = CurrentDropPtr->Radius * FMath::RandRange(0.3f, 0.5f);
        Position = CurrentDropPtr->Position + FVector2D(
            FMath::RandRange(-0.2f, 0.2f), -0.4 
        ) * CurrentDropPtr->Radius;
        Emit(
            Position,
            FVector2D(0.0, 0.0),
            FVector2D::UnitVector + FVector2D(0.2, CurrentDropPtr->Velocity.Y * kStretchVelocityFactor),
            Radius,
            kBirthTimeOutsideOfFinger
        );

        // Make area conservative
        CurrentDropPtr->AdjustArea(- Radius * Radius * kAreaLossFactor);
        CurrentDropPtr->Velocity *= kVelocityLossFactor;
    }
}

TSet<int> DropSystem::Tick(float DeltaSeconds, const FVector2D& ClipSize)
{
    TSet<int> MovedIDs = Simulate(DeltaSeconds);
    MovedIDs = Clip(ClipSize, MovedIDs);
    SplitTrailDrops(DeltaSeconds, MovedIDs);
    ProcessOverlaps(MovedIDs);
    return MovedIDs;
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

void DropSystem::ProcessOverlaps(const TSet<int>& MovedIDs)
{
    TArray<IDPair> IDPairs;
    TArray<int> IDs;
    m_Drops.GetKeys(IDs);

    for (auto i : MovedIDs) {
        for (auto j : IDs) {
            if (i == j) continue;
            if (MovedIDs.Contains(j) && i > j) continue;

            if (m_Drops[i]->AreOverlapped(m_Drops[j])) {
                IDPairs.Add(std::make_pair(i, j));
            }
        }
    }

    ActiveTrailDrops(IDPairs);
    MergeDrops(IDPairs);
}

void DropSystem::ActiveTrailDrops(const TArray<IDPair>& OverlappedPairs)
{

    TSet<int> SeparateDrops;
    m_Drops.GetKeys(SeparateDrops);

    for (auto CurrentPair : OverlappedPairs) {
        SeparateDrops.Remove(CurrentPair.first);
        SeparateDrops.Remove(CurrentPair.second);
    }

    for (auto ID : SeparateDrops) {
        if (m_Drops[ID]->BirthTimeSeconds == kBirthTimeOutsideOfFinger)
            m_Drops[ID]->BirthTimeSeconds = m_World->GetTimeSeconds();
    }
}

void DropSystem::MergeDrops(const TArray<IDPair>& OverlappedPairs)
{
    for (auto CurrentPair : OverlappedPairs)
        MergeDrop(CurrentPair.first, CurrentPair.second);
}


void DropSystem::MergeDrop(int ID1, int ID2)
{
    if (!m_Drops.Find(ID1) || !m_Drops.Find(ID2))
        return;
    if (!m_Drops[ID1]->IsActive() || !m_Drops[ID2]->IsActive())
        return;
    if (m_Drops[ID2]->Radius > m_Drops[ID1]->Radius)
        std::swap(ID1, ID2);
    
    float MassOld = m_Drops[ID1]->GetMass();
    m_Drops[ID1]->AdjustArea(m_Drops[ID2]->Radius * m_Drops[ID2]->Radius * kAreaGainFactor);
    m_Drops[ID1]->Velocity *= MassOld / m_Drops[ID1]->GetMass();

    Kill(ID2);
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
    FVector2D StretchFactor;
    FDrawToRenderTargetContext Context;
    TMap<int, FVector2D> RadiusCached;
    TMap<int, FVector2D> PositionCached;

    // Draw Drops
    UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(
        m_World, RT_Drops, Canvas, CanvasSize, Context
    );

    Drop* CurrentDrop;
    float NormalLife, Radius, MappedLife;
    FVector2D Size2D, Position;
    for (auto& Iter : m_Drops) {
        CurrentDrop = Iter.Value;
        if (!CurrentDrop->IsActive())
            continue;
        NormalLife = FMath::Clamp(
            CurrentTime - CurrentDrop->BirthTimeSeconds,
            0.0f, 1.0f
        ); // From 0 to 1

        // From 1 to 0
        MappedLife = FMath::Pow(1 - NormalLife, kRadiusAnimationExp);

        StretchFactor = FMath::Lerp(FVector2D::UnitVector, CurrentDrop->Stretch, MappedLife);
        Radius = (MappedLife * 0.7 + 1.0) * CurrentDrop->Radius;
        Radius *= m_RadiusRenderFactor;
        Size2D = FVector2D(Radius, Radius * ViewPortRatio) * 2 * StretchFactor;
        Position = CurrentDrop->Position - Size2D * 0.5;
        Canvas->K2_DrawTexture(
            T_Raindrop,
            Position,
            Size2D,
            FVector2D::ZeroVector,  // CoordinatePosition
            FVector2D::UnitVector,  // CoordinateSize
            FLinearColor::White,    // RenderColor
            BLEND_AlphaComposite   //BlendMode;
        );
        PositionCached.Add(Iter.Key, Position);
        RadiusCached.Add(Iter.Key, Size2D);

    }
    UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(m_World, Context);

    // Draw Trails
    UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(
        m_World, RT_MovedDrops, Canvas, CanvasSize, Context
    );

    for (auto ID : MovedIDs) {
        if (!m_Drops[ID]->IsActive())
            continue;

        Size2D = FVector2D(m_Drops[ID]->Radius, m_Drops[ID]->Radius * ViewPortRatio) * 2;
        Position = m_Drops[ID]->Position - Size2D * 0.5;

        Canvas->K2_DrawTexture(
            T_Raindrop,
            Position, Size2D,
            FVector2D::ZeroVector,  // CoordinatePosition
            FVector2D::UnitVector,  // CoordinateSize
            FLinearColor::White,    // RenderColor
            BLEND_AlphaComposite   //BlendMode;
        );

    }
    UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(m_World, Context);
}

void DropSystem::MarkDropsOutsideFinger(const FVector2D& Center, float Radius)
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
        CurrentDrop->BirthTimeSeconds = kBirthTimeOutsideOfFinger;
    }
}
