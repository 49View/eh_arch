//
//  room_render.cpp
//  sixthmaker
//
//  Created by Dado on 19/03/2017.
//
//

#include "room_render.hpp"
#include <core/math/path_util.h>
#include <core/resources/profile.hpp>
#include <core/v_data.hpp>
#include <core/resources/resource_builder.hpp>
#include <core/font_utils.hpp>
#include <poly/scene_graph.h>
#include <poly/vdata_assembler.h>
#include <graphics/renderer.h>
#include "../models/house_bsdata.hpp"
#include "../models/room_service.hpp"
#include <eh_arch/controller/arch_render_controller.hpp>

#include "house_render.hpp"
#include "wall_render.hpp"
#include "kitchen_render.hpp"

namespace RoomRender {

    V2f fitTextInBox( const Font& font, const std::string& text, const Rect2f& bbox, float& fontHeight ) {
        auto textRect = FontUtils::measure(text, &font, fontHeight);
        if ( bbox.width() < textRect.width() ) {
            float ratio = ( textRect.width() / bbox.width() ) * 1.2f; // add text ratio with some slack
            fontHeight /= ratio;
            textRect = FontUtils::measure(text, &font, fontHeight);
        }
        float textStartOffset = ( bbox.width() - textRect.width() ) * 0.5f;
        return bbox.centreLeft() + V2fc::X_AXIS * textStartOffset;
    }

    void IMHouseRender( Renderer& rr, SceneGraph& sg, const RoomBSData *room, const ArchRenderController& ims ) {
//        bool drawDebug = fpRenderMode == Use2dDebugRendering::True;
//        auto color = drawDebug ? Color4f::RANDA1().A(0.5f) : C4f::WHITE.A(0.5f);

//        rr.draw<DPoly>( room->mPerimeterSegments, 0.025f, color, true );
//        if ( drawDebug) {
//            rr.draw<DLine>( room->mPerimeterSegments, 0.03f, C4f::RED, false );
//        }
        auto rm = ims.floorPlanShader();
        auto color = ims.getFillColor(room->hash, RoomService::roomColor(room).A(0.5f));
        auto lineWidth = ims.floorPlanScaler(0.0075f);

        bool drawDebug = ims.isFloorPlanRenderModeDebug();
        if ( drawDebug ) {
            rr.draw<DPoly>(room->mPerimeterSegments, color, ims.pm(), room->hashFeature("perimeter", 0));
        }

        if ( !RoomService::isGeneric(room) ) {
            auto roomName = RoomService::roomNames(room);
            JMATH::Rect2f bestBBox(room->mMaxEnclsingBoundingBox);
            auto font = sg.FM().get(S::DEFAULT_FONT).get();
            float fontHeight = 0.32f;

            // If the text width is bigger than the best bbox then we need to scale it down, we might need to
            // normalise it across the whole floorplan as this may produce different text sizes per room which looks
            // awful, will take care of this later, probably having a HouseService function that scans all the rooms
            // and pre-compute the maximum common denominator of all font heights for all rooms.

            auto textPos = FontUtils::fitTextInBox(font, roomName, bestBBox, fontHeight);
            textPos -= V2fc::Y_AXIS * fontHeight * 0.5f;

            auto measureText = RoomService::roomSizeToString(room);
            auto measurePos = FontUtils::fitTextInBox(font, measureText, bestBBox, fontHeight);
            measurePos += V2fc::Y_AXIS * fontHeight * 0.5f;

            rr.draw<DText>(FDS{ roomName, font, textPos, fontHeight }, C4f::BLACK, ims.pm(),
                           room->hashFeature(roomName, 0));

            rr.draw<DText>(FDS{ measureText, font, measurePos, fontHeight }, C4f::BLACK, ims.pm(),
                           room->hashFeature(roomName, 1));
        }

        int ffc = 0;
        for ( const auto& ff : room->mFittedFurniture ) {
            Matrix4f mt{ ff.position3d * V3f::MASK_Y_OUT, ff.rotation, ff.size };
            mt.mult(ims.pm()());
            rr.draw<DLine>(sg.PL(ff.symbolRef), C4f::BLACK, RDSPreMult(mt), rm, lineWidth,
                           room->hashFeature(ff.symbolRef, ffc++));
        }
    }

