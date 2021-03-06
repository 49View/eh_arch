//
//  wall_render.hpp
//  sixthmaker
//
//  Created by Dado on 19/03/2017.
//
//

#pragma once

#include <core/resources/resource_types.hpp>
#include <core/math/vector4f.h>

class SceneGraph;
class Renderer;
class ArchRenderController;

struct WallBSData;
struct ArchSegment;
struct MaterialAndColorProperty;

namespace WallRender {
    void IMHouseRender( Renderer& rr, SceneGraph& sg, const WallBSData *mWall, const ArchRenderController& arc );
    GeomSPContainer make3dGeometry( SceneGraph& sg, GeomSP eRootH, const std::vector<ArchSegment>& wss,
                                    const MaterialAndColorProperty& wallMaterial );
};
