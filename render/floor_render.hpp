//
//
//  Created by Dado on 12/03/2017.
//
//

#pragma once

#include <core/resources/resource_types.hpp>

class SceneGraph;
class Renderer;
struct FloorBSData;
class ArchRenderController;

namespace FloorRender {
    void IMHouseRender( Renderer& rr, SceneGraph& sg, const FloorBSData *data, const ArchRenderController& arc );
    GeomSP make3dGeometry( SceneGraph& sg, GeomSP eRootH, const FloorBSData* f );
}
