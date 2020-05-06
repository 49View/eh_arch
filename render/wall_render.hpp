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

struct WallBSData;
class SceneGraph;
class Renderer;
struct RDSPreMult;
enum class Use2dDebugRendering;

namespace WallRender  {
    void make2dGeometry( Renderer& rr, SceneGraph& sg, const WallBSData *mWall, Use2dDebugRendering bDrawDebug, const RDSPreMult& pm );
    GeomSPContainer make3dGeometry( SceneGraph& sg, const WallBSData* mWall,
                                    const V3fVectorOfVector& ceilingContours );
};
