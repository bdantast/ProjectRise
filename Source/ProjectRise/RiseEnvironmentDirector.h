#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RiseEnvironmentDirector.generated.h"

class ADirectionalLight;
class ASkyLight;
class AExponentialHeightFog;
class APostProcessVolume;

/**
 * Diretor de ambiente: ajusta a cena do mapa para o "look" do concept art.
 *
 * O concept e uma cena fria de ceu encoberto (slate blue #31455A no topo, #404C4F
 * no meio, #313B37 na base, saturacao ~0.44 e brilho ~0.32). O padrao do UE
 * (sol forte + ceu claro + auto exposure) deixa tudo lavado e sem saturacao,
 * entao aqui o sol fica suave, o ceu/sky light domina, entra fog frio e o
 * pos-processamento trava a exposicao e esfria a imagem.
 *
 * Todos os valores sao UPROPERTY para calibrar rapido comparando a paleta do
 * screenshot com a paleta de Art/Reference/Concept.png (scripts palette.ps1).
 */
UCLASS()
class PROJECTRISE_API ARiseEnvironmentDirector : public AActor
{
	GENERATED_BODY()

public:
	ARiseEnvironmentDirector();

	virtual void BeginPlay() override;

	// Aplica tudo de novo (usado para calibrar em runtime)
	UFUNCTION(BlueprintCallable, Category = "Rise|Visual")
	void ApplyEnvironment();

	// ------------------------------------------------ Sol
	UPROPERTY(EditAnywhere, Category = "Rise|Visual|Sol")
	float SunIntensity = 2.2f;

	UPROPERTY(EditAnywhere, Category = "Rise|Visual|Sol")
	float SunPitch = -38.0f;

	UPROPERTY(EditAnywhere, Category = "Rise|Visual|Sol")
	float SunYaw = 28.0f;

	UPROPERTY(EditAnywhere, Category = "Rise|Visual|Sol")
	FLinearColor SunColor = FLinearColor(1.0f, 0.97f, 0.92f);

	// ------------------------------------------------ Ceu / luz ambiente
	UPROPERTY(EditAnywhere, Category = "Rise|Visual|Ceu")
	float SkyLightIntensity = 1.6f;

	UPROPERTY(EditAnywhere, Category = "Rise|Visual|Ceu")
	FLinearColor SkyLightLowerHemisphere = FLinearColor(0.055f, 0.062f, 0.070f);

	// ------------------------------------------------ Neblina
	UPROPERTY(EditAnywhere, Category = "Rise|Visual|Neblina")
	float FogDensity = 0.012f;

	UPROPERTY(EditAnywhere, Category = "Rise|Visual|Neblina")
	float FogHeightFalloff = 0.12f;

	UPROPERTY(EditAnywhere, Category = "Rise|Visual|Neblina")
	FLinearColor FogInscattering = FLinearColor(0.062f, 0.080f, 0.102f);

	UPROPERTY(EditAnywhere, Category = "Rise|Visual|Neblina")
	bool bVolumetricFog = true;

	// ------------------------------------------------ Exposicao
	// 1.0 = exposicao fixa em EV0; menor = cena mais escura
	UPROPERTY(EditAnywhere, Category = "Rise|Visual|Exposicao")
	float ExposureMinBrightness = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Rise|Visual|Exposicao")
	float ExposureMaxBrightness = 0.5f;

// Botao principal de brilho: +1.0 dobra a luz da cena
	UPROPERTY(EditAnywhere, Category = "Rise|Visual|Exposicao")
	float ExposureBias = 1.3f;

	// ------------------------------------------------ Cor
	UPROPERTY(EditAnywhere, Category = "Rise|Visual|Cor")
	float WhiteTemp = 5200.0f;

	UPROPERTY(EditAnywhere, Category = "Rise|Visual|Cor")
	float ColorSaturation = 1.06f;

	UPROPERTY(EditAnywhere, Category = "Rise|Visual|Cor")
	float ColorContrast = 1.08f;

	UPROPERTY(EditAnywhere, Category = "Rise|Visual|Cor")
	FLinearColor ColorGain = FLinearColor(1.02f, 1.03f, 1.05f);

	UPROPERTY(EditAnywhere, Category = "Rise|Visual|Cor")
	float BloomIntensity = 0.35f;

	UPROPERTY(EditAnywhere, Category = "Rise|Visual|Cor")
	float VignetteIntensity = 0.18f;

private:
	void ConfigureSun();
	void ConfigureSkyLight();
	void ConfigureFog();
	void ConfigurePostProcess();
	void HideLeftoverLevelActors();

	UPROPERTY()
	TWeakObjectPtr<ADirectionalLight> Sun;

	UPROPERTY()
	TWeakObjectPtr<ASkyLight> SkyLight;

	UPROPERTY()
	TWeakObjectPtr<AExponentialHeightFog> Fog;

	UPROPERTY()
	TWeakObjectPtr<APostProcessVolume> PostProcess;
};


