//
//  wall_render.hpp
//  sixthmaker
//
//  Created by Dado on 19/03/2017.
//
//

#pragma once

#include <core/math/vector3f.h>
#include <core/resources/resource_types.hpp>
#include <core/math/vector4f.h>

struct WallBSData;
class SceneGraph;
class Renderer;
struct RDSPreMult;
enum class FloorPlanRenderMode;
struct ArchSegment;

namespace WallRender  {
    void make2dGeometry( Renderer& rr, SceneGraph& sg, const WallBSData *mWall, FloorPlanRenderMode fpRenderMode, const RDSPreMult& pm );
    GeomSPContainer make3dGeometry( SceneGraph& sg, const WallBSData* mWall,
                                    const V3fVectorOfVector& ceilingContours );

    GeomSPContainer renderWalls( SceneGraph &sg, const std::vector<ArchSegment> &wss, const std::string &wallMaterial,
                                 const C4f &wallColor );
};
