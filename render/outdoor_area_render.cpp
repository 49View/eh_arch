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
            if ( oa.extrusionType == 0 ) {
                auto color = arc.getFillColor(w, C4fc::LIGHT_GREY);
                rr.draw<DFlatPoly>(oa.bPoints, color, sm, w->hashFeature("outdoorAreaFlatBseIM", cc++));
            } else if ( oa.extrusionType == 1 ) {
                auto color = arc.getFillColor(w, C4fc::SKY_BLUE);
                rr.draw<DLine>(oa.bPoints, color, sm, w->hashFeature("outdoorAreaFlatBseIM", cc++), oa.followerWidth);
            }
        }
    }

    std::shared_ptr<Profile> makeTopBumpProfile( const std::string& _name, const V3f& size ) {
        return ProfileMaker{ _name }.sd(10).o().ly(size.y()).ax(size.x(), 10).ly(-size.y()).make();
    }

    std::shared_ptr<Profile> makeBevelTopProfile( const std::string& _name, const V3f& size ) {
        float bevelRadius = size.x() * 0.4f;
        float topWidth = size.x() - bevelRadius * 2.0f;
        return ProfileMaker{ _name }.sd(10).o().ly(size.y()).axHalfLeft(bevelRadius, 4).lx(topWidth).axHalfRight(bevelRadius, 4).ly(-size.y()).make();
    }

    GeomSP make3dGeometry( SceneGraph& sg, GeomSP eRootH, const OutdoorAreaBSData *w ) {
        auto lRootH = eRootH->addChildren("OutdoorArea"+ std::to_string(w->hash));

        for ( const auto& oa : w->Boundaries() ) {
            if ( oa.extrusionType == 0 ) {
                sg.GB<GT::Extrude>(PolyOutLine{ XZY::C(oa.bPoints, oa.elevation), V3f::UP_AXIS, oa.zPull },
                                   w->Position(), oa.outdoorBoundaryMaterial, lRootH);
            } else if ( oa.extrusionType == 1 ) {
                auto profile = makeBevelTopProfile("Outdoor1", V3f{oa.followerWidth, oa.zPull, 0.0f});
                sg.GB<GT::Follower>( profile,  XZY::C(oa.bPoints, oa.elevation), w->Position(), oa.outdoorBoundaryMaterial, lRootH);
            }
        }

        return lRootH;
    }
}