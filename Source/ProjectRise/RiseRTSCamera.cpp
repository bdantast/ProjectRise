#include "RiseRTSCamera.h"
#include "Components/SceneComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "InputCoreTypes.h"

ARiseRTSCamera::ARiseRTSCamera()
{
    PrimaryActorTick.bCanEverTick = true;

    // Componente Raiz
    TransformComponent = CreateDefaultSubobject<USceneComponent>(TEXT("TransformComponent"));
    SetRootComponent(TransformComponent);

    // Braço da Câmera (Spring Arm)
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->SetRelativeRotation(FRotator(-60.0f, 0.0f, 0.0f));
    CameraBoom->TargetArmLength = TargetArmLength;
    CameraBoom->bDoCollisionTest = false; // Desativa colisão para não dar "trancos" na câmera de RTS
    CameraBoom->bEnableCameraLag = true;
    CameraBoom->CameraLagSpeed = 12.0f;

    // Câmera
    CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
    CameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);

    AutoPossessPlayer = EAutoReceiveInput::Player0;
}

void ARiseRTSCamera::BeginPlay()
{
    Super::BeginPlay();
    TargetArmLength = CameraBoom->TargetArmLength;
}

void ARiseRTSCamera::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 1. Processar Movimento (Pan) orientado ao ângulo atual da câmera
    if (!MovementInput.IsZero())
    {
        FRotator CameraRotation = GetActorRotation();
        CameraRotation.Pitch = 0.0f; // Manter movimento paralelo ao chão
        CameraRotation.Roll = 0.0f;

        FVector WorldDirection = UKismetMathLibrary::TransformDirection(FTransform(CameraRotation), MovementInput.GetSafeNormal());
        // Pan mais rapido quando a camera esta mais afastada (mapa de 240 m)
        const float ZoomScale = FMath::Clamp(CameraBoom->TargetArmLength / 3000.0f, 0.6f, 5.0f);
        FVector NewLocation = GetActorLocation() + (WorldDirection * MoveSpeed * ZoomScale * DeltaTime);
        SetActorLocation(NewLocation);
    }

    // 2. Interpolação suave de Zoom (Smooth Interp)
    if (!FMath::IsNearlyEqual(CameraBoom->TargetArmLength, TargetArmLength, 0.1f))
    {
        CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, TargetArmLength, DeltaTime, ZoomSpeed);
    }
}

void ARiseRTSCamera::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    // Bindings de Movimento (WASD / Flechas)
    PlayerInputComponent->BindAxis("MoveForward", this, &ARiseRTSCamera::MoveForward);
    PlayerInputComponent->BindAxis("MoveRight", this, &ARiseRTSCamera::MoveRight);

    // Bindings de Zoom (roda do mouse)
    PlayerInputComponent->BindAxisKey(EKeys::MouseWheelAxis, this, &ARiseRTSCamera::MouseWheelZoom);

    // Bindings de Rotação (segurar botão do meio do mouse e arrastar)
    PlayerInputComponent->BindAction("EnableRotation", IE_Pressed, this, &ARiseRTSCamera::StartRotation);
    PlayerInputComponent->BindAction("EnableRotation", IE_Released, this, &ARiseRTSCamera::StopRotation);
    PlayerInputComponent->BindAxis("RotateCamera", this, &ARiseRTSCamera::RotateCamera);
}




void ARiseRTSCamera::MoveForward(float Value)
{
    MovementInput.X = Value;
}

void ARiseRTSCamera::MoveRight(float Value)
{
    MovementInput.Y = Value;
}

void ARiseRTSCamera::ZoomIn()
{
    TargetArmLength = FMath::Clamp(TargetArmLength - ZoomStep, MinTargetArmLength, MaxTargetArmLength);
}

void ARiseRTSCamera::ZoomOut()
{
    TargetArmLength = FMath::Clamp(TargetArmLength + ZoomStep, MinTargetArmLength, MaxTargetArmLength);
}

void ARiseRTSCamera::RotateCamera(float Value)
{
    UWorld* World = GetWorld();
    if (bIsRotating && Value != 0.0f && World)
    {
        FRotator NewRotation = GetActorRotation();
        NewRotation.Yaw += Value * RotateSpeed * World->GetDeltaSeconds();
        SetActorRotation(NewRotation);
    }
}

void ARiseRTSCamera::MouseWheelZoom(float Value)
{
    if (Value > 0.1f)
    {
        ZoomIn();
    }
    else if (Value < -0.1f)
    {
        ZoomOut();
    }
}
void ARiseRTSCamera::StartRotation()
{
    bIsRotating = true;
}

void ARiseRTSCamera::StopRotation()
{
    bIsRotating = false;
}
