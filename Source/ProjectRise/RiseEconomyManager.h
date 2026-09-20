#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "RiseEconomyManager.generated.h"

// Enum para identificar cada tipo de recurso
UENUM(BlueprintType)
enum class ERiseResourceType : uint8
{
    Food        UMETA(DisplayName = "Food"),
    MicroChips  UMETA(DisplayName = "MicroChips"),
    RareMetal   UMETA(DisplayName = "RareMetal"),
    Petrol      UMETA(DisplayName = "Petrol"),
    Dollars     UMETA(DisplayName = "Dollars"),
    Knowledge   UMETA(DisplayName = "Knowledge")
};

// Estrutura agrupando a quantidade dos 6 recursos
USTRUCT(BlueprintType)
struct FRiseResourceContainer
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Economy")
    float Food = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Economy")
    float MicroChips = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Economy")
    float RareMetal = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Economy")
    float Petrol = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Economy")
    float Dollars = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Economy")
    float Knowledge = 0.0f;

    // Leitura/escrita por tipo (usado pela producao e pela IA)
    float GetResource(ERiseResourceType ResourceType) const
    {
        switch (ResourceType)
        {
        case ERiseResourceType::Food:       return Food;
        case ERiseResourceType::MicroChips: return MicroChips;
        case ERiseResourceType::RareMetal:  return RareMetal;
        case ERiseResourceType::Petrol:     return Petrol;
        case ERiseResourceType::Dollars:    return Dollars;
        case ERiseResourceType::Knowledge:  return Knowledge;
        }
        return 0.0f;
    }

    void AddResource(ERiseResourceType ResourceType, float Amount)
    {
        switch (ResourceType)
        {
        case ERiseResourceType::Food:       Food += Amount;       break;
        case ERiseResourceType::MicroChips: MicroChips += Amount; break;
        case ERiseResourceType::RareMetal:  RareMetal += Amount;  break;
        case ERiseResourceType::Petrol:     Petrol += Amount;     break;
        case ERiseResourceType::Dollars:    Dollars += Amount;    break;
        case ERiseResourceType::Knowledge:  Knowledge += Amount;  break;
        }
    }

    bool HasEnough(const FRiseResourceContainer& Cost) const
    {
        return Food >= Cost.Food
            && MicroChips >= Cost.MicroChips
            && RareMetal >= Cost.RareMetal
            && Petrol >= Cost.Petrol
            && Dollars >= Cost.Dollars
            && Knowledge >= Cost.Knowledge;
    }

    bool ConsumeIfAffordable(const FRiseResourceContainer& Cost)
    {
        if (!HasEnough(Cost))
        {
            return false;
        }

        Food -= Cost.Food;
        MicroChips -= Cost.MicroChips;
        RareMetal -= Cost.RareMetal;
        Petrol -= Cost.Petrol;
        Dollars -= Cost.Dollars;
        Knowledge -= Cost.Knowledge;
        return true;
    }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnResourcesChanged, const FRiseResourceContainer&, UpdatedResources);

UCLASS()
class PROJECTRISE_API URiseEconomyManager : public UGameInstance
{
    GENERATED_BODY()

public:
    URiseEconomyManager();

    // Evento disparado para a UI (Blueprints) sempre que os recursos alterarem
    UPROPERTY(BlueprintAssignable, Category = "Economy|Events")
    FOnResourcesChanged OnResourcesChanged;

    // Retorna a estrutura com todos os recursos
    UFUNCTION(BlueprintCallable, Category = "Economy")
    FRiseResourceContainer GetCurrentResources() const { return CurrentResources; }

    // Retorna a quantidade de um recurso específico
    UFUNCTION(BlueprintCallable, Category = "Economy")
    float GetResourceAmount(ERiseResourceType ResourceType) const;

    // Adiciona recurso ao saldo atual
    UFUNCTION(BlueprintCallable, Category = "Economy")
    void AddResource(ERiseResourceType ResourceType, float Amount);

    // Tenta gastar um recurso. Retorna true se houver saldo suficiente
    UFUNCTION(BlueprintCallable, Category = "Economy")
    bool ConsumeResource(ERiseResourceType ResourceType, float Amount);

    // Verifica se possui saldo suficiente para um custo agrupado
    UFUNCTION(BlueprintCallable, Category = "Economy")
    bool CanAfford(const FRiseResourceContainer& Cost) const;

    // Consome um grupo de recursos de uma só vez (ex: construir uma estrutura/unidade)
    UFUNCTION(BlueprintCallable, Category = "Economy")
    bool ConsumeResourcesGroup(const FRiseResourceContainer& Cost);

private:
    UPROPERTY(EditAnywhere, Category = "Economy|Initial")
    FRiseResourceContainer CurrentResources;
};

// --------------------------------------------------------------------------------------
// Economia por faccao.
// Faccao 0 = jogador (saldo no URiseEconomyManager / GameInstance).
// Demais faccoes = saldo no ARiseAIManager correspondente.
// --------------------------------------------------------------------------------------

PROJECTRISE_API void RiseDepositToFaction(const UObject* WorldContext, int32 FactionID, ERiseResourceType ResourceType, float Amount);
PROJECTRISE_API bool RiseFactionCanAfford(const UObject* WorldContext, int32 FactionID, const FRiseResourceContainer& Cost);
PROJECTRISE_API bool RiseFactionConsumeResources(const UObject* WorldContext, int32 FactionID, const FRiseResourceContainer& Cost);
PROJECTRISE_API bool RiseFactionGetResources(const UObject* WorldContext, int32 FactionID, FRiseResourceContainer& OutResources);