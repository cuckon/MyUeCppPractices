#pragma once
#include <CoreMinimal.h>

#include "Drop.h"


class DropSystem
{
public:
    DropSystem();
    ~DropSystem();

    template<class... Types> Drop* Emit(Types... Args);
    void Kill(int ID);
    TSet<int> Simulate(float TimeDeltaSeconds);
    void Draw(UTextureRenderTarget2D* RT_Drops,
        UTextureRenderTarget2D* RT_MovedDrops, UTexture* T_Raindrop,
        float ViewPortRatio, const TSet<int>& MovedIDs
    );
    void Activate(const FVector2D& Center, float Radius);
    void Kill(const FVector2D& Center, float Radius);
    void SplitTrailDrops(float DeltaSeconds);
    //void Radius
    TSet<int> Clip(const FVector2D& Size, const TSet<int>& MovedIDs);

    TMap<int, Drop*> m_Drops;
    float m_RadiusRenderFactor = 10.0f;
    UWorld* m_World;
    float m_Gravity = 10.0;
    float m_StaticFriction = 450.0;     // Force that imposed on drops
    float m_DynamicFriction = 350.0;
    float m_Density = 25.0;
    float m_VelocityScale = 1.0;
    float m_SplitTrailVelocityThreshold = 300.0f;

private:
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