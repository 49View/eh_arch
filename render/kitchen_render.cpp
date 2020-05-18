//
// Created by dado on 14/05/2020.
//

#include "kitchen_render.hpp"
#include <core/math/path_util.h>
#include <core/resources/profile.hpp>
#include <core/v_data.hpp>
#include <core/resources/resource_builder.hpp>
#include <poly/scene_graph.h>
#include <poly/vdata_assembler.h>
#include <poly/follower.hpp>
#include "../models/house_bsdata.hpp"
#include "../models/room_service.hpp"
#include "house_render.hpp"

namespace KitchenRender {

    void drawFlatDoubleDrawer( SceneGraph& sg, KitchenData& kd, const KitchenDrawer& kup ) {
        auto rotation = Quaternion{ RoomService::furnitureAngleFromNormal(kup.normal), V3f::UP_AXIS };
        float dp = kd.drawersPadding.x();
        float unitYSizeAvailable = kup.unitHeight;
        for ( int m = 0; m < 2; m++ ) {
            float unitHeight = m == 0 ? unitYSizeAvailable * 0.75f : unitYSizeAvailable * .25f;
            float uz = kup.z + ( m == 0 ? 0.0f : unitYSizeAvailable * 0.75f + dp );
            auto linex = FollowerService::createLinePath(kup.p1, kup.p2, kd.drawersThickness, uz);
            sg.GB<GT::Extrude>(PolyOutLine{ linex, V3f::UP_AXIS, unitHeight },
                               GT::M(kd.unitsMaterial));
            auto handleWidth = sg.GM(kd.drawersHandleModel)->BBox3d()->calcWidth();
            float drawerWidth = distance(kup.p1, kup.p2);
            if ( drawerWidth > handleWidth * 1.2f ) {
                auto rotationHandle = rotation;
                V3f mixHandlePos = { 0.5f, 0.5f, 0.5f };
                if ( unitHeight > drawerWidth ) {
                    auto rotationZ = Quaternion{ M_PI_2, V3f::Z_AXIS };
                    rotationHandle = rotationZ * rotation;
                    mixHandlePos.setX(0.15f);
                    mixHandlePos.setY(0.15f);
                }
                sg.GB<GT::Asset>(kd.drawersHandleModel,
                                 XZY::C(mix(kup.p1, kup.p2, mixHandlePos) + kup.normal * kd.drawersThickness * 0.5f,
                                        uz + unitHeight * 0.5f), GT::Rotate(rotationHandle));
            }
        }
    }

    void drawFillerDrawer( SceneGraph& sg, KitchenData& kd, const KitchenDrawer& kuw ) {
        auto linex = FollowerService::createLinePath(kuw.p1, kuw.p2, kd.drawersThickness, kuw.z);
        sg.GB<GT::Extrude>(PolyOutLine{ linex, V3f::UP_AXIS, kuw.unitHeight }, GT::M(kd.unitsMaterial), kuw.color);
    }

