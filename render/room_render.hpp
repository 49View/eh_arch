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
class IMHouseRenderSettings;

namespace RoomRender {
    void IMHouseRender( Renderer& rr, SceneGraph& sg, const RoomBSData *data, const IMHouseRenderSettings& ims );
    void make3dGeometry( SceneGraph& sg, RoomBSData* r, HouseRenderContainer& ret );
}

