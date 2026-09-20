#include "RiseTerritoryManager.h"
#include "DrawDebugHelpers.h"

ARiseTerritoryManager::ARiseTerritoryManager()
{
    PrimaryActorTick.bCanEverTick = true;
}

void ARiseTerritoryManager::BeginPlay()
{
    Super::BeginPlay();
    GenerateGrid();
}

void ARiseTerritoryManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bShowDebugGrid)
    {
        DrawDebugGrid();
    }
}

void ARiseTerritoryManager::GenerateGrid()
{
    GridMap.Empty();
    FVector Origin = GetActorLocation();

    for (int32 x = 0; x < GridWidth; ++x)
    {
        for (int32 y = 0; y < GridHeight; ++y)
        {
            FIntPoint Coord(x, y);
            FRiseTerritoryCell Cell;
            Cell.GridCoordinate = Coord;
            Cell.WorldPosition = Origin + FVector(x * CellSize, y * CellSize, 0.0f);
            Cell.OwnerFactionID = -1; // Neutro
            Cell.bIsOccupied = false;

            GridMap.Add(Coord, Cell);
        }
    }
}

bool ARiseTerritoryManager::ClaimCell(FIntPoint GridCoord, int32 FactionID)
{
    if (FRiseTerritoryCell* Cell = GridMap.Find(GridCoord))
    {
        Cell->OwnerFactionID = FactionID;
        Cell->bIsOccupied = true;

        OnTerritoryClaimed.Broadcast(GridCoord, FactionID);
        return true;
    }
    return false;
}

FIntPoint ARiseTerritoryManager::WorldToGridPosition(FVector WorldPos) const
{
    FVector LocalPos = WorldPos - GetActorLocation();
    int32 x = FMath::RoundToInt(LocalPos.X / CellSize);
    int32 y = FMath::RoundToInt(LocalPos.Y / CellSize);
    return FIntPoint(x, y);
}

bool ARiseTerritoryManager::GetCellData(FIntPoint GridCoord, FRiseTerritoryCell& OutCell) const
{
    if (const FRiseTerritoryCell* Cell = GridMap.Find(GridCoord))
    {
        OutCell = *Cell;
        return true;
    }
    return false;
}

void ARiseTerritoryManager::DrawDebugGrid()
{
    for (const auto& KVP : GridMap)
    {
        const FRiseTerritoryCell& Cell = KVP.Value;
        FVector Center = Cell.WorldPosition + FVector(CellSize * 0.5f, CellSize * 0.5f, 10.0f);

        FColor GridColor = FColor::White;
        if (Cell.OwnerFactionID == 0) GridColor = FColor::Blue;        // Jogador / Facção 0
        else if (Cell.OwnerFactionID == 1) GridColor = FColor::Red;   // Inimigo / Facção 1

        DrawDebugBox(GetWorld(), Center, FVector(CellSize * 0.48f, CellSize * 0.48f, 2.0f), GridColor, false, -1.0f, 0, 2.0f);
    }
}