//
// Created by dado on 30/05/2020.
//

#pragma once

#include <eh_arch/models/htypes.hpp>
#include <graphics/renderer.h>

struct HouseBSData;

namespace ShowHouseMatrixFlags {
    static constexpr uint64_t None = 0;
    static constexpr uint64_t Show2dFloorPlan = 1u << 1u;
    static constexpr uint64_t Show3dFloorPlan = 1u << 2u;
    static constexpr uint64_t Show3dHouse = 1u << 3u;
    static constexpr uint64_t UseDebugMode = 1u << 4u;
};

using ShowHouseMatrixFlagsT = uint64_t;

class ShowHouseMatrix {
public:
    ShowHouseMatrix() = default;
    ShowHouseMatrix( ShowHouseMatrixFlagsT flags ) : flags(flags) {}
    ShowHouseMatrix( ShowHouseMatrixFlagsT flags, float fp2DScreenRatio, float fp2DScreenPadding ) : flags(flags),
                                                                                                     fp2dScreenRatio(
                                                                                                             fp2DScreenRatio),
                                                                                                     fp2dScreenPadding(
                                                                                                             fp2DScreenPadding) {}
    [[nodiscard]] float getFp2DScreenRatio() const {
        return fp2dScreenRatio;
    }
    void setFp2DScreenRatio( float fp2DScreenRatio ) {
        fp2dScreenRatio = fp2DScreenRatio;
    }
    [[nodiscard]] float getFp2DScreenPadding() const {
        return fp2dScreenPadding;
    }
    void setFp2DScreenPadding( float fp2DScreenPadding ) {
        fp2dScreenPadding = fp2DScreenPadding;
    }
    [[nodiscard]] ShowHouseMatrixFlagsT getFlags() const {
        return flags;
    }
    void setFlags( ShowHouseMatrixFlagsT _flags ) {
        flags = _flags;
    }
private:
    ShowHouseMatrixFlagsT flags = ShowHouseMatrixFlags::None;
    float fp2dScreenRatio = 4.0f;
    float fp2dScreenPadding = 0.02f;
};

class IMHouseRenderSettings {
public:
    IMHouseRenderSettings( const RDSPreMult& pm, FloorPlanRenderMode renderMode );
    IMHouseRenderSettings( FloorPlanRenderMode renderMode );

    [[nodiscard]] FloorPlanRenderMode renderMode() const;
    void renderMode( FloorPlanRenderMode rm );
    [[nodiscard]]const RDSPreMult& pm() const;
    void pm( const RDSPreMult& pm );

    DShaderMatrix floorPlanShader() const;
    float floorPlanScaler( float value ) const;
    C4f floorPlanElemColor( const C4f& nominalColor ) const;
    C4f floorPlanElemColor() const;
    bool isFloorPlanRenderModeDebug() const;
    bool isFloorPlanRenderMode2d() const;

    void addToSelectionList( int64_t hash );
    void addToFeatureSelectionList( const ArchStructuralFeatureIndex& asfi );

    template<typename T>
    C4f getFillColor( const T& elem, const C4f& c1, const C4f& c2 ) const {

        if constexpr ( std::is_same_v<T, int64_t> ) {
            if ( auto it = selection.find(elem); it != selection.end() ) {
                return selectedColor;
            }
        }

        if constexpr ( std::is_same_v<T, ArchStructuralFeatureIndex> ) {
            if ( auto it = featuresSelection.find(elem); it != featuresSelection.end() ) {
                return selectedColor;
            }
        }

        if ( isFloorPlanRenderModeDebug() ) {
            return c1;
        }

        return c2;
    }

    template<typename T>
    C4f getFillColor( const T& elem, const C4f& c1 ) const {
        return getFillColor(elem, c1, c1);
    }

private:
    std::unordered_set<int64_t> selection;
    std::unordered_set<ArchStructuralFeatureIndex, ArchStructuralFeatureIndexHashFunctor> featuresSelection;
    RDSPreMult mPm{ Matrix4f::MIDENTITY() };
    FloorPlanRenderMode mRenderMode = FloorPlanRenderMode::Normal2d;
    C4f selectedColor = C4f::ORANGE_SCHEME1_1;
};