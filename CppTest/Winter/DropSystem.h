#pragma once
#include <CoreMinimal.h>

#include "Drop.h"

typedef std::pair<int, int> IDPair;

class DropSystem
{
public:
    DropSystem();
    ~DropSystem();

    template<class... Types> Drop* Emit(Types... Args);
    void Kill(int ID);
    void Draw(UTextureRenderTarget2D* RT_Drops,
        UTextureRenderTarget2D* RT_MovedDrops, UTexture* T_Raindrop,
        float ViewPortRatio, const TSet<int>& MovedIDs
    );
    void MarkDropsOutsideFinger(const FVector2D& Center, float Radius);
    void Kill(const FVector2D& Center, float Radius);
    TSet<int> Tick(float TimeDeltaSeconds, const FVector2D& ClipSize);

    TMap<int, Drop*> m_Drops;
    float m_RadiusRenderFactor = 1.0f;  // For compensating the texture alpha margin
    UWorld* m_World;
    float m_Gravity = 10.0;
    float m_StaticFriction = 450.0;     // Force that imposed on drops
    float m_DynamicFriction = 430.0;
    float m_VelocityScale = 30.0;
    float m_SplitTrailVelocityThreshold = 50.0f;

private:
    void SplitTrailDrops(float DeltaSeconds, const TSet<int>& MovedIDs);
    TSet<int> Clip(const FVector2D& Size, const TSet<int>& MovedIDs);
    TSet<int> Simulate(float TimeDeltaSeconds);
    void ProcessOverlaps(const TSet<int>& MovedIDs);
    void ActiveTrailDrops(const TArray<IDPair>& OverlappedPairs);
    void MergeDrops(const TArray<IDPair>& OverlappedPairs);
    void MergeDrop(int ID1, int ID2);

    int m_NextID = 0;
};


/* Put it here because
*  https://stackoverflow.com/questions/495021/why-can-templates-only-be-implemented-in-the-header-file
*/
template<class... Types>
Drop* DropSystem::Emit(Types... Args)
{
    Drop* NewDrop = new Drop(Args...);
    m_Drops.Add(m_NextID, NewDrop);
    m_NextID++;
    return NewDrop;
}