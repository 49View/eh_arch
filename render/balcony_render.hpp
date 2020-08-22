//
// Created by dado on 19/08/2020.
//

#pragma once

#include "../models/house_bsdata.hpp"
#include <core/resources/resource_types.hpp>

class SceneGraph;
class Renderer;
class ArchRenderController;
struct BalconyBSData;
struct RDSPreMult;

namespace BalconyRender {
    void IMHouseRender( Renderer& rr, SceneGraph& sg, const BalconyBSData *data, const ArchRenderController& arc );
    GeomSPContainer make3dGeometry( SceneGraph& sg, GeomSP eRootH, const BalconyBSData* mData );
};
