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
        int cc = 0;
        for ( const auto& oa : w->Boundaries() ) {
            auto color = arc.getFillColor(w, Color4f::LIGHT_GREY);
            rr.draw<DFlatPoly>(oa.bPoints, color, sm, w->hashFeature("outdoorAreaFlatBseIM", cc++));
        }
    }

    GeomSP make3dGeometry( SceneGraph& sg, GeomSP eRootH, const OutdoorAreaBSData *w ) {
        auto lRootH = eRootH->addChildren("OutdoorArea"+ std::to_string(w->hash));

        for ( const auto& oa : w->Boundaries() ) {
            sg.GB<GT::Extrude>(PolyOutLine{ XZY::C(oa.bPoints, 0.0f), V3f::UP_AXIS, oa.zPull },
                               w->Position(), oa.outdoorBoundaryMaterial, lRootH);
        }

        return lRootH;
    }
}