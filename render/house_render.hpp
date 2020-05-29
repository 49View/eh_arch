//
// Created by Dado on 22/10/2019.
//

#pragma once

#include <core/resources/resource_types.hpp>
#include <eh_arch/models/htypes.hpp>
#include <graphics/renderer.h>

class SceneGraph;
class Renderer;
class DShaderMatrix;
class Vector4f;
class Matrix4f;

struct HouseBSData;
struct RDSPreMult;

class HouseRenderContainer {
public:
    std::vector<GeomSP> wallsGB;
    std::vector<GeomSP> covingGB;
    std::vector<GeomSP> skirtingGB;
    std::vector<GeomSP> windowsGB;
    std::vector<GeomSP> doorsGB;
    std::vector<GeomSP> furnituresGB;
    GeomSP floor;
    GeomSP ceiling;
};

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

    RDSPreMult pm{Matrix4f::MIDENTITY()};
    ShowHouseMatrix shwoHouseMatrix;
    FloorPlanRenderMode renderMode = FloorPlanRenderMode::Normal2d;
};

namespace HouseRender {

    DShaderMatrix floorPlanShader( FloorPlanRenderMode fpRenderMode );
    Vector4f floorPlanElemColor( FloorPlanRenderMode fpRenderMode, const Vector4f& nominalColor );
    Vector4f floorPlanElemColor( FloorPlanRenderMode fpRenderMode );
    float floorPlanScaler( FloorPlanRenderMode fpRenderMode, float value, const Matrix4f& pm );

    void IMHouseRender( Renderer& rr, SceneGraph& sg, const HouseBSData *mData, const IMHouseRenderSettings& ims );

    HouseRenderContainer make3dGeometry( SceneGraph& sg, const HouseBSData *mData );
}


