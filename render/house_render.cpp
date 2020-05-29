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

    void IMHouseRender( Renderer& rr, SceneGraph& sg, const HouseBSData *data, const IMHouseRenderSettings& ims ) {

        rr.clearBucket(CommandBufferLimits::UI2dStart);

        bool drawDebug = isFloorPlanRenderModeDebug(ims.renderMode());
        auto rm = ims.floorPlanShader();

        // We have 3 combinations here:
        // 1) It's a 3d floorPlan with a source image, render the source image as a background
        // 2) it's a 2d floorPlan so no images allowed, render a flat poly
        // 3) it's a 3d floorPlan but it hasn't got a source image, (IE not HouseMakerBitmap), renders a flat poly
        if ( data->sourceData.floorPlanSize != V2fc::ZERO && !isFloorPlanRenderMode2d(ims.renderMode()) ) {
            // 1)
            auto floorPlanRect = Rect2f{ 0.0f, 0.0f, data->sourceData.floorPlanSize.x(),
                                         data->sourceData.floorPlanSize.y() };
            rr.draw<DRect>(floorPlanRect, C4f::WHITE.A(.3f), RDSImage(data->sourceData.floorPlanSourceName),
                           RDSRectAxis::XZ, "floorplanImage");
        } else if ( isFloorPlanRenderMode2d(ims.renderMode()) ) {
            // 2)
            float padding = 0.01f;
            auto houseRect = Rect2f{ 0.0f, 0.0f, data->bbox.bottomRight().x() + padding,
                                     data->bbox.bottomRight().y() + padding };
            rr.draw<DPoly>(rm, houseRect.pointscw(), C4f::WHITE.A(.5f), ims.pm());
        } else {
            // 3)
            rr.draw<DRect>(data->bbox, C4f::WHITE.A(.3f), RDSRectAxis::XZ);
        }

        for ( const auto& f : data->mFloors ) {
            if ( drawDebug ) {
                for ( const auto& seg : f->orphanedUShapes ) {
                    rr.draw<DCircle>(XZY::C(seg.middle), Color4f::WHITE, rm, 0.1f, ims.pm());
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

    HouseRenderContainer make3dGeometry( SceneGraph& sg, const HouseBSData *data ) {
        HouseRenderContainer ret{};
        sg.addSkybox(data->defaultSkybox);
        for ( const auto& f : data->mFloors ) {
            FloorRender::make3dGeometry(sg, f.get(), ret);
        }
        return ret;
    }
}

DShaderMatrix IMHouseRenderSettings::floorPlanShader() const {
    return isFloorPlanRenderMode2d() ? DShaderMatrix{ DShaderMatrixValue2dColor } : DShaderMatrix{
            DShaderMatrixValue3dColor };
}

float IMHouseRenderSettings::floorPlanScaler( float value ) const {
    if ( isFloorPlanRenderMode2d() ) {
        return max(mPm()[0] * value, 1.0f / getScreenSizef.y());
    }
    return value;
}

Color4f IMHouseRenderSettings::floorPlanElemColor( const C4f& nominalColor ) const {
    return isFloorPlanRenderModeDebug() ? nominalColor : C4f::BLACK;
}

Color4f IMHouseRenderSettings::floorPlanElemColor() const {
    return isFloorPlanRenderModeDebug() ? Color4f::RANDA1() : C4f::BLACK;
}

IMHouseRenderSettings::IMHouseRenderSettings( const RDSPreMult& pm, FloorPlanRenderMode renderMode ) : mPm(pm),
                                                                                                       mRenderMode(
                                                                                                               renderMode) {}
IMHouseRenderSettings::IMHouseRenderSettings( FloorPlanRenderMode renderMode ) : mRenderMode(renderMode) {}

FloorPlanRenderMode IMHouseRenderSettings::renderMode() const {
    return mRenderMode;
}

bool IMHouseRenderSettings::isFloorPlanRenderModeDebug() const {
    return ::isFloorPlanRenderModeDebug(mRenderMode);
}

bool IMHouseRenderSettings::isFloorPlanRenderMode2d() const {
    return ::isFloorPlanRenderMode2d(mRenderMode);
}

void IMHouseRenderSettings::renderMode( FloorPlanRenderMode rm ) {
    mRenderMode = rm;
}
const RDSPreMult& IMHouseRenderSettings::pm() const {
    return mPm;
}

void IMHouseRenderSettings::pm( const RDSPreMult& pm ) {
    mPm = pm;
}

void IMHouseRenderSettings::addToSelectionList( int64_t hash ) {
    selection.emplace(hash);
}
