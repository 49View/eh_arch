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
//            rr.draw<DFlatPoly>( room->mPerimeterSegments, 0.025f, C4f::RED, true );
//        }
        auto roomName = RoomService::roomNames(room);
        auto sm = arc.floorPlanShader();
        auto color = arc.getFillColor(room, C4f::PASTEL_GRAY);
        auto lineWidth = arc.floorPlanScaler(0.015f);

        bool drawDebug = arc.isFloorPlanRenderModeDebug();
        if ( drawDebug && arc.isSelected(room) ) {
            rr.draw<DLine>(room->mPerimeterSegments, lineWidth*2.0f, color, true, room->hashFeature("perimeter"+sm.hash(), 0));
//            rr.draw<DFlatPoly>(room->mPerimeterSegments, C4f::WHITE*0.9f, arc.pm(),
//                           room->hashFeature("perimeter", 0));
        }

        int ffc = 0;
        for ( const auto& ff : room->mFittedFurniture ) {
            auto ffColor = arc.getFillColor(ff.get(), C4f::BLACK);
            Matrix4f mt{ ff->Position() * V3f::MASK_Y_OUT, ff->Rotation(), ff->Size() * ff->Scale() };
            mt.mult(arc.pm()());
            rr.draw<DLine>(sg.PL(ff->symbolRef), ffColor, RDSPreMult(mt), sm, lineWidth,
                           room->hashFeature("ff"+sm.hash(), ffc++));
        }


        JMATH::Rect2f bestBBox(room->mMaxEnclosingBoundingBox);
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
                       room->hashFeature("T1", 0));

        rr.draw<DText>(FDS{ measureText, font, measurePos, fontHeight }, color, arc.pm(),
                       room->hashFeature("T2", 1));

        rr.draw<DText>(FDS{ areaSQm, font, areaPos, fontHeight }, color, arc.pm(),
                       room->hashFeature("T3", 2));

//        for ( auto& cov : room->mvSkirtingSegments ) {
//            rr.draw<DLine>(cov, 0.01f, C4f::BLUE, arc.pm(), room->hashFeature("skirting", 0));
//        }
    }

    void createCovingSegments( SceneGraph& sg, GeomSP eRootH, RoomBSData *w ) {

        // Only add geometry if room type needs is
        if ( RoomService::roomNeedsCoving(w) && w->mHasCoving ) {
//        mCovingH = std::make_shared<GeomAsset>();
            for ( const auto& cov : w->mvCovingSegments ) {
                ASSERT(cov.size() >= 2);
                bool bWrap = true;//isVerySimilar( cov.front(), cov.back() );
                FollowerFlags ff = bWrap ? FollowerFlags::WrapPath : FollowerFlags::Defaults;

                if ( auto profile = sg.PL(w->covingProfile); profile ) {
                    sg.GB<GT::Follower>(profile, w->Position(), eRootH, XZY::C(cov, w->Height()), ff, PolyRaise::VerticalNeg,
                                        GT::ForceNormalAxis(Vector3f::UP_AXIS),
                                        GT::Flip(V2fc::X_AXIS), w->covingMaterial);
                }
            }
        }
    }

    void createSkirtingSegments( SceneGraph& sg, GeomSP eRootH, RoomBSData *w ) {
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
                    sg.GB<GT::Follower>(profile, w->Position(), eRootH, XZY::C(cov, 0.0f), GT::ForceNormalAxis(Vector3f::UP_AXIS),
                                        ff, GT::Flip(V2fc::X_AXIS), w->skirtingMaterial);
                }
            }
        }
    }

    GeomSP make3dGeometry( SceneGraph& sg, GeomSP eRootH, RoomBSData *w ) {

        auto lRootH = eRootH->addChildren("Room"+ std::to_string(w->hash));

        RoomRender::createCovingSegments(sg, lRootH, w);
        RoomRender::createSkirtingSegments(sg, lRootH, w);
        WallRender::make3dGeometry(sg, lRootH, w->mWallSegmentsSorted, w->wallsMaterial);

        float zPull = 0.001f;
        auto outline = PolyOutLine{ XZY::C(w->mPerimeterSegments), V3f::UP_AXIS, zPull };
        sg.GB<GT::Extrude>(outline, lRootH, V3f{ V3f::UP_AXIS * -zPull } + w->Position(), w->floorMaterial,
                                       GT::Tag(ArchType::FloorT));

        for ( const auto& lf : w->mLightFittings ) {
            auto spotlightGeom = sg.GB<GT::Asset>(w->spotlightGeom, lRootH, w->Position() + XZY::C(lf.lightPosition) + V3f::UP_AXIS * 0.023f);
            auto lKey = lf.key;
            sg.add<Light>(lKey,
                          Light{ LightType_Point, lf.key, w->spotlightGeom, XZY::C(lf.lightPosition) + V3f::UP_AXIS_NEG * w->spotLightYOffset*2.0f + w->Position(),
                                 3.5f, 0.0f, V3f::Y_AXIS * .5f });
        }
        for ( const auto& lf : w->mSwitchesLocators ) {
            sg.GB<GT::Asset>("lightswitch", lRootH, V3f{ lf.x(), 1.2f, lf.y() } + w->Position(),
                             GT::Rotate(Quaternion{ lf.z(), V3f::UP_AXIS }));
        }
        for ( const auto& lf : w->mSocketLocators ) {
            sg.GB<GT::Asset>("powerSocket", lRootH, V3f{ lf.x(), .252f, lf.y() } + w->Position(),
                             GT::Rotate(Quaternion{ lf.z(), V3f::UP_AXIS }));
        }
        for ( auto& fur : w->mFittedFurniture ) {
            if (!fur->name.empty()) {
                auto furn = sg.GB<GT::Asset>(fur->name, lRootH, fur->Position(), GT::Rotate(fur->Rotation()), GT::Scale(fur->Scale()));
                if ( furn ) {
                    fur->linkedUUID = furn->UUiDCopy();
//                    sg.GB<GT::Shape>(ShapeType::Cube, fur->Center(), GT::Rotate(fur->Rotation()), GT::Scale{fur->Size()}, C4f::BLUE_SHADOW );
                } else {
                    LOGRS("For some reason I cannot load a fitted furniture, it's empty, on room " << RS::roomName(w))
                }
            }
        }
        if ( RoomService::hasRoomType(w, ASType::Kitchen) ) {
            KitchenRender::render(sg, lRootH, w);
        }

        sg.GB<GT::Extrude>(outline, lRootH, V3f{ V3f::UP_AXIS * (w->Height() - zPull) } + w->Position(),
                                         w->ceilingMaterial,
                                         GT::Tag(ArchType::CeilingT));
        return lRootH;
    }
}
