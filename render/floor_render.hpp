//
//
//  Created by Dado on 12/03/2017.
//
//

#pragma once

class SceneGraph;
class Renderer;
struct FloorBSData;
struct RDSPreMult;
class HouseRenderContainer;
enum class FloorPlanRenderMode;

namespace FloorRender {
    void make2dGeometry( Renderer& rr, SceneGraph& sg, const FloorBSData *data, FloorPlanRenderMode fpRenderMode,
                         const RDSPreMult &_pm );
    void make3dGeometry( SceneGraph& sg, const FloorBSData* f, HouseRenderContainer& ret );
}
