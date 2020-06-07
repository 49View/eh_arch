//
//
//  Created by Dado on 19/03/2017.
//
//

#include "floor_render.hpp"

#include <core/math/path_util.h>
#include <core/resources/resource_builder.hpp>
#include <poly/scene_graph.h>
#include <poly/vdata_assembler.h>
#include <poly/poly_services.hpp>
#include <graphics/renderer.h>

#include <eh_arch/controller/arch_render_controller.hpp>
#include "../models/house_bsdata.hpp"
#include "house_render.hpp"
#include "wall_render.hpp"
#include "room_render.hpp"
#include "window_render.hpp"
#include "door_render.hpp"

namespace FloorRender {

    void IMHouseRender( Renderer &rr, SceneGraph &sg, const FloorBSData *f, const ArchRenderController& ims ) {
        bool drawDebug = isFloorPlanRenderModeDebug(ims.renderMode());
        auto rm = ims.floorPlanShader();

        if ( drawDebug ) {
            int ousc = 0;
            for ( const auto& seg : f->orphanedUShapes ) {
                rr.draw<DCircle>(XZY::C(seg.middle), Color4f::WHITE, rm, 0.075f, ims.pm(), seg.hashFeature("orphanedUshape", ousc++));
            }
            for ( const auto& seg : f->orphanedWallSegments ) {
                rr.draw<DLine>(XZY::C(seg.p1), XZY::C(seg.p2), Color4f::RED, rm, 0.075f, ims.pm(), f->hashFeature("orphanedWallSegments", ousc++));
            }
        }

        for ( const auto& w : f->walls ) {
            WallRender::IMHouseRender(rr, sg, w.get(), ims);
        }
        for ( const auto& w : f->rooms ) {
            RoomRender::IMHouseRender(rr, sg, w.get(), ims);
        }
        for ( const auto& w : f->windows ) {
            WindowRender::IMHouseRender(rr, sg, w.get(), ims);
        }
        for ( const auto& w : f->doors ) {
            DoorRender::IMHouseRender(rr, sg, w.get(), ims);
        }
    }

    void make3dGeometry( SceneGraph &sg, const FloorBSData *f, HouseRenderContainer &ret ) {

        // External walls of this floor
        WallRender::renderWalls( sg, f->perimeterArchSegments, f->externalWallsMaterial, f->externalWallsColor );
        // Ceilings
        ret.ceiling = sg.GB<GT::Extrude>( PolyOutLine{ XZY::C( f->mPerimeterSegments ), V3f::UP_AXIS, 0.1f },
                                          V3f{ V3f::UP_AXIS * f->height },
                                          GT::M( f->defaultCeilingMaterial ),
                                          f->defaultCeilingColor,
                                          GT::Tag( ArchType::CeilingT ));

        for ( const auto &w : f->rooms ) {
            RoomRender::make3dGeometry( sg, w.get(), ret );
        }
        for ( const auto &w : f->windows ) {
            auto ws = WindowRender::make3dGeometry( sg, w.get());
            ret.windowsGB.insert( ret.windowsGB.end(), ws.begin(), ws.end());
        }
        for ( const auto &w : f->doors ) {
            auto ws = DoorRender::make3dGeometry( sg, w.get());
            ret.doorsGB.insert( ret.doorsGB.end(), ws.begin(), ws.end());
        }
    }
}
