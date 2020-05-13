//
//  room_render.hpp
//  sixthmaker
//
//  Created by Dado on 19/03/2017.
//
//

#pragma once

#include <core/resources/resource_types.hpp>

class SceneGraph;
struct RoomBSData;
class Renderer;
struct RoomBSData;
struct RDSPreMult;
class HouseRenderContainer;
enum class Use2dDebugRendering;

namespace RoomRender {
    void make2dGeometry( Renderer& rr, SceneGraph& sg, const RoomBSData *data, Use2dDebugRendering bDrawDebug,
                         const RDSPreMult &_pm );
    void make3dGeometry( SceneGraph& sg, RoomBSData* r, HouseRenderContainer& ret );
}

