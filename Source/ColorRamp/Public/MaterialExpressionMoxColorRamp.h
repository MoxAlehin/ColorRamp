// Copyright MoxAlehin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Materials/MaterialExpression.h"
#include "MaterialExpressionMoxColorRamp.generated.h"

UENUM()
enum EInterpolationType : uint8
{
    Constant,
    Linear,
    Ease,
    BSpline UMETA(DisplayName="B-Spline")
};

UENUM()
enum EPinType : uint8
{
    HidePins,
    HidePinsDistributed UMETA(DisplayName="Hide Pins (Distributed)"),
    ShowColorPins,
    ShowColorPinsDistributed UMETA(DisplayName="Show Color Pins (Distributed)"),
    ShowPositionPins,
    ShowAllPinsAlternate UMETA(DisplayName="Show All Pins (Alternate)"),
    ShowAllPinsGroup UMETA(DisplayName="Show All Pins (Group)")
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

UCLASS(CollapseCategories, HideCategories = Object, MinimalAPI)
class UMaterialExpressionMoxColorRamp : public UMaterialExpression
{
    GENERATED_UCLASS_BODY()

    UPROPERTY(meta = (RequiredInput = "false", ToolTip = "Defaults to 'ConstAlpha' if not specified"))
    FExpressionInput Alpha;

    UPROPERTY(EditAnywhere, Category = "MaterialExpressionColorRamp", meta = (OverridingInputProperty = "Alpha", UIMin = "0.0", UIMax = "1.0"))
    float ConstAlpha;

    UPROPERTY(EditAnywhere, Category = "MaterialExpressionColorRamp", meta = (DisplayName = "Interpolation Type", ShowAsInputPin = "Advanced"))
    TEnumAsByte<enum EInterpolationType> InterpolationType;

    UPROPERTY(EditAnywhere, Category = "MaterialExpressionColorRamp", meta = (DisplayName = "Pin Type", ShowAsInputPin = "Advanced"))
    TEnumAsByte<enum EPinType> PinType;

    UPROPERTY(EditAnywhere, Category = "MaterialExpressionColorRamp")
    TArray<FColorRampPoint> ColorPoints;

    void RebuildOutputs();
    
#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
    virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex) override;
    virtual void GetCaption(TArray<FString>& OutCaptions) const override;
    virtual TArrayView<FExpressionInput*> GetInputsView() override;
    virtual FExpressionInput* GetInput(int32 InputIndex) override;
    virtual FName GetInputName(int32 InputIndex) const override;
    virtual uint32 GetInputType(int32 InputIndex) override { return MCT_Float; }
    static int32 ApplyEaseInOutInterpolation(FMaterialCompiler* Compiler, int32 AlphaIndex, int32 PrevPositionIndex, int32 PositionIndex);
    int32 ApplyBSplineInterpolation(FMaterialCompiler* Compiler, int32 AlphaIndex, int32 CurrentIndex);
#endif
};
