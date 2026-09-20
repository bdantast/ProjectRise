#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RiseEconomyManager.h"
#include "RiseUnit.h"
#include "RiseAIManager.generated.h"

class ARiseBase;

/**
 * Comandante da faccao inimiga.
 *
 * Responsabilidades:
 *  - Banco de recursos proprio (o jogador usa o URiseEconomyManager).
 *  - Renda passiva + deposito das coletoras inimigas.
 *  - Decide o que produzir na base inimiga (coletoras primeiro, depois exercito).
 *  - Ataca em ondas periodicamente, com intervalo diminuindo a cada onda.
 *  - Defende a base quando unidades do jogador se aproximam.
 *
 * Nao usa NavMesh/BehaviorTree: as unidades se movem em linha reta (padrao do projeto),
 * entao as ordens sao dadas diretamente pelos metodos publicos de ARiseUnit.
 */
UCLASS()
class PROJECTRISE_API ARiseAIManager : public AActor
{
	GENERATED_BODY()

public:
	ARiseAIManager();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	int32 GetFactionID() const { return FactionID; }

	// Saldo da faccao (coletoras inimigas depositam aqui via RiseDepositToFaction)
	void AddResource(ERiseResourceType ResourceType, float Amount);
	bool ConsumeResources(const FRiseResourceContainer& Cost);
	const FRiseResourceContainer& GetResources() const { return Resources; }

	UFUNCTION(BlueprintPure, Category = "Rise|AI")
	int32 GetWaveCount() const { return WaveCount; }

	// Recursos iniciais e renda passiva por segundo
	UPROPERTY(EditAnywhere, Category = "Rise|AI")
	int32 FactionID = 1;

	UPROPERTY(EditAnywhere, Category = "Rise|AI")
	FRiseResourceContainer StartingResources;

	UPROPERTY(EditAnywhere, Category = "Rise|AI")
	FRiseResourceContainer IncomePerSecond;

	// Ritmo de decisao/producao (segundos)
	UPROPERTY(EditAnywhere, Category = "Rise|AI")
	float DecisionInterval = 5.0f;

	// Primeira onda e intervalo entre ondas (o intervalo encurta a cada onda)
	UPROPERTY(EditAnywhere, Category = "Rise|AI")
	float FirstAttackDelay = 75.0f;

	UPROPERTY(EditAnywhere, Category = "Rise|AI")
	float BaseWaveInterval = 42.0f;

	UPROPERTY(EditAnywhere, Category = "Rise|AI")
	float MinWaveInterval = 20.0f;

	// Quantas coletoras a IA quer manter vivas
	UPROPERTY(EditAnywhere, Category = "Rise|AI")
	int32 DesiredWorkers = 3;

	// Quantos tanques antes de variar a composicao
	UPROPERTY(EditAnywhere, Category = "Rise|AI")
	int32 DesiredTanks = 3;

	// Raio em que a IA considera a base sob ataque
	UPROPERTY(EditAnywhere, Category = "Rise|AI")
	float DefenseRadius = 7000.0f;

	// So ataca quando tiver esse numero de unidades de combate (evita ondas suicidas)
	UPROPERTY(EditAnywhere, Category = "Rise|AI")
	int32 MinAttackUnits = 5;

	// Distancia do alvo onde a tropa se concentra antes de avancar junto
	UPROPERTY(EditAnywhere, Category = "Rise|AI")
	float GatherDistance = 6500.0f;

	// Raio considerado "chegou no ponto de reuniao"
	UPROPERTY(EditAnywhere, Category = "Rise|AI")
	float GatherRadius = 2000.0f;

	// Tempo maximo esperando a tropa se juntar antes de atacar assim mesmo
	UPROPERTY(EditAnywhere, Category = "Rise|AI")
	float GatherTimeout = 40.0f;

	// Se ainda nao juntou tropa suficiente, tenta de novo nesse intervalo
	UPROPERTY(EditAnywhere, Category = "Rise|AI")
	float AttackRetryInterval = 12.0f;

	// Reforco: unidades novas/ociosas recebem ordem de ataque nesse intervalo
	UPROPERTY(EditAnywhere, Category = "Rise|AI")
	float ReinforceInterval = 5.0f;

private:
	void TickEconomy(float DeltaTime);
	void TickProduction(float DeltaTime);
	void TickMilitary(float DeltaTime);
	void TickWorkerAssignments(float DeltaTime);
	void TickDefense(float DeltaTime);

	ARiseBase* GetHomeBase();
	AActor* FindAttackTarget() const;
	bool HasAliveEnemyBase() const;
	bool ChooseUnitToBuild(ERiseUnitType& OutType) const;
	int32 CountUnitsOfType(ERiseUnitType Type) const;
	int32 CountCombatUnits() const;
	int32 CountUnitsNear(const FVector& Location, float Radius) const;
	void OrderCombatUnits(AActor* Target, bool bOnlyIdle);
	void OrderCombatUnitsToMove(const FVector& Destination, bool bOnlyIdle);

	bool TryQueueUnit(ERiseUnitType Type);

	UPROPERTY()
	FRiseResourceContainer Resources;

	TWeakObjectPtr<ARiseBase> HomeBase;
	TWeakObjectPtr<AActor> WaveTarget;
	FVector RallyPoint = FVector::ZeroVector;
	bool bAssaulting = false;
	float GatherTimer = 0.0f;
	float ElapsedTime = 0.0f;
	float DecisionTimer = 0.0f;
	float AttackTimer = 0.0f;
	float ReinforceTimer = 0.0f;
	float WorkerAssignTimer = 0.0f;
	float DefenseTimer = 0.0f;
	float StatusTimer = 0.0f;
	int32 WaveCount = 0;
};
