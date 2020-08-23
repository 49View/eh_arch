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
enum class FloorPlanRenderMode;

namespace KitchenRender {
    GeomSP render( SceneGraph& sg, GeomSP eRootH, RoomBSData* r );
}



