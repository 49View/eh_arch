//
// Created by dado on 19/08/2020.
//

#include "outdoor_area_render.hpp"

#include <core/math/path_util.h>
#include <core/resources/resource_builder.hpp>
#include <poly/scene_graph.h>
#include <graphics/renderer.h>

#include <eh_arch/controller/arch_render_controller.hpp>

namespace OutdoorAreaRender {

    void IMHouseRender( Renderer& rr, SceneGraph& sg, const OutdoorAreaBSData* w, const ArchRenderController& arc ) {
        auto sm = arc.floorPlanShader();
        auto color = arc.getFillColor(w, Color4f::LIGHT_GREY);
        rr.draw<DFlatPoly>(w->epoints, color, sm, w->hashFeature("outdoorAreaFlatBseIM", 0));
    }

    GeomSPContainer make3dGeometry( SceneGraph& sg, GeomSP eRootH, const OutdoorAreaBSData *w ) {
        auto geom = sg.GB<GT::Extrude>(PolyOutLine{ XZY::C(w->epoints, 0.0f), V3f::UP_AXIS, w->floorHeight }, w->Position(), w->externalFloorMaterial, eRootH);
        GeomSPContainer ret;
        ret.emplace_back(geom);
        return ret;
    }
}