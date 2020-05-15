//
// Created by dado on 14/05/2020.
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

namespace KitchenRender {
    void render( SceneGraph& sg, RoomBSData* r, HouseRenderContainer& ret );
}



