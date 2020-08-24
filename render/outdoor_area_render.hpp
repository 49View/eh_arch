//
// Created by dado on 19/08/2020.
//

#pragma once

#include "../models/house_bsdata.hpp"
#include <core/resources/resource_types.hpp>

class SceneGraph;
class Renderer;
class ArchRenderController;
struct OutdoorAreaBSData;
struct RDSPreMult;

namespace OutdoorAreaRender {
    void IMHouseRender( Renderer& rr, SceneGraph& sg, const OutdoorAreaBSData *data, const ArchRenderController& arc );
    GeomSPContainer make3dGeometry( SceneGraph& sg, GeomSP eRootH, const OutdoorAreaBSData* mData );
};
