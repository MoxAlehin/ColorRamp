#pragma once

#include "CoreMinimal.h"
#include "Materials/MaterialExpression.h"
#include "MaterialExpressionColorRamp.generated.h"

UENUM()
enum class EInterpolationType : uint8
{
    Constant,
    Linear,
    Ease
};

USTRUCT()
struct FColorRampPoint
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY()
    FExpressionInput Color;

    UPROPERTY()
    FExpressionInput Position;

    UPROPERTY(EditAnywhere, Category = "ColorRampPoint", meta = (ShowAsInputPin = "Advanced", DisplayName = "Default Color"))
    FLinearColor DefaultColor;

    UPROPERTY(EditAnywhere, Category = "ColorRampPoint", meta = (ShowAsInputPin = "Advanced", DisplayName = "Default Position", UIMin = "0.0", UIMax = "1.0"))
    float DefaultPosition;

    FColorRampPoint()
        : Color(), Position(), DefaultColor(FLinearColor::Black), DefaultPosition(0.0f) {}

    FColorRampPoint(const FExpressionInput& InColor, const FExpressionInput& InPosition, const FLinearColor& InDefaultColor, float InDefaultPosition)
        : Color(InColor), Position(InPosition), DefaultColor(InDefaultColor), DefaultPosition(InDefaultPosition) {}
};

UCLASS(collapsecategories, hidecategories = Object, MinimalAPI)
class UMaterialExpressionColorRamp : public UMaterialExpression
{
    GENERATED_UCLASS_BODY()

public:
    UPROPERTY(meta = (RequiredInput = "false", ToolTip = "Defaults to 'ConstAlpha' if not specified"))
    FExpressionInput Alpha;

    UPROPERTY(EditAnywhere, Category = "MaterialExpressionColorRamp", meta = (OverridingInputProperty = "Alpha", UIMin = "0.0", UIMax = "1.0"))
    float ConstAlpha;

    UPROPERTY(EditAnywhere, Category = "MaterialExpressionColorRamp")
    TArray<FColorRampPoint> ColorPoints;

    UPROPERTY(EditAnywhere, Category = "MaterialExpressionColorRamp")
    EInterpolationType InterpolationType;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
    void RebuildOutputs();
#endif

    virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex) override;
    virtual void GetCaption(TArray<FString>& OutCaptions) const override;
    virtual TArrayView<FExpressionInput*> GetInputsView() override;
    virtual FExpressionInput* GetInput(int32 InputIndex) override;
    virtual FName GetInputName(int32 InputIndex) const override;
    virtual uint32 GetInputType(int32 InputIndex) override { return MCT_Float; }

private:
    int32 ApplyEaseInOutInterpolation(FMaterialCompiler* Compiler, int32 AlphaIndex, int32 PrevPositionIndex, int32 PositionIndex);
};
