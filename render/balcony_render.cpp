//
// Created by dado on 19/08/2020.
//

#include "balcony_render.hpp"

#include <core/math/path_util.h>
#include <core/resources/resource_builder.hpp>
#include <poly/scene_graph.h>
#include <graphics/renderer.h>

#include <eh_arch/controller/arch_render_controller.hpp>

namespace BalconyRender {

    void IMHouseRender( Renderer& rr, SceneGraph& sg, const BalconyBSData* w, const ArchRenderController& arc ) {
        auto sm = arc.floorPlanShader();
        auto color = arc.getFillColor(w, Color4f::LIGHT_GREY);
        rr.draw<DFlatPoly>(w->epoints, color, sm, w->hashFeature("balconyFlatBseIM", 0));
    }

    GeomSPContainer make3dGeometry( SceneGraph& sg, const BalconyBSData *w ) {
        auto mRootH = EF::create<Geom>("Balcony");

        sg.GB<GT::Extrude>(PolyOutLine{ XZY::C(w->epoints, w->z), V3f::UP_AXIS, w->floorHeight }, w->balconyFloorMaterial, mRootH);

        GeomSPContainer ret;
        ret.emplace_back(mRootH);
        sg.addNode(mRootH);
        return ret;
    }
}