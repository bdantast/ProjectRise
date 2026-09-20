#include "RiseEconomyManager.h"
#include "RiseAIManager.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

URiseEconomyManager::URiseEconomyManager()
{
    // Valores iniciais padrão para testes
    CurrentResources.Food = 300.0f;
    CurrentResources.MicroChips = 120.0f;
    CurrentResources.RareMetal = 150.0f;
    CurrentResources.Petrol = 150.0f;
    CurrentResources.Dollars = 800.0f;
    CurrentResources.Knowledge = 120.0f;
}

float URiseEconomyManager::GetResourceAmount(ERiseResourceType ResourceType) const
{
    switch (ResourceType)
    {
    case ERiseResourceType::Food:       return CurrentResources.Food;
    case ERiseResourceType::MicroChips: return CurrentResources.MicroChips;
    case ERiseResourceType::RareMetal:  return CurrentResources.RareMetal;
    case ERiseResourceType::Petrol:     return CurrentResources.Petrol;
    case ERiseResourceType::Dollars:    return CurrentResources.Dollars;
    case ERiseResourceType::Knowledge:  return CurrentResources.Knowledge;
    default: return 0.0f;
    }
}

void URiseEconomyManager::AddResource(ERiseResourceType ResourceType, float Amount)
{
    if (Amount <= 0.0f) return;

    switch (ResourceType)
    {
    case ERiseResourceType::Food:       CurrentResources.Food += Amount; break;
    case ERiseResourceType::MicroChips: CurrentResources.MicroChips += Amount; break;
    case ERiseResourceType::RareMetal:  CurrentResources.RareMetal += Amount; break;
    case ERiseResourceType::Petrol:     CurrentResources.Petrol += Amount; break;
    case ERiseResourceType::Dollars:    CurrentResources.Dollars += Amount; break;
    case ERiseResourceType::Knowledge:  CurrentResources.Knowledge += Amount; break;
    }

    OnResourcesChanged.Broadcast(CurrentResources);
}

bool URiseEconomyManager::ConsumeResource(ERiseResourceType ResourceType, float Amount)
{
    if (Amount <= 0.0f) return true;

    if (GetResourceAmount(ResourceType) < Amount)
    {
        return false; // Saldo insuficiente
    }

    switch (ResourceType)
    {
    case ERiseResourceType::Food:       CurrentResources.Food -= Amount; break;
    case ERiseResourceType::MicroChips: CurrentResources.MicroChips -= Amount; break;
    case ERiseResourceType::RareMetal:  CurrentResources.RareMetal -= Amount; break;
    case ERiseResourceType::Petrol:     CurrentResources.Petrol -= Amount; break;
    case ERiseResourceType::Dollars:    CurrentResources.Dollars -= Amount; break;
    case ERiseResourceType::Knowledge:  CurrentResources.Knowledge -= Amount; break;
    }

    OnResourcesChanged.Broadcast(CurrentResources);
    return true;
}

namespace
{
    ARiseAIManager* FindAIManager(const UObject* WorldContext, int32 FactionID)
    {
        if (!WorldContext || FactionID == 0)
        {
            return nullptr;
        }

        UWorld* World = WorldContext->GetWorld();
        if (!World)
        {
            return nullptr;
        }

        for (TActorIterator<ARiseAIManager> It(World); It; ++It)
        {
            ARiseAIManager* Manager = *It;
            if (IsValid(Manager) && Manager->GetFactionID() == FactionID)
            {
                return Manager;
            }
        }

        return nullptr;
    }
}

void RiseDepositToFaction(const UObject* WorldContext, int32 FactionID, ERiseResourceType ResourceType, float Amount)
{
    if (Amount <= 0.0f)
    {
        return;
    }

    if (FactionID == 0)
    {
        if (URiseEconomyManager* Economy = Cast<URiseEconomyManager>(UGameplayStatics::GetGameInstance(WorldContext)))
        {
            Economy->AddResource(ResourceType, Amount);
        }
        return;
    }

    if (ARiseAIManager* Manager = FindAIManager(WorldContext, FactionID))
    {
        Manager->AddResource(ResourceType, Amount);
    }
}

bool RiseFactionCanAfford(const UObject* WorldContext, int32 FactionID, const FRiseResourceContainer& Cost)
{
    if (FactionID == 0)
    {
        if (URiseEconomyManager* Economy = Cast<URiseEconomyManager>(UGameplayStatics::GetGameInstance(WorldContext)))
        {
            return Economy->CanAfford(Cost);
        }
        return false;
    }

    if (ARiseAIManager* Manager = FindAIManager(WorldContext, FactionID))
    {
        return Manager->GetResources().HasEnough(Cost);
    }

    return false;
}

bool RiseFactionConsumeResources(const UObject* WorldContext, int32 FactionID, const FRiseResourceContainer& Cost)
{
	if (FactionID == 0)
	{
		URiseEconomyManager* Economy = Cast<URiseEconomyManager>(UGameplayStatics::GetGameInstance(WorldContext));
		if (Economy)
		{
			return Economy->ConsumeResourcesGroup(Cost);
		}
		UE_LOG(LogTemp, Error, TEXT("[Economia] GameInstance NAO e RiseEconomyManager (cast falhou)"));
		return false;
	}

    if (ARiseAIManager* Manager = FindAIManager(WorldContext, FactionID))
    {
        return Manager->ConsumeResources(Cost);
    }

    return false;
}

bool RiseFactionGetResources(const UObject* WorldContext, int32 FactionID, FRiseResourceContainer& OutResources)
{
    if (FactionID == 0)
    {
        if (URiseEconomyManager* Economy = Cast<URiseEconomyManager>(UGameplayStatics::GetGameInstance(WorldContext)))
        {
            OutResources = Economy->GetCurrentResources();
            return true;
        }
        return false;
    }

    if (ARiseAIManager* Manager = FindAIManager(WorldContext, FactionID))
    {
        OutResources = Manager->GetResources();
        return true;
    }

    return false;
}

bool URiseEconomyManager::CanAfford(const FRiseResourceContainer& Cost) const
{
    return CurrentResources.Food >= Cost.Food &&
        CurrentResources.MicroChips >= Cost.MicroChips &&
        CurrentResources.RareMetal >= Cost.RareMetal &&
        CurrentResources.Petrol >= Cost.Petrol &&
        CurrentResources.Dollars >= Cost.Dollars &&
        CurrentResources.Knowledge >= Cost.Knowledge;
}

bool URiseEconomyManager::ConsumeResourcesGroup(const FRiseResourceContainer& Cost)
{
    if (!CanAfford(Cost))
    {
        return false;
    }

    CurrentResources.Food -= Cost.Food;
    CurrentResources.MicroChips -= Cost.MicroChips;
    CurrentResources.RareMetal -= Cost.RareMetal;
    CurrentResources.Petrol -= Cost.Petrol;
    CurrentResources.Dollars -= Cost.Dollars;
    CurrentResources.Knowledge -= Cost.Knowledge;

    OnResourcesChanged.Broadcast(CurrentResources);
    return true;
}