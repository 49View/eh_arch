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

    void IMHouseRender( Renderer& rr, SceneGraph& sg, const HouseBSData *mData, const ArchRenderController& arc );

    HouseRenderContainer make3dGeometry( Renderer& rr, SceneGraph& sg, const HouseBSData *mData );
}
