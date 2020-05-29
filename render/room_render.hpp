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
enum class FloorPlanRenderMode;

namespace RoomRender {
    void IMHouseRender( Renderer& rr, SceneGraph& sg, const RoomBSData *data, FloorPlanRenderMode fpRenderMode,
                         const RDSPreMult &_pm );
    void make3dGeometry( SceneGraph& sg, RoomBSData* r, HouseRenderContainer& ret );
}

