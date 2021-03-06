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

class SceneGraph;
class Renderer;
class ArchRenderController;
struct WindowBSData;
struct RDSPreMult;

namespace WindowRender {
    void IMHouseRender( Renderer& rr, SceneGraph& sg, const WindowBSData *data, const ArchRenderController& arc );
    GeomSP make3dGeometry( SceneGraph& sg, GeomSP eRootH, WindowBSData* mData );
};
