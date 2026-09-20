#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RiseTerritoryManager.generated.h"

// Estrutura para representar uma célula do território no mapa
USTRUCT(BlueprintType)
struct FRiseTerritoryCell
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Territory")
    FIntPoint GridCoordinate = FIntPoint(0, 0);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Territory")
    FVector WorldPosition = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Territory")
    int32 OwnerFactionID = -1; // -1 significa neutro/não dominado

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Territory")
    bool bIsOccupied = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTerritoryClaimed, FIntPoint, CellCoord, int32, NewOwnerID);

UCLASS()
class PROJECTRISE_API ARiseTerritoryManager : public AActor
{
    GENERATED_BODY()

public:
    ARiseTerritoryManager();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

    // Dimensões da grade (ex: 20x20 células)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Territory|Grid Config")
    int32 GridWidth = 20;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Territory|Grid Config")
    int32 GridHeight = 20;

    // Tamanho em unidades Unreal de cada célula (ex: 500 = 5m x 5m)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Territory|Grid Config")
    float CellSize = 500.0f;

    // Desenhar a grade para debug no editor/jogo
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Territory|Debug")
    bool bShowDebugGrid = true;

    // Evento disparado quando um território muda de dono
    UPROPERTY(BlueprintAssignable, Category = "Territory|Events")
    FOnTerritoryClaimed OnTerritoryClaimed;

    // Inicializa a grade do mapa
    UFUNCTION(BlueprintCallable, Category = "Territory")
    void GenerateGrid();

    // Reivindica uma célula para uma facção
    UFUNCTION(BlueprintCallable, Category = "Territory")
    bool ClaimCell(FIntPoint GridCoord, int32 FactionID);

    // Converte posição do mundo 3D em coordenada da grade
    UFUNCTION(BlueprintCallable, Category = "Territory")
    FIntPoint WorldToGridPosition(FVector WorldPos) const;

    // Obtém dados de uma célula
    UFUNCTION(BlueprintCallable, Category = "Territory")
    bool GetCellData(FIntPoint GridCoord, FRiseTerritoryCell& OutCell) const;

private:
    // TMap para acesso rápido às células pelas coordenadas (X, Y)
    TMap<FIntPoint, FRiseTerritoryCell> GridMap;

    void DrawDebugGrid();
};