#include "MaterialExpressionColorRamp.h"
#include "MaterialCompiler.h"

UMaterialExpressionColorRamp::UMaterialExpressionColorRamp(const FObjectInitializer& ObjectInitializer)
    :   Super(ObjectInitializer),
        ConstAlpha(1.0f), 
        InterpolationType(EInterpolationType::Linear),
        PinType(EPinType::HidePins)
{
    ColorPoints.Add(FColorRampPoint(FExpressionInput(), FExpressionInput(), FLinearColor::Black, 0.0f));
    ColorPoints.Add(FColorRampPoint(FExpressionInput(), FExpressionInput(), FLinearColor::White, 1.0f));
}

int32 UMaterialExpressionColorRamp::Compile(FMaterialCompiler* Compiler, int32 OutputIndex)
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

int32 UMaterialExpressionColorRamp::ApplyEaseInOutInterpolation(FMaterialCompiler* Compiler, int32 AlphaIndex, int32 PrevPositionIndex, int32 PositionIndex)
{
    int32 t = Compiler->Div(Compiler->Sub(AlphaIndex, PrevPositionIndex), Compiler->Sub(PositionIndex, PrevPositionIndex));
    int32 easeInOut = Compiler->Sub(Compiler->Mul(t, t), Compiler->Mul(Compiler->Sub(t, Compiler->Constant(1.0f)), Compiler->Sub(t, Compiler->Constant(1.0f))));
    return Compiler->Clamp(easeInOut, Compiler->Constant(0.0f), Compiler->Constant(1.0f));
}

void UMaterialExpressionColorRamp::GetCaption(TArray<FString>& OutCaptions) const
{
    OutCaptions.Add(TEXT("Color Ramp"));
}

TArrayView<FExpressionInput*> UMaterialExpressionColorRamp::GetInputsView()
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

FExpressionInput* UMaterialExpressionColorRamp::GetInput(int32 InputIndex)
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

FName UMaterialExpressionColorRamp::GetInputName(int32 InputIndex) const
{
    if (InputIndex == 0)
    {
        return GET_MEMBER_NAME_STRING_CHECKED(UMaterialExpressionColorRamp, Alpha);
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

#if WITH_EDITOR
void UMaterialExpressionColorRamp::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    RebuildOutputs();

    if (PropertyChangedEvent.GetMemberPropertyName() == GET_MEMBER_NAME_CHECKED(UMaterialExpressionColorRamp, PinType))
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

    if (PropertyChangedEvent.GetMemberPropertyName() == GET_MEMBER_NAME_CHECKED(UMaterialExpressionColorRamp, ColorPoints) ||
        PropertyChangedEvent.GetMemberPropertyName() == GET_MEMBER_NAME_CHECKED(UMaterialExpressionColorRamp, InterpolationType) ||
        PropertyChangedEvent.GetMemberPropertyName() == GET_MEMBER_NAME_CHECKED(UMaterialExpressionColorRamp, PinType))
    {
        if (GraphNode)
        {
            GraphNode->ReconstructNode();
        }
    }

    Super::PostEditChangeProperty(PropertyChangedEvent);
}

void UMaterialExpressionColorRamp::RebuildOutputs()
{
    Outputs.Reset(1);
    bShowOutputNameOnPin = false;
    Outputs.Add(FExpressionOutput(TEXT("")));
}
#endif
