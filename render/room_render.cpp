//
//  room_render.cpp
//  sixthmaker
//
//  Created by Dado on 19/03/2017.
//
//

#include "room_render.hpp"
#include <core/util.h>
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

    void IMHouseRender( Renderer& rr, SceneGraph& sg, const RoomBSData *room, const ArchRenderController& arc ) {

//        if ( drawDebug) {
//            rr.draw<DPoly>( room->mPerimeterSegments, 0.025f, C4f::RED, true );
//        }
        auto rm = arc.floorPlanShader();
        auto color = arc.getFillColor(room->hash, C4f::BLACK);
        auto lineWidth = arc.floorPlanScaler(0.015f);

        bool drawDebug = arc.isFloorPlanRenderModeDebug();
        if ( drawDebug && !arc.isFloorPlanRenderModeDebugSelection() ) {
            rr.draw<DLine>( room->mPerimeterSegments, lineWidth, C4f::YELLOW, true );
//            rr.draw<DPoly>(room->mPerimeterSegments, C4f::WHITE*0.9f, arc.pm(),
//                           room->hashFeature("perimeter", 0));
        }

        int ffc = 0;
        for ( const auto& ff : room->mFittedFurniture ) {
            Matrix4f mt{ ff.position3d * V3f::MASK_Y_OUT, ff.rotation, ff.size };
            mt.mult(arc.pm()());
            rr.draw<DLine>(sg.PL(ff.symbolRef), C4f::PASTEL_GRAY, RDSPreMult(mt), rm, lineWidth*2.0f,
                           room->hashFeature(ff.symbolRef, ffc++));
        }


        auto roomName = RoomService::roomNames(room);
        JMATH::Rect2f bestBBox(room->mMaxEnclsingBoundingBox);
        auto font = sg.FM().get(S::DEFAULT_FONT).get();
        float fontHeight = 0.32f;

        // If the text width is bigger than the best bbox then we need to scale it down, we might need to
        // normalise it across the whole floorplan as this may produce different text sizes per room which looks
        // awful, will take care of this later, probably having a HouseService function that scans all the rooms
        // and pre-compute the maximum common denominator of all font heights for all rooms.

        auto textPos = FontUtils::fitTextInBox(font, roomName, bestBBox, fontHeight);
        textPos -= V2fc::Y_AXIS * fontHeight;

        auto measureText = RoomService::roomSizeToString(room);
        auto measurePos = FontUtils::fitTextInBox(font, measureText, bestBBox, fontHeight);

        auto areaSQm = sqmToString(room->area);
        auto areaPos = FontUtils::fitTextInBox(font, areaSQm, bestBBox, fontHeight);
        areaPos += V2fc::Y_AXIS * fontHeight;

        rr.draw<DText>(FDS{ roomName, font, textPos, fontHeight }, color, arc.pm(),
                       room->hashFeature(roomName + color.toString(), 0));

        rr.draw<DText>(FDS{ measureText, font, measurePos, fontHeight }, color, arc.pm(),
                       room->hashFeature(roomName + color.toString(), 1));

        rr.draw<DText>(FDS{ areaSQm, font, areaPos, fontHeight }, color, arc.pm(),
                       room->hashFeature(roomName + color.toString(), 2));

//        for ( auto& cov : room->mvSkirtingSegments ) {
//            rr.draw<DLine>(cov, 0.01f, C4f::BLUE, arc.pm(), room->hashFeature("skirting", 0));
//        }
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
        WallRender::renderWalls(sg, w->mWallSegmentsSorted, w->wallsMaterial.materialHash, w->wallsMaterial.color);
        ret.covingGB.insert(ret.covingGB.end(), wc.begin(), wc.end());
        ret.skirtingGB.insert(ret.skirtingGB.end(), ws.begin(), ws.end());

        auto outline = PolyOutLine{ XZY::C(w->mPerimeterSegments), V3f::UP_AXIS, 0.1f };
        ret.floor = sg.GB<GT::Extrude>(outline,
                                       V3f{ V3f::UP_AXIS * -0.1f },
                                       GT::M(w->floorMaterial.materialHash),
                                       GT::Tag(ArchType::FloorT));

        for ( const auto& lf : w->mLightFittingsLocators ) {
            auto spotlightGeom = sg.GB<GT::Asset>(w->spotlightGeom, XZY::C(lf)+V3f::UP_AXIS*0.023f);
            auto lKey = ResourceGroup::Light + lf.toString();
            sg.add<Light>(lKey,
                          Light{ LightType_Point, w->spotlightGeom, XZY::C(lf) + V3f::UP_AXIS_NEG * 0.1f,
                                 3.5f, 1.0f, V3f::Y_AXIS * .5f });
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
