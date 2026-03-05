/*#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "IGolfHoleProvider.generated.h"

//does the interface provide the graph, paths, or just start/end + raw geo?   I would lean towards just start and end first
// struct FGolfHoleGraph
// {
//     TArray<FGolfHoleVert> Verts;
//     TArray<FGolfHolePath> Paths;
// };


// struct FGolfHolePath
// {
//     GENERATED_BODY()
    
//     TArray<int32> PathEdges;
// };

// struct FGolfHoleVert
// {
//     int32 index;
//     FVector Location;
//     float SafeTargetRadius;
//     TArray<int32> OutEdges;
// }


UINTERFACE(MinimalAPI, BlueprintType)
class UGolfHoleProvider : public UInterface
{
    GENERATED_BODY()
};

class VRGOLFTESTING_API IGolfHoleProvider
{
    GENERATED_BODY()
    
public:

    


    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ISM Runtime|Conversion")
    FVector GetTeeLocation() const;
    FVector GetTeeLocation_Implementation()const { return FVector::Zero; };

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ISM Runtime|Conversion")
    FVector GetHoleLocation() const;
    FVector GetHoleLocation_Implementation()const { return FVector::Zero; };
    
}*/