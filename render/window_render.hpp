//
//  window_render.hpp
//  sixthmaker
//
//  Created by Dado on 19/03/2017.
//
//

#pragma once

#include "../models/house_bsdata.hpp"
#include <core/resources/resource_types.hpp>

struct WindowBSData;
class SceneGraph;
class Renderer;
struct RDSPreMult;
enum class FloorPlanRenderMode;

namespace WindowRender {
    void make2dGeometry( Renderer& rr, SceneGraph& sg, const WindowBSData *data, FloorPlanRenderMode fpRenderMode,
                         const RDSPreMult &_pm );
    GeomSPContainer make3dGeometry( SceneGraph& sg, WindowBSData* mData );
};
