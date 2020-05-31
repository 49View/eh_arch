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

    void IMHouseRender( Renderer &rr, SceneGraph &sg, const RoomBSData *data, const ArchRenderController& ims ) {
//        bool drawDebug = fpRenderMode == Use2dDebugRendering::True;
//        auto color = drawDebug ? Color4f::RANDA1().A(0.5f) : C4f::WHITE.A(0.5f);

//        rr.draw<DPoly>( data->mPerimeterSegments, 0.025f, color, true );
//        if ( drawDebug) {
//            rr.draw<DLine>( data->mPerimeterSegments, 0.03f, C4f::RED, false );
//        }
        auto rm = ims.floorPlanShader();
        auto lineWidth = ims.floorPlanScaler(0.0075f);

        if ( !RoomService::isGeneric( data )) {
            rr.draw<DText2d>(
                    FDS{ RoomService::roomNames( data ), sg.FM().get( S::DEFAULT_FONT ).get(), data->bbox.centreLeft(),
                         .4f }, C4f::BLACK, ims.pm() );
        }

        for ( const auto &ff : data->mFittedFurniture ) {
            Matrix4f mt{ ff.position3d * V3f::MASK_Y_OUT, ff.rotation, ff.size };
            mt.mult( ims.pm()() );
            rr.draw<DLine>( sg.PL( ff.symbolRef ), C4f::BLACK, RDSPreMult( mt ), rm, lineWidth );

        }
    }

    GeomSPContainer createCovingSegments( SceneGraph &sg, RoomBSData *w ) {
        GeomSPContainer ret;

        // Only add geometry if room type needs is
        if ( RoomService::roomNeedsCoving( w ) && w->mHasCoving ) {
//        mCovingH = std::make_shared<GeomAsset>();
            for ( const auto &cov : w->mvCovingSegments ) {
                ASSERT( cov.size() >= 2 );
                bool bWrap = true;//isVerySimilar( cov.front(), cov.back() );
                FollowerFlags ff = bWrap ? FollowerFlags::WrapPath : FollowerFlags::Defaults;

                if ( auto profile = sg.PL( w->covingProfile ); profile ) {
                    ret.emplace_back(
                            sg.GB<GT::Follower>( profile, XZY::C( cov, w->height ), ff, PolyRaise::VerticalNeg,
                                                 GT::ForceNormalAxis( Vector3f::UP_AXIS ),
                                                 GT::Flip( V2fc::X_AXIS ), GT::M( w->covingMaterial )));
                }
            }
        }
        return ret;
    }

    GeomSPContainer createSkirtingSegments( SceneGraph &sg, RoomBSData *w ) {
        GeomSPContainer ret;
        if ( RoomService::roomNeedsCoving( w )) {
            for ( auto &cov : w->mvSkirtingSegments ) {
                ASSERT( cov.size() >= 2 );
                // !!! Check if wraps before we optimize the collinear
                //            bool bWrap = false;//isVerySimilar( cov.front(), cov.back() );
                //            FollowerFlags ff = bWrap ? FollowerFlags::WrapPath | FollowerFlags::NoCaps : FollowerFlags::Defaults;
                // we add a little bit of gap to make sure the skirting doesn't touch exactly the ground to avoid SH self intersecting problems
                //            float skirtingGapFromFloor = 0.00001f;

                bool bWrap = false;//isVerySimilar( cov.front(), cov.back() );
                FollowerFlags ff = bWrap ? FollowerFlags::WrapPath : FollowerFlags::Defaults;

                if ( auto profile = sg.PL( w->skirtingProfile ); profile ) {
                    ret.emplace_back(
                            sg.GB<GT::Follower>( profile, XZY::C( cov, 0.0f ), GT::ForceNormalAxis( Vector3f::UP_AXIS ),
                                                 ff, GT::Flip( V2fc::X_AXIS ), GT::M( w->skirtingMaterial )));
                }
            }
        }
        return ret;
    }

    void make3dGeometry( SceneGraph &sg, RoomBSData *w, HouseRenderContainer &ret ) {
        auto wc = RoomRender::createCovingSegments( sg, w );
        auto ws = RoomRender::createSkirtingSegments( sg, w );
        WallRender::renderWalls( sg, w->mWallSegmentsSorted, w->wallMaterial, w->wallColor );
        ret.covingGB.insert( ret.covingGB.end(), wc.begin(), wc.end());
        ret.skirtingGB.insert( ret.skirtingGB.end(), ws.begin(), ws.end());

        auto outline = PolyOutLine{ XZY::C( w->mPerimeterSegments ), V3f::UP_AXIS, 0.1f };
        ret.floor = sg.GB<GT::Extrude>( outline,
                                        V3f{ V3f::UP_AXIS * -0.1f },
                                        GT::M( w->floorMaterial ),
                                        GT::Tag( ArchType::FloorT ));

        for ( const auto &lf : w->mLightFittingsLocators ) {
            auto spotlightGeom = sg.GB<GT::Asset>( w->spotlightGeom, XZY::C( lf ));
            auto lKey = ResourceGroup::Light + lf.toString();
            sg.add<Light>( lKey,
                           Light{ LightType_Point, w->spotlightGeom, XZY::C( lf ) + V3f::UP_AXIS_NEG * 0.8f,
                                  8.0f, 1.0f, V3f::Y_AXIS * .5f } );
        }
        for ( const auto &lf : w->mSwichesLocators ) {
            sg.GB<GT::Asset>( "lightswitch", V3f{ lf.x(), 1.2f, lf.y() },
                              GT::Rotate( Quaternion{ lf.z(), V3f::UP_AXIS } ));
        }
        for ( const auto &lf : w->mSocketLocators ) {
            sg.GB<GT::Asset>( "powersocket", V3f{ lf.x(), .252f, lf.y() },
                              GT::Rotate( Quaternion{ lf.z(), V3f::UP_AXIS } ));
        }
        for ( const auto &fur : w->mFittedFurniture ) {
            auto furn = sg.GB<GT::Asset>( fur.name, fur.position3d, GT::Rotate( fur.rotation ));
            ret.furnituresGB.emplace_back( furn );
        }
        if ( RoomService::hasRoomType(w, ASType::Kitchen) ) {
            KitchenRender::render(sg, w, ret);
        }
    }
}
