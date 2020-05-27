//
//  door_render.hpp
//  sixthmaker
//
//  Created by Dado on 19/03/2017.
//
//

#pragma once

#include <core/resources/resource_types.hpp>

class SceneGraph;
class Renderer;
struct DoorBSData;
struct RDSPreMult;
enum class FloorPlanRenderMode;

namespace DoorRender {
    void make2dGeometry( Renderer& rr, SceneGraph& sg, const DoorBSData *data, FloorPlanRenderMode fpRenderMode,
                         const RDSPreMult &_pm );
    GeomSPContainer make3dGeometry( SceneGraph& sg, const DoorBSData* mData );
}
