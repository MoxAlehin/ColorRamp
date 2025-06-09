// Copyright MoxAlehin. All Rights Reserved.

#include "MaterialExpressionMoxColorRamp.h"
#include "MaterialCompiler.h"

#define LOCTEXT_NAMESPACE "MaterialExpression"

UMaterialExpressionMoxColorRamp::UMaterialExpressionMoxColorRamp(const FObjectInitializer& ObjectInitializer)
    :   Super(ObjectInitializer),
        ConstAlpha(1.0f), 
        InterpolationType(EInterpolationType::Linear),
        PinType(EPinType::HidePinsDistributed)
{
    ColorPoints.Add(FColorRampPoint(FExpressionInput(), FExpressionInput(), FLinearColor::Black, 0.0f));
    ColorPoints.Add(FColorRampPoint(FExpressionInput(), FExpressionInput(), FLinearColor::White, 1.0f));
    MenuCategories.Add(LOCTEXT( "Gradient", "Gradient" ));
}

void UMaterialExpressionMoxColorRamp::RebuildOutputs()
{
    Outputs.Reset(1);
    bShowOutputNameOnPin = false;
    Outputs.Add(FExpressionOutput(TEXT("")));
}

#if WITH_EDITOR

int32 UMaterialExpressionMoxColorRamp::Compile(FMaterialCompiler* Compiler, int32 OutputIndex)
{
    int32 AlphaIndex = Alpha.Expression ? Alpha.Compile(Compiler) : Compiler->Constant(ConstAlpha);

    if (ColorPoints.Num() == 0)
    {
        return Compiler->Errorf(TEXT("Color points are missing"));
    }

    if (PinType == EPinType::HidePinsDistributed || PinType == EPinType::ShowColorPinsDistributed)
    {
        for (int32 i = 0; i < ColorPoints.Num(); ++i)
        {
            ColorPoints[i].DefaultPosition = static_cast<float>(i) / (ColorPoints.Num() - 1);
        }
    }

    int32 Result = -1;
    for (int32 i = 0; i < ColorPoints.Num(); ++i)
    {
        int32 ColorIndex = ColorPoints[i].Color.Expression ? ColorPoints[i].Color.Compile(Compiler) : Compiler->Constant3(ColorPoints[i].DefaultColor.R, ColorPoints[i].DefaultColor.G, ColorPoints[i].DefaultColor.B);
        int32 PositionIndex = ColorPoints[i].Position.Expression ? ColorPoints[i].Position.Compile(Compiler) : Compiler->Constant(ColorPoints[i].DefaultPosition);

        if (i == 0)
        {
            Result = ColorIndex;
        }
        else
        {
            int32 PrevPositionIndex = ColorPoints[i - 1].Position.Expression ? ColorPoints[i - 1].Position.Compile(Compiler) : Compiler->Constant(ColorPoints[i - 1].DefaultPosition);
            int32 LerpAlpha;

            switch (InterpolationType)
            {
                case EInterpolationType::Constant:
                    LerpAlpha = Compiler->Step(PositionIndex, AlphaIndex);
                    break;
                case EInterpolationType::Linear:
                    LerpAlpha = Compiler->Div(Compiler->Sub(AlphaIndex, PrevPositionIndex), Compiler->Sub(PositionIndex, PrevPositionIndex));
                    LerpAlpha = Compiler->Clamp(LerpAlpha, Compiler->Constant(0.0f), Compiler->Constant(1.0f));
                    break;
                case EInterpolationType::Ease:
                    LerpAlpha = ApplyEaseInOutInterpolation(Compiler, AlphaIndex, PrevPositionIndex, PositionIndex);
                    break;
                case EInterpolationType::BSpline:
                    LerpAlpha = ApplyBSplineInterpolation(Compiler, AlphaIndex, i);
                    break;
                default:
                    LerpAlpha = Compiler->Div(Compiler->Sub(AlphaIndex, PrevPositionIndex), Compiler->Sub(PositionIndex, PrevPositionIndex));
                    LerpAlpha = Compiler->Clamp(LerpAlpha, Compiler->Constant(0.0f), Compiler->Constant(1.0f));
                    break;
            }

            Result = Compiler->Lerp(Result, ColorIndex, LerpAlpha);
        }
    }

    return Result;
}

int32 UMaterialExpressionMoxColorRamp::ApplyEaseInOutInterpolation(FMaterialCompiler* Compiler, int32 AlphaIndex, int32 PrevPositionIndex, int32 PositionIndex)
{
    int32 t = Compiler->Div(Compiler->Sub(AlphaIndex, PrevPositionIndex), Compiler->Sub(PositionIndex, PrevPositionIndex));
    int32 easeInOut = Compiler->Sub(Compiler->Mul(t, t), Compiler->Mul(Compiler->Sub(t, Compiler->Constant(1.0f)), Compiler->Sub(t, Compiler->Constant(1.0f))));
    return Compiler->Clamp(easeInOut, Compiler->Constant(0.0f), Compiler->Constant(1.0f));
}

// Function to apply B-Spline interpolation
// Ensure there are at least four points for cubic B-Spline
// if (ColorPoints.Num() < 4)
// {
//     return Compiler->Errorf(TEXT("B-Spline interpolation requires at least 4 color points"));
// }

int32 UMaterialExpressionMoxColorRamp::ApplyBSplineInterpolation(FMaterialCompiler* Compiler, int32 AlphaIndex, int32 CurrentIndex)
{
    int32 t = Compiler->Constant(AlphaIndex);  // Placeholder for AlphaIndex as parameter t
    int32 B = Compiler->Constant(0.0f);        // Initialize B for accumulating the B-Spline result

    // Example using cubic B-spline basis; adjust depending on the required control points
    for (int32 i = -1; i <= 2; ++i)
    {
        int32 k = CurrentIndex + i; // Calculate control point index offset
        if (k < 0 || k >= ColorPoints.Num()) continue; // Skip out-of-bounds control points

        int32 PositionIndex = ColorPoints[k].Position.Expression ? ColorPoints[k].Position.Compile(Compiler) : Compiler->Constant(ColorPoints[k].DefaultPosition);
        int32 ColorIndex = ColorPoints[k].Color.Expression ? ColorPoints[k].Color.Compile(Compiler) : Compiler->Constant3(ColorPoints[k].DefaultColor.R, ColorPoints[k].DefaultColor.G, ColorPoints[k].DefaultColor.B);

        int32 BasisValue = Compiler->Constant(1.0f);
        B = Compiler->Add(B, Compiler->Mul(BasisValue, ColorIndex));  // Accumulate weighted color points
    }

    return Compiler->Clamp(B, Compiler->Constant(0.0f), Compiler->Constant(1.0f));
}

void UMaterialExpressionMoxColorRamp::GetCaption(TArray<FString>& OutCaptions) const
{
    OutCaptions.Add(TEXT("Color Ramp"));
}

TArrayView<FExpressionInput*> UMaterialExpressionMoxColorRamp::GetInputsView()
{
    CachedInputs.Empty();
    CachedInputs.Add(&Alpha);

    switch (PinType)
    {
        case EPinType::ShowColorPinsDistributed:
        case EPinType::ShowColorPins:
            for (FColorRampPoint& Point : ColorPoints)
            {
                CachedInputs.Add(&Point.Color);
            }
            break;

        case EPinType::ShowPositionPins:
            for (FColorRampPoint& Point : ColorPoints)
            {
                CachedInputs.Add(&Point.Position);
            }
            break;

        case EPinType::ShowAllPinsGroup:
            for (FColorRampPoint& Point : ColorPoints)
            {
                CachedInputs.Add(&Point.Color);
            }
            for (FColorRampPoint& Point : ColorPoints)
            {
                CachedInputs.Add(&Point.Position);
            }
            break;

        case EPinType::ShowAllPinsAlternate:
            for (FColorRampPoint& Point : ColorPoints)
            {
                CachedInputs.Add(&Point.Color);
                CachedInputs.Add(&Point.Position);
            }
            break;
        default:
            break;
    }

    return CachedInputs;
}

FExpressionInput* UMaterialExpressionMoxColorRamp::GetInput(int32 InputIndex)
{
    if (InputIndex == 0)
    {
        return &Alpha;
    }

    int32 PointIndex;
    switch (PinType)
    {
    case EPinType::ShowColorPinsDistributed:
    case EPinType::ShowColorPins:
        PointIndex = InputIndex - 1;
        if (PointIndex >= 0 && PointIndex < ColorPoints.Num())
        {
            return &ColorPoints[PointIndex].Color;
        }
        break;

    case EPinType::ShowPositionPins:
        PointIndex = InputIndex - 1;
        if (PointIndex >= 0 && PointIndex < ColorPoints.Num())
        {
            return &ColorPoints[PointIndex].Position;
        }
        break;

    case EPinType::ShowAllPinsAlternate:
        PointIndex = (InputIndex - 1) / 2;
        if (PointIndex >= 0 && PointIndex < ColorPoints.Num())
        {
            if ((InputIndex - 1) % 2 == 0)
            {
                return &ColorPoints[PointIndex].Color;
            }
            else
            {
                return &ColorPoints[PointIndex].Position;
            }
        }
        break;

    case EPinType::ShowAllPinsGroup:
        if (InputIndex - 1 < ColorPoints.Num())
        {
            return &ColorPoints[InputIndex - 1].Color;
        }
        else if (InputIndex - 1 < ColorPoints.Num() * 2)
        {
            return &ColorPoints[InputIndex - 1 - ColorPoints.Num()].Position;
        }
        break;

    default:
        break;
    }

    return nullptr;
}

FName UMaterialExpressionMoxColorRamp::GetInputName(int32 InputIndex) const
{
    if (InputIndex == 0)
    {
        return GET_MEMBER_NAME_STRING_CHECKED(UMaterialExpressionMoxColorRamp, Alpha);
    }

    int32 PointIndex;
    switch (PinType)
    {
        case EPinType::ShowColorPinsDistributed:
        case EPinType::ShowColorPins:
            PointIndex = InputIndex - 1;
            if (PointIndex >= 0 && PointIndex < ColorPoints.Num())
            {
                return FName(*FString::Printf(TEXT("Color %d"), PointIndex));
            }
            break;

        case EPinType::ShowPositionPins:
            PointIndex = InputIndex - 1;
            if (PointIndex >= 0 && PointIndex < ColorPoints.Num())
            {
                return FName(*FString::Printf(TEXT("Position %d"), PointIndex));
            }
            break;

        case EPinType::ShowAllPinsAlternate:
            PointIndex = (InputIndex - 1) / 2;
            if (PointIndex >= 0 && PointIndex < ColorPoints.Num())
            {
                if ((InputIndex - 1) % 2 == 0)
                {
                    return FName(*FString::Printf(TEXT("Color %d"), PointIndex));
                }
                else
                {
                    return FName(*FString::Printf(TEXT("Position %d"), PointIndex));
                }
            }
            break;

        case EPinType::ShowAllPinsGroup:
            if (InputIndex - 1 < ColorPoints.Num())
            {
                return FName(*FString::Printf(TEXT("Color %d"), InputIndex - 1));
            }
            else if (InputIndex - 1 < ColorPoints.Num() * 2)
            {
                return FName(*FString::Printf(TEXT("Position %d"), InputIndex - 1 - ColorPoints.Num()));
            }
            break;

        default:
            break;
    }

    return NAME_None;
}

void UMaterialExpressionMoxColorRamp::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    RebuildOutputs();

    if (PropertyChangedEvent.GetMemberPropertyName() == GET_MEMBER_NAME_CHECKED(UMaterialExpressionMoxColorRamp, PinType))
    {
        if (PinType == EPinType::HidePins || 
            PinType == EPinType::ShowPositionPins ||
            PinType == EPinType::HidePinsDistributed)
        {
            for (FColorRampPoint& Point : ColorPoints)
            {
                Point.Color.Expression = nullptr;
            }
        }
        if (PinType == EPinType::HidePins || 
            PinType == EPinType::HidePinsDistributed ||
            PinType == EPinType::ShowColorPinsDistributed ||
            PinType == EPinType::ShowColorPins)
        {
            for (FColorRampPoint& Point : ColorPoints)
            {
                Point.Position.Expression = nullptr;
            }
        }
    }

    if (PropertyChangedEvent.GetMemberPropertyName() == GET_MEMBER_NAME_CHECKED(UMaterialExpressionMoxColorRamp, ColorPoints) ||
        PropertyChangedEvent.GetMemberPropertyName() == GET_MEMBER_NAME_CHECKED(UMaterialExpressionMoxColorRamp, InterpolationType) ||
        PropertyChangedEvent.GetMemberPropertyName() == GET_MEMBER_NAME_CHECKED(UMaterialExpressionMoxColorRamp, PinType))
    {
        if (GraphNode)
        {
            GraphNode->ReconstructNode();
        }
    }

    Super::PostEditChangeProperty(PropertyChangedEvent);
}

#endif

#undef LOCTEXT_NAMESPACE