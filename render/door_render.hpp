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
class ArchRenderController;
struct DoorBSData;
struct RDSPreMult;

namespace DoorRender {
    void IMHouseRender( Renderer& rr, SceneGraph& sg, const DoorBSData *data, const ArchRenderController& arc );
    GeomSP make3dGeometry( SceneGraph& sg, const GeomSP& eRootH, const DoorBSData* mData );
}