    GeomSPContainer createCovingSegments( SceneGraph& sg, RoomBSData *w ) {
        GeomSPContainer ret;

        // Only add geometry if room type needs is
        if ( RoomService::roomNeedsCoving(w) && w->mHasCoving ) {
//        mCovingH = std::make_shared<GeomAsset>();
            for ( const auto& cov : w->mvCovingSegments ) {
                ASSERT(cov.size() >= 2);
                bool bWrap = true;//isVerySimilar( cov.front(), cov.back() );
                FollowerFlags ff = bWrap ? FollowerFlags::WrapPath : FollowerFlags::Defaults;

                if ( auto profile = sg.PL(w->covingProfile); profile ) {
                    ret.emplace_back(
                            sg.GB<GT::Follower>(profile, XZY::C(cov, w->height), ff, PolyRaise::VerticalNeg,
                                                GT::ForceNormalAxis(Vector3f::UP_AXIS),
                                                GT::Flip(V2fc::X_AXIS), GT::M(w->covingMaterial)));
                }
            }
        }
        return ret;
    }

    GeomSPContainer createSkirtingSegments( SceneGraph& sg, RoomBSData *w ) {
        GeomSPContainer ret;
        if ( RoomService::roomNeedsCoving(w) ) {
            for ( auto& cov : w->mvSkirtingSegments ) {
                ASSERT(cov.size() >= 2);
                // !!! Check if wraps before we optimize the collinear
                //            bool bWrap = false;//isVerySimilar( cov.front(), cov.back() );
                //            FollowerFlags ff = bWrap ? FollowerFlags::WrapPath | FollowerFlags::NoCaps : FollowerFlags::Defaults;
                // we add a little bit of gap to make sure the skirting doesn't touch exactly the ground to avoid SH self intersecting problems
                //            float skirtingGapFromFloor = 0.00001f;

                bool bWrap = false;//isVerySimilar( cov.front(), cov.back() );
                FollowerFlags ff = bWrap ? FollowerFlags::WrapPath : FollowerFlags::Defaults;

                if ( auto profile = sg.PL(w->skirtingProfile); profile ) {
                    ret.emplace_back(
                            sg.GB<GT::Follower>(profile, XZY::C(cov, 0.0f), GT::ForceNormalAxis(Vector3f::UP_AXIS),
                                                ff, GT::Flip(V2fc::X_AXIS), GT::M(w->skirtingMaterial)));
                }
            }
        }
        return ret;
    }

    void make3dGeometry( SceneGraph& sg, RoomBSData *w, HouseRenderContainer& ret ) {
        auto wc = RoomRender::createCovingSegments(sg, w);
        auto ws = RoomRender::createSkirtingSegments(sg, w);
        WallRender::renderWalls(sg, w->mWallSegmentsSorted, w->wallMaterial, w->wallColor);
        ret.covingGB.insert(ret.covingGB.end(), wc.begin(), wc.end());
        ret.skirtingGB.insert(ret.skirtingGB.end(), ws.begin(), ws.end());

        auto outline = PolyOutLine{ XZY::C(w->mPerimeterSegments), V3f::UP_AXIS, 0.1f };
        ret.floor = sg.GB<GT::Extrude>(outline,
                                       V3f{ V3f::UP_AXIS * -0.1f },
                                       GT::M(w->floorMaterial),
                                       GT::Tag(ArchType::FloorT));

        for ( const auto& lf : w->mLightFittingsLocators ) {
            auto spotlightGeom = sg.GB<GT::Asset>(w->spotlightGeom, XZY::C(lf));
            auto lKey = ResourceGroup::Light + lf.toString();
            sg.add<Light>(lKey,
                          Light{ LightType_Point, w->spotlightGeom, XZY::C(lf) + V3f::UP_AXIS_NEG * 0.8f,
                                 8.0f, 1.0f, V3f::Y_AXIS * .5f });
        }
        for ( const auto& lf : w->mSwichesLocators ) {
            sg.GB<GT::Asset>("lightswitch", V3f{ lf.x(), 1.2f, lf.y() },
                             GT::Rotate(Quaternion{ lf.z(), V3f::UP_AXIS }));
        }
        for ( const auto& lf : w->mSocketLocators ) {
            sg.GB<GT::Asset>("powersocket", V3f{ lf.x(), .252f, lf.y() },
                             GT::Rotate(Quaternion{ lf.z(), V3f::UP_AXIS }));
        }
        for ( const auto& fur : w->mFittedFurniture ) {
            auto furn = sg.GB<GT::Asset>(fur.name, fur.position3d, GT::Rotate(fur.rotation));
            ret.furnituresGB.emplace_back(furn);
        }
        if ( RoomService::hasRoomType(w, ASType::Kitchen) ) {
            KitchenRender::render(sg, w, ret);
        }
    }
}
