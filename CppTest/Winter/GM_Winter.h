// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"

#include "DropSystem.h"

#include "GM_Winter.generated.h"

/**
 * 
 */
UCLASS()
class CPPTEST_API AGM_Winter : public AGameModeBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere)
        UTextureRenderTarget2D* RT_Drops;
    UPROPERTY(EditAnywhere)
        UTextureRenderTarget2D* RT_Strokes;
    UPROPERTY(EditAnywhere)
        UTextureRenderTarget2D* RT_MovedDrops;
    UPROPERTY(EditAnywhere)
        UMaterialInterface* M_Brush;
    UPROPERTY(EditAnywhere)
        UTexture* T_Raindrop;
    UPROPERTY(EditAnywhere)
        float DropRadiusRenderFactor = 10;

public:
    AGM_Winter();

    void StartPlay() override;
    void Tick(float DeltaSeconds) override;
    void FingerPressed();
    void FingerReleased();

    APlayerController* PlayerController;

private:
    void OnStrokeEnd();
    TSet<int> SimDrops(float DeltaSeconds);
    void DrawDrops(const TSet<int>& MovedIDs);
    void OnMouseMove(const FVector2D& FingerPos);
    void ActivateDrops(const FVector2D& Center, float Radius);
    void EmitDrop(
        const FVector2D& Pos_RT, float Chance,
        float RadiusMin, float RadiusMax, float RadiusExp
    );
    void CleanDropsAtPos(const FVector2D& Pos_RT, float Size);

    UWorld* m_World;
    float m_ViewportScale;
    float m_ViewportRatio;
    FVector2D m_ViewFactor;  // ViewportScale / ViewportSize, updated every tick.
    bool m_FingerPressed;
    bool m_JustPressed;
    FVector2D m_LastPosition;  //in viewport local space
    // FVectoer2D m_SecondLastPosition;
    // UMaterialInstanceDynamic* m_M_BrushInstance;
    DropSystem m_DropSystem;
    FVector2D m_RenderTargetSize;
};
