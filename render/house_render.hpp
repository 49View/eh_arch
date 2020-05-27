//
// Created by Dado on 22/10/2019.
//

#pragma once

#include <core/resources/resource_types.hpp>
#include <eh_arch/models/htypes.hpp>

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

namespace HouseRender {

    DShaderMatrix floorPlanShader( FloorPlanRenderMode fpRenderMode );
    Vector4f floorPlanElemColor( FloorPlanRenderMode fpRenderMode, const Vector4f& nominalColor );
    Vector4f floorPlanElemColor( FloorPlanRenderMode fpRenderMode );
    float floorPlanScaler( FloorPlanRenderMode fpRenderMode, float value, const Matrix4f& pm );

    void make2dGeometry( Renderer& rr, SceneGraph& sg, const HouseBSData *mData, const RDSPreMult& _pm,
                         FloorPlanRenderMode fpRenderMode = FloorPlanRenderMode::Normal2d );

    HouseRenderContainer make3dGeometry( SceneGraph& sg, const HouseBSData *mData );
}


