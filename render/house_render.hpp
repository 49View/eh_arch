//
// Created by Dado on 22/10/2019.
//

#pragma once

#include <core/resources/resource_types.hpp>
#include <eh_arch/models/htypes.hpp>

class ArchRenderController;
class SceneGraph;
class Renderer;
struct DShaderMatrix;
class Vector4f;
class Matrix4f;

struct HouseBSData;

class HouseRenderContainer {
public:
    HouseRenderContainer() = default;
    explicit HouseRenderContainer( std::string  houseId ) : houseId(std::move(houseId)) {}
    std::vector<GeomSP> externalWallsGB;
    std::vector<GeomSP> wallsGB;
    std::vector<GeomSP> covingGB;
    std::vector<GeomSP> skirtingGB;
    std::vector<GeomSP> windowsGB;
    std::vector<GeomSP> outdoorSpacesGB;
    std::vector<GeomSP> doorsGB;
    std::vector<GeomSP> furnituresGB;
    std::vector<GeomSP> floorsGB;
    std::vector<GeomSP> ceilingsGB;
    std::string houseId{};
};

namespace HouseRender {
    void IMHouseDrawSourceDataFloorPlan( Renderer& rr, SceneGraph& sg, const HouseBSData *data, const ArchRenderController& arc );
    void IMHouseRender( Renderer& rr, SceneGraph& sg, const HouseBSData *mData, const ArchRenderController& arc );
    HouseRenderContainer make3dGeometry( Renderer& rr, SceneGraph& sg, const HouseBSData *mData );
}
