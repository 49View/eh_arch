//
//
//  Created by Dado on 19/03/2017.
//
//

#include "floor_render.hpp"

#include <core/math/path_util.h>
#include <poly/scene_graph.h>
#include <graphics/renderer.h>

#include <eh_arch/controller/arch_render_controller.hpp>
#include "wall_render.hpp"
#include "room_render.hpp"
#include "window_render.hpp"
#include "door_render.hpp"
#include "outdoor_area_render.hpp"

namespace FloorRender {

    void IMHouseRender( Renderer& rr, SceneGraph& sg, const FloorBSData *f, const ArchRenderController& arc ) {
        bool drawDebug = isFloorPlanRenderModeDebug(arc.renderMode());
        auto sm = arc.floorPlanShader();

        if ( drawDebug ) {
            int oCounter = 0;
            for ( const auto& seg : f->orphanedUShapes ) {
                rr.draw<DCircle>(XZY::C(seg.middle), C4fc::WHITE, sm, 0.075f, arc.pm(),
                                 seg.hashFeature("orphanedUShape" + sm.hash(), oCounter++));
            }
            for ( const auto& seg : f->orphanedWallSegments ) {
                rr.draw<DLine>(XZY::C(seg.p1), XZY::C(seg.p2), C4fc::RED, sm, 0.075f, arc.pm(),
                               f->hashFeature("orphanedWallSegments" + sm.hash(), oCounter++));
            }
        }

        for ( const auto& w : f->outdoorAreas ) {
            OutdoorAreaRender::IMHouseRender(rr, sg, w.get(), arc);
        }
        for ( const auto& w : f->walls ) {
            WallRender::IMHouseRender(rr, sg, w.get(), arc);
        }
        for ( const auto& w : f->windows ) {
            WindowRender::IMHouseRender(rr, sg, w.get(), arc);
        }
        for ( const auto& w : f->doors ) {
            DoorRender::IMHouseRender(rr, sg, w.get(), arc);
        }
        for ( const auto& w : f->rooms ) {
            RoomRender::IMHouseRender(rr, sg, w.get(), arc);
        }
    }

    GeomSP make3dGeometry( SceneGraph& sg, GeomSP eRootH, const FloorBSData *f ) {
        auto lRootH = EF::create<Geom>("Floor"+ std::to_string(f->hash));

        // External walls of this floor
        WallRender::make3dGeometry(sg, lRootH, f->perimeterArchSegments, f->externalWallsMaterial);

        for ( const auto& w : f->windows ) {
            WindowRender::make3dGeometry(sg, lRootH, w.get());
        }
        for ( const auto& w : f->doors ) {
            DoorRender::make3dGeometry(sg, lRootH, w.get());
        }
        for ( const auto& w : f->outdoorAreas ) {
            OutdoorAreaRender::make3dGeometry(sg, lRootH, w.get());
        }
        for ( const auto& w : f->rooms ) {
            RoomRender::make3dGeometry(sg, lRootH, w.get());
        }

        return eRootH->addChildren(lRootH);
    }
}
