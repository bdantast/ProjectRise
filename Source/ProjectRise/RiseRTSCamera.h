#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "RiseRTSCamera.generated.h"

class USceneComponent;
class USpringArmComponent;
class UCameraComponent;

UCLASS()
class PROJECTRISE_API ARiseRTSCamera : public APawn
{
    GENERATED_BODY()

public:
    ARiseRTSCamera();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    // --- Componentes ---
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera Components")
    USceneComponent* TransformComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera Components")
    USpringArmComponent* CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera Components")
    UCameraComponent* CameraComponent;

    // --- Configurações de Movimento (Pan) ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Config|Move")
    float MoveSpeed = 4000.0f;

    // --- Configurações de Zoom ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Config|Zoom")
    float MinTargetArmLength = 500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Config|Zoom")
    float MaxTargetArmLength = 14000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Config|Zoom")
    float ZoomStep = 500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Config|Zoom")
    float ZoomSpeed = 10.0f;

    // --- Configurações de Rotação ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Config|Rotate")
    float RotateSpeed = 100.0f;

    // --- Métodos de Entrada ---
    void MoveForward(float Value);
    void MoveRight(float Value);
    void ZoomIn();
    void ZoomOut();
    void RotateCamera(float Value);
    void MouseWheelZoom(float Value);

    // Funções separadas para o BindAction do botão de rotação (evita erros de template)
    void StartRotation();
    void StopRotation();

private:
    float TargetArmLength = 3500.0f;
    FVector MovementInput = FVector::ZeroVector;
    bool bIsRotating = false;
};