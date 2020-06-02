//
//
//  Created by Dado on 12/03/2017.
//
//

#pragma once

class SceneGraph;
class Renderer;
struct FloorBSData;
class HouseRenderContainer;
class ArchRenderController;

namespace FloorRender {
    void IMHouseRender( Renderer& rr, SceneGraph& sg, const FloorBSData *data, const ArchRenderController& ims );
    void make3dGeometry( SceneGraph& sg, const FloorBSData* f, HouseRenderContainer& ret );
}
