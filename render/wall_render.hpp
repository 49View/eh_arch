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
struct RDSPreMult;
class IMHouseRenderSettings;
struct WallBSData;
struct ArchSegment;

namespace WallRender  {
    void IMHouseRender( Renderer& rr, SceneGraph& sg, const WallBSData *mWall, const IMHouseRenderSettings& ims );
    GeomSPContainer make3dGeometry( SceneGraph& sg, const WallBSData* mWall,
                                    const V3fVectorOfVector& ceilingContours );

    GeomSPContainer renderWalls( SceneGraph &sg, const std::vector<ArchSegment> &wss, const std::string &wallMaterial,
                                 const C4f &wallColor );
};