    void render( SceneGraph& sg, RoomBSData *w, HouseRenderContainer& ret ) {
        KitchenData kd = w->kitchenData;

        for ( const auto& kwd : kd.kitchenDrawers ) {
            switch ( kwd.type) {
                case KitchenDrawerType::Filler:
                    drawFillerDrawer(sg, kd, kwd);
                    break;
                case KitchenDrawerType::FlatBasic:
                    drawFlatDoubleDrawer(sg, kd, kwd);
                    break;
            }
        }

        for ( const auto& kwp : kd.kitchenWorktopPath ) {
            if ( !kwp.flags.hasSink ) {
                auto linex = FollowerService::createLinePath(kwp.p1, kwp.p2, kd.kitchenWorktopDepth,
                                                             kd.kitchenWorktopHeight);
                sg.GB<GT::Extrude>(PolyOutLine{ linex, V3f::UP_AXIS, kd.worktopThickness }, GT::M(kd.worktopMaterial));
            } else {
                // I've decided to split the worktop in 4 pieces (a'la having a frame around the sink)
                // instead of clipping an hole through it as I still don't trust booleans they are still too risky
                auto sinkHalfDepth = sg.GM(kd.sinkModel)->BBox3d()->calcDepth() * 0.5f;
                // Add some filling to make sure the whole sink is cover, ie to cover up round edges
                auto sinkHalfWidth = ( sg.GM(kd.sinkModel)->BBox3d()->calcWidth() * 0.5f ) - 0.02f;
                auto kn = normalize(kwp.p2 - kwp.p1);
                auto sinko1 = kwp.sinkPos + kwp.normal * kd.kitchenWorktopDepth * 0.5f;
                auto knhw = kn * sinkHalfWidth;
                auto sinkp1 = sinko1 - knhw;
                auto sinkp2 = sinko1 + knhw;

                // Left side of the worktop
                auto linex = FollowerService::createLinePath(kwp.p1, sinkp1, kd.kitchenWorktopDepth,
                                                             kd.kitchenWorktopHeight);
                sg.GB<GT::Extrude>(PolyOutLine{ linex, V3f::UP_AXIS, kd.worktopThickness }, GT::M(kd.worktopMaterial));

                // Right side of the worktop
                auto linex2 = FollowerService::createLinePath(sinkp2, kwp.p2, kd.kitchenWorktopDepth,
                                                              kd.kitchenWorktopHeight);
                sg.GB<GT::Extrude>(PolyOutLine{ linex2, V3f::UP_AXIS, kd.worktopThickness }, GT::M(kd.worktopMaterial));

                // Add some filling (0.02f) for the top/bottom also
                float topAndBottomDepth = ( ( kd.kitchenWorktopDepth - sinkHalfDepth * 2.0f ) * 0.5f ) + 0.02f;

                // Top side of the worktop
                auto topSideOffset = ( kwp.normal * ( kd.kitchenWorktopDepth * 0.5f - topAndBottomDepth * 0.5f ) );
                auto sinkp3a = sinkp1 + topSideOffset;
                auto sinkp3b = sinkp2 + topSideOffset;
                auto linex3 = FollowerService::createLinePath(sinkp3b, sinkp3a, topAndBottomDepth,
                                                              kd.kitchenWorktopHeight);
                sg.GB<GT::Extrude>(PolyOutLine{ linex3, V3f::UP_AXIS, kd.worktopThickness }, GT::M(kd.worktopMaterial));

                // Bottom side of the worktop
                auto sinkp4a = sinkp1 - topSideOffset;
                auto sinkp4b = sinkp2 - topSideOffset;
                auto linex4 = FollowerService::createLinePath(sinkp4b, sinkp4a, topAndBottomDepth,
                                                              kd.kitchenWorktopHeight);
                sg.GB<GT::Extrude>(PolyOutLine{ linex4, V3f::UP_AXIS, kd.worktopThickness }, GT::M(kd.worktopMaterial));
            }
        }

        for ( const auto& kwp : kd.kitchenSkirtingPath ) {
            auto linex = FollowerService::createLinePath(kwp.p1, kwp.p2, 0.02f, 0.0f);
            sg.GB<GT::Extrude>(PolyOutLine{ linex, V3f::UP_AXIS, kd.skirtingHeight }, GT::M(kd.unitsMaterial));
        }

        float topOfWorktop = kd.kitchenWorktopHeight + kd.worktopThickness;
        for ( const auto& kup : kd.kitchenWorktopPath ) {
            auto rotation = Quaternion{ RoomService::furnitureAngleFromNormal(kup.normal), V3f::UP_AXIS };
            if ( kup.flags.hasCooker ) {
                auto mp = kup.cookerPos;
                auto extractorHeight = sg.GM(kd.extractorHoodModel)->BBox3d()->calcHeight();
                sg.GB<GT::Asset>(kd.cooktopModel,
                                 XZY::C(mp + kup.normal * kd.kitchenWorktopDepth * 0.5f, topOfWorktop),
                                 GT::Rotate(rotation));
                sg.GB<GT::Asset>(kd.extractorHoodModel,
                                 XZY::C(mp + kup.normal * kd.kitchenWorktopDepth * 0.5f, w->height-extractorHeight),
                                 GT::Rotate(rotation));
                sg.GB<GT::Asset>(kd.ovenPanelModel,
                                 XZY::C(mp + kup.normal * ( kd.kitchenWorktopDepth - kd.kitchenUnitsRecess ),
                                        topOfWorktop - kd.worktopThickness - kd.drawersPadding.y()), GT::Rotate(rotation));
            }
            if ( kup.flags.hasSink ) {
                sg.GB<GT::Asset>(kd.sinkModel,
                                 XZY::C(kup.sinkPos + kup.normal * kd.kitchenWorktopDepth * 0.5f, topOfWorktop),
                                 GT::Rotate(rotation));
            }
        }
    }

}

