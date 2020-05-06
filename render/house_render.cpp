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
#include <poly/vdata_assembler.h>
#include <poly/scene_graph.h>
#include <graphics/ghtypes.hpp>
#include <graphics/renderer.h>

namespace HouseRender {

    void make2dGeometry( Renderer& rr, SceneGraph& sg, const HouseBSData *mData, const RDSPreMult& _pm, Use2dDebugRendering bDrawDebug ) {

//        for ( const auto& seg : FloorServiceIntermediateData::RCUnconnectedSegments() ) {
//            rr.draw<DLine>( seg.first, seg.second, 0.015f, true, Color4f::RANDA1() );
//        }

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

//        for ( const auto& seg : FloorServiceIntermediateData::WSG() ) {
//            rr.draw<DLine>( seg.p1, seg.p2, 0.035f );
//        }
//        for ( const auto& seg : FloorServiceIntermediateData::WSGE() ) {
//            rr.draw<DLine>( seg.p1, seg.p2, 0.04f, C4f::CYAN );
//        }

    }

    HouseRenderContainer make3dGeometry( SceneGraph& sg, const HouseBSData *mData ) {

        HouseRenderContainer ret{};

        sg.addSkybox( mData->defaultSkybox );
        for ( const auto& f : mData->mFloors ) {
            auto lFloorPath = FloorService::calcPlainPath( f.get());

            for ( const auto& w : f->walls ) {
                auto ws = WallRender::make3dGeometry( sg, w.get(), f->ceilingContours );
                ret.wallsGB.insert( ret.wallsGB.end(), ws.begin(), ws.end());
            }
            for ( const auto& w : f->rooms ) {
                auto wc = RoomRender::createCovingSegments( sg, w.get());
                auto ws = RoomRender::createSkirtingSegments( sg, w.get());
                ret.covingGB.insert( ret.covingGB.end(), wc.begin(), wc.end());
                ret.skirtingGB.insert( ret.skirtingGB.end(), ws.begin(), ws.end());

                auto outline = PolyOutLine{ XZY::C( w->mPerimeterSegments ), V3f::UP_AXIS, 0.1f };
                ret.floor = sg.GB<GT::Extrude>( outline,
                                                V3f{ V3f::UP_AXIS * -0.1f },
                                                GT::M( w->floorMaterial ),
                                                GT::Tag( ArchType::FloorT ));

                for ( const auto& lf : w->mLightFittingsLocators ) {
                    auto spotlightGeom = sg.GB<GT::Asset>( w->spotlightGeom, XZY::C( lf ));
                    auto lKey = ResourceGroup::Light + lf.toString();
                    sg.add<Light>( lKey,
                                   Light{ LightType_Point, w->spotlightGeom, XZY::C( lf ) + V3f::UP_AXIS_NEG * 0.8f,
                                          0.5f, 0.5f, V3f::Y_AXIS * 5.0f } );
                }
            }
            ret.ceiling = sg.GB<GT::Extrude>( PolyOutLine{ XZY::C( f->mPerimeterSegments ), V3f::UP_AXIS, 0.1f },
                                              V3f{ V3f::UP_AXIS * mData->defaultCeilingHeigh },
                                              GT::M( f->defaultCeilingMaterial ),
                                              GT::Tag( ArchType::CeilingT ));
        }

        for ( const auto& f : mData->mFloors ) {
            for ( const auto& w : f->windows ) {
                auto ws = WindowRender::make3dGeometry( sg, w.get());
                ret.windowsGB.insert( ret.windowsGB.end(), ws.begin(), ws.end());
            }
            for ( const auto& w : f->doors ) {
                auto ws = DoorRender::make3dGeometry( sg, w.get());
                ret.doorsGB.insert( ret.doorsGB.end(), ws.begin(), ws.end());
            }
            for ( const auto& w : f->rooms ) {
                for ( const auto& fur : w->mFittedFurniture ) {
                    auto furn = sg.GB<GT::Asset>( fur.name, fur.position3d, GT::Rotate( fur.rotation ));
                    ret.furnituresGB.emplace_back( furn );
                }
            }
        }
        return ret;
    }

}