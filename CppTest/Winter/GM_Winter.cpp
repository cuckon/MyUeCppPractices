// Fill out your copyright notice in the Description page of Project Settings.
#include "Winter/GM_Winter.h"

#include "Kismet/GameplayStatics.h"
#include "Engine/Canvas.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Blueprint/WidgetLayoutLibrary.h"

#include "Common.h"


PRAGMA_OPTION

const float kMoveThreshold = 2.5;  // pixel in viewport
const FVector kPawnPos(-110, 0.0, 33.0); 
const float kFingerSizeRT = 20;
const int kBrushSpace = 5; // px
const float kDefaultPressure = 0.3;

TSharedPtr<FWindowsStylusInputInterface> CreateStylusInputInterface();

AGM_Winter::AGM_Winter()
    :RT_Drops(nullptr),
    RT_Strokes(nullptr),
    RT_MovedDrops(nullptr),
    M_Brush(nullptr),
    T_Raindrop(nullptr),
    m_StylusPressure(kDefaultPressure),
    m_LastStylusPressure(kDefaultPressure),
    m_FingerPressed(false),
    m_JustPressed(false)
{
    PrimaryActorTick.bCanEverTick = true;
    m_StylusInputInterface = CreateStylusInputInterface();

    UE_LOG(LogTemp, Log, TEXT("Setup done."));
}


template<class T>
bool EnsurePointer(T* ptr, const FString& name) {
    if (!ptr) {
        UE_LOG(LogInit, Error, TEXT("%s is not initialized."), *name);
        return false;
    }
    return true;
}


void AGM_Winter::StartPlay()
{
    Super::StartPlay();

    // Initialize Variables
    m_World = GetWorld();
    m_RenderTargetSize = FVector2D(static_cast<float>(RT_Drops->SizeX));

    m_DropSystem.m_RadiusRenderFactor = DropRadiusRenderFactor;
    m_DropSystem.m_World = m_World;
    PlayerController = UGameplayStatics::GetPlayerController(m_World, 0);
    m_ViewportScale = UWidgetLayoutLibrary::GetViewportScale(m_World);

    // Inputs
    UInputComponent* ControllerComponent = m_World->GetFirstPlayerController()->InputComponent;
    ControllerComponent->BindKey(
        EKeys::LeftMouseButton, IE_Pressed, this, &AGM_Winter::FingerPressed
    );
    ControllerComponent->BindKey(
        EKeys::LeftMouseButton, IE_Released, this, &AGM_Winter::FingerReleased
    );
    ControllerComponent->BindKey(
        EKeys::RightMouseButton, IE_Pressed, this, &AGM_Winter::PutBigDrop
    );
    PlayerController->bShowMouseCursor = true;

    // Prepare Game Objects
    bool AllPointersReady = EnsurePointer(RT_Strokes, "RT_Strokes") &&
        EnsurePointer(RT_MovedDrops, "RT_MovedDrops") &&
        EnsurePointer(M_Brush, "M_Brush") &&
        EnsurePointer(RT_Drops, "RT_Drops") &&
        EnsurePointer(T_Raindrop, "T_Raindrop");
    if (!AllPointersReady) {
        UE_LOG(LogInit, Fatal, TEXT("Not all necessary variables are specified in AGM_Winter."));
        return;
    }

    UKismetRenderingLibrary::ClearRenderTarget2D(
        m_World, RT_Strokes, FLinearColor(0.0f, 0.0f, 0.0f, 1.0f)
    );
    UKismetRenderingLibrary::ClearRenderTarget2D(
        m_World, RT_MovedDrops, FLinearColor(0.0f, 0.0f, 0.0f, 0.0f)
    );
    // m_M_BrushInstance = UKismetMaterialLibrary::CreateDynamicMaterialInstance(
    //    m_World, M_Brush
    // );
    APawn* pawn = UGameplayStatics::GetPlayerPawn(m_World, 0);
    pawn->SetActorLocation(kPawnPos);



    UE_LOG(LogTemp, Log, TEXT("m_StylusInputInterface->NumInputDevices(): %d"), m_StylusInputInterface->NumInputDevices());

}


void AGM_Winter::Tick(float DeltaSeconds)
{
    TickStylusInputs();

    int SizeX, SizeY;
    PlayerController->GetViewportSize(SizeX, SizeY);
    m_ViewportRatio = static_cast<float>(SizeX) / SizeY;    
    m_ViewFactor = FVector2D(m_ViewportScale, m_ViewportScale) \
        / FVector2D((float)SizeX, (float)SizeY);
    UKismetRenderingLibrary::ClearRenderTarget2D
        (m_World, RT_Drops, FLinearColor(0.0f, 0.0f, 0.0f, 0.0f)
    );
    FVector2D CurrentFingerPos = UWidgetLayoutLibrary::GetMousePositionOnViewport(m_World);

    if (m_FingerPressed) {
        bool MovedFarEnough = FVector2D::Distance(CurrentFingerPos, m_LastPosition) > kMoveThreshold;
        if (MovedFarEnough || m_JustPressed) {
            OnMouseMove(CurrentFingerPos);
            m_LastPosition = CurrentFingerPos;
        }
    }

    m_JustPressed = false;

    TSet<int> MovedIDs = SimDrops(DeltaSeconds);
    DrawDrops(MovedIDs);
}

void AGM_Winter::TickStylusInputs()
{
    if (m_StylusInputInterface.IsValid())
    {
        m_StylusInputInterface->Tick();

        for (int DeviceIdx = 0; DeviceIdx < m_StylusInputInterface->NumInputDevices(); ++DeviceIdx)
        {
            IStylusInputDevice* InputDevice = m_StylusInputInterface->GetInputDevice(DeviceIdx);
            if (InputDevice->IsDirty())
            {
                InputDevice->Tick();

                // Just let the last stylus' pressure be the one we will query.
                // In most case there's only one or no stylus.
                if (InputDevice->GetCurrentState().IsStylusDown() && 
                    InputDevice->GetCurrentState().GetPressure() > 0) {
                    m_LastStylusPressure = m_StylusPressure;
                    m_StylusPressure = InputDevice->GetCurrentState().GetPressure();
                }
            }
        }
    }
}


void AGM_Winter::FingerPressed()
{
    m_FingerPressed = true;
    m_JustPressed = true;
    m_LastPosition = UWidgetLayoutLibrary::GetMousePositionOnViewport(m_World);
}

void AGM_Winter::FingerReleased()
{
    m_FingerPressed = false;

    ActivateDrops(FVector2D(-100.0f, 0.0f), 0.0f);
}

void AGM_Winter::PutBigDrop()
{
    FVector2D Pos = UWidgetLayoutLibrary::GetMousePositionOnViewport(m_World);
    FVector2D Pos_RT = Pos * m_RenderTargetSize * m_ViewFactor;
    EmitDrop(
        Pos_RT, kDropEmitChanceStrokeEnd,
        kDropEmitRadiusMinStrokeEnd, kDropEmitRadiusMaxStrokeEnd,
        kDropEmitRadiusExpStrokeEnd,
        m_World->GetTimeSeconds()
    );
}


TSet<int> AGM_Winter::SimDrops(float DeltaSeconds)
{
    return m_DropSystem.Tick(DeltaSeconds, m_RenderTargetSize);
}


void AGM_Winter::DrawDrops(const TSet<int>& MovedIDs)
{
    TSet<int> ShrinkingIDs = m_DropSystem.GetShrinkingIDs();
    m_DropSystem.Draw(
        RT_Drops, RT_MovedDrops, T_Raindrop, m_ViewportRatio,
        MovedIDs.Union(ShrinkingIDs)
    );
}


/**
* Get called when finger pressed and moved on screen.
* @param FingerPos - Position of finger in viewport local space.
*/
void AGM_Winter::OnMouseMove(const FVector2D& FingerPos)
{
    UCanvas* Canvas;
    FVector2D CanvasSize;
    FDrawToRenderTargetContext Context;

    UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(
        m_World, RT_Strokes, Canvas, CanvasSize, Context
    );
    FVector2D ActiveLocation_RTSpace = CanvasSize * m_ViewFactor * FingerPos;
    ActivateDrops(ActiveLocation_RTSpace, kFingerSizeRT * 0.5);

    FVector2D Diff = FingerPos - m_LastPosition;
    float MovedLength = Diff.Size();
    int NSteps = FMath::RoundToInt(MovedLength / kBrushSpace);
    NSteps = FMath::Max(1, NSteps);
    
    FVector2D DrawPos_RTSpace;
    FVector2D DrawPos_ViewportSpace;
    FVector2D ScreenPos;
    FVector2D Size2D_RT;
    float StepDistance = MovedLength / NSteps;
    float Pressure, SizePressureFactor;
    FVector2D StepVec = Diff.GetSafeNormal() * StepDistance;
    for (int i = 1; i <= NSteps + 1; ++i) {
        DrawPos_ViewportSpace = i * StepVec + m_LastPosition;
        DrawPos_RTSpace = CanvasSize * m_ViewFactor * DrawPos_ViewportSpace;
        EmitDrop(
            DrawPos_RTSpace, kDropEmitChanceDefault, kDropEmitRadiusMinDefault,
            kDropEmitRadiusMaxDefault, kDropEmitRadiusExpDefault
        );
        Pressure = FMath::Lerp(m_LastStylusPressure, m_StylusPressure, (float)(i) / NSteps);
        SizePressureFactor = 0.3 + FMath::Pow(Pressure, 0.7) * 1.5;

        Size2D_RT = FVector2D(
            kFingerSizeRT * SizePressureFactor,
            kFingerSizeRT * m_ViewportRatio * SizePressureFactor
        );
        ScreenPos = DrawPos_RTSpace - Size2D_RT * 0.5;

        Canvas->K2_DrawMaterial(
            M_Brush, ScreenPos, Size2D_RT, FVector2D(0.0, 0.0)
        );

        const float ContactFactor = 0.55;
        m_DropSystem.Kill(DrawPos_RTSpace, kFingerSizeRT * 0.5 * ContactFactor);
    }

    UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(m_World, Context);
}


/**
* Activate drops around `Center`.
* @param Center - Position in RenderTarget space.
* @param Size - Finger size in RenderTarget space.
*/
void AGM_Winter::ActivateDrops(const FVector2D& Center, float Radius)
{
    m_DropSystem.MarkDropsOutsideFinger(Center, Radius);
}

void AGM_Winter::EmitDrop(
    const FVector2D& Pos_RT, float Chance, float RadiusMin, float RadiusMax, float RadiusExp,
    float BirthTime
)
{
    float Dice = FMath::RandRange(0.0f, 1.0f);
    if (Dice >= Chance)
        return;

    float Radius = FMath::GetMappedRangeValueUnclamped(
        FVector2D(0.0f, 1.0f), FVector2D(RadiusMin, RadiusMax),
        FMath::Pow(FMath::RandRange(0.0f, 1.0f), RadiusExp)
    );

    m_DropSystem.Emit(
        Pos_RT, FVector2D(0.0, 0.0), FVector2D(0.0, 0.0), Radius, BirthTime
    );
}

void AGM_Winter::CleanDropsAtPos(const FVector2D& Pos_RT, float Size)
{
}
