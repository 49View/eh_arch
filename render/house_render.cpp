//
// Created by Dado on 22/10/2019.
//

#include "house_render.hpp"
#include <core/resources/resource_builder.hpp>
#include "../models/floor_service.hpp"
#include "wall_render.hpp"
#include "room_render.hpp"
#include "window_render.hpp"
#include "door_render.hpp"
#include "floor_render.hpp"
#include <poly/vdata_assembler.h>
#include <poly/scene_graph.h>
#include <graphics/ghtypes.hpp>
#include <graphics/renderer.h>

namespace HouseRender {

    void make2dGeometry( Renderer& rr, SceneGraph& sg, const HouseBSData *mData, const RDSPreMult& _pm, Use2dDebugRendering bDrawDebug ) {

        bool drawDebug = bDrawDebug == Use2dDebugRendering::True;

        float padding=0.01f;
        auto houseRect = Rect2f{ 0.0f, 0.0f, mData->bbox.bottomRight().x()+padding, mData->bbox.bottomRight().y()+padding };
        rr.draw<DPoly2d>( houseRect.pointscw(), C4f::WHITE.A( .9f ), _pm);

//        if ( mData->sourceData.floorPlanSize != V2f::ZERO ) {
//            auto floorPlanRect = Rect2f{ 0.0f, 0.0f, mData->sourceData.floorPlanSize.x(),
//                                         mData->sourceData.floorPlanSize.y() };
//            rr.draw<DPoly2d>( floorPlanRect.pointscw(), C4f::WHITE.A( 0.25f ), pm);
//        }

        for ( const auto& f : mData->mFloors ) {
            if ( drawDebug ) {
                for ( const auto& seg : f->orphanedUShapes ) {
                    rr.draw<DCircle2d>( XZY::C( seg.middle ), Color4f::WHITE, 0.1f );
                }
            }

            auto lFloorPath = FloorService::calcPlainPath( f.get());
            for ( const auto& w : f->walls ) {
                WallRender::make2dGeometry( rr, sg, w.get(), bDrawDebug, _pm);
            }
            for ( const auto& w : f->rooms ) {
                RoomRender::make2dGeometry( rr, sg, w.get(), bDrawDebug, _pm);
            }
            for ( const auto& w : f->windows ) {
                WindowRender::make2dGeometry( rr, sg, w.get(), bDrawDebug, _pm);
            }
            for ( const auto& w : f->doors ) {
                DoorRender::make2dGeometry( rr, sg, w.get(), bDrawDebug, _pm);
            }
        }

//        int q = 0;
//        for ( const auto& seg : FloorService::getUSI() ) {
//            if ( q == 0 ) {
//                rr.draw<DLine>( XZY::C(seg.s1->points[1]), XZY::C(seg.s1->points[2]), Color4f::WHITE, 0.1f );
//                rr.draw<DLine>( XZY::C(seg.s2->points[1]), XZY::C(seg.s2->points[2]), Color4f::BLACK, 0.1f );
//                rr.draw<DCircle>( XZY::C(seg.p), Color4f::WHITE, 0.05f );
//            }
//            ++q;
//        }
    }

    HouseRenderContainer make3dGeometry( SceneGraph& sg, const HouseBSData *mData ) {
        HouseRenderContainer ret{};
        sg.addSkybox( mData->defaultSkybox );
        for ( const auto& f : mData->mFloors ) {
            FloorRender::make3dGeometry( sg, f.get(), ret );
        }
        return ret;
    }

}