#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "RiseSelectableInterface.generated.h"

UINTERFACE(MinimalAPI)
class URiseSelectableInterface : public UInterface
{
    GENERATED_BODY()
};

class PROJECTRISE_API IRiseSelectableInterface
{
    GENERATED_BODY()

public:
    // Funções chamadas ao selecionar ou desselecionar a unidade
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Selection")
    void SelectEntity();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Selection")
    void DeselectEntity();
};
