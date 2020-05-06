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

namespace RoomRender {

    void make2dGeometry( Renderer& rr, SceneGraph& sg, const RoomBSData *data, Use2dDebugRendering bDrawDebug,
                         const RDSPreMult &_pm ) {
//        bool drawDebug = bDrawDebug == Use2dDebugRendering::True;
//        auto color = drawDebug ? Color4f::RANDA1().A(0.5f) : C4f::WHITE.A(0.5f);

//        rr.draw<DPoly>( data->mPerimeterSegments, 0.025f, color, true );
//        if ( drawDebug) {
//            rr.draw<DLine>( data->mPerimeterSegments, 0.03f, C4f::RED, false );
//        }
        if ( !RoomService::isGeneric(data) ) {
            rr.draw<DText2d>( FDS{ RoomService::roomName(data), sg.FM().get(S::DEFAULT_FONT).get(), XZY::C(data->bbox.centreLeft()), .4f*0.1f }, _pm );
        }
    }

    GeomSPContainer createCovingSegments( SceneGraph& sg, RoomBSData* mData ) {
        GeomSPContainer ret;

        // Only add geometry if room type needs is
        if ( RoomService::roomNeedsCoving( mData ) && mData->mHasCoving ) {
//        mCovingH = std::make_shared<GeomAsset>();
            for (const auto& cov : mData->mvCovingSegments) {
                ASSERT( cov.size() >= 2 );
                bool bWrap = true;//isVerySimilar( cov.front(), cov.back() );
                FollowerFlags ff = bWrap ? FollowerFlags::WrapPath : FollowerFlags::Defaults;

                if ( auto profile = sg.PL(mData->covingProfile); profile ) {
                    ret.emplace_back(
                            sg.GB<GT::Follower>( profile, XZY::C( cov, mData->height ), ff, PolyRaise::VerticalNeg,
                                                 GT::ForceNormalAxis( Vector3f::UP_AXIS ),
                                                 GT::Flip( Vector2f::X_AXIS )));
                }
            }
        }
        return ret;
    }

    GeomSPContainer createSkirtingSegments( SceneGraph& sg, RoomBSData* mData ) {
        GeomSPContainer ret;
        if ( RoomService::roomNeedsCoving( mData )) {
            for ( auto& cov : mData->mvSkirtingSegments ) {
                ASSERT( cov.size() >= 2 );
                // !!! Check if wraps before we optimize the collinear
    //            bool bWrap = false;//isVerySimilar( cov.front(), cov.back() );
    //            FollowerFlags ff = bWrap ? FollowerFlags::WrapPath | FollowerFlags::NoCaps : FollowerFlags::Defaults;
                // we add a little bit of gap to make sure the skirting doesn't touch exactly the ground to avoid SH self intersecting problems
    //            float skirtingGapFromFloor = 0.00001f;

                bool bWrap = false;//isVerySimilar( cov.front(), cov.back() );
                FollowerFlags ff = bWrap ? FollowerFlags::WrapPath : FollowerFlags::Defaults;

                if ( auto profile = sg.PL(mData->skirtingProfile); profile ) {
                    ret.emplace_back( sg.GB<GT::Follower>( profile, XZY::C(cov, 0.0f), GT::ForceNormalAxis(Vector3f::UP_AXIS), ff, GT::Flip(Vector2f::X_AXIS) ) );
                }
            }
        }
        return ret;
    }

    void createCeiling( SceneGraph& sg, GeomSP mRootH, RoomBSData* mData, const CeilingType /*_ct*/, const CeilingMatType _cmt ) {

//        if ( RoomService::roomNeedsCoving( mData ) && mData->mHasCoving ) {
//            for (const auto& cov : mData->mvCovingSegments) {
//                V3fVector v3;
//                for ( const auto c : cov ) { v3.emplace_back( XZY::C(c) ); }
//                sg.GB<GT::Extrude>( PolyOutLine{ v3, 0.1f }, V3f::UP_AXIS*mData->height );
//            }
//        }

//      ### This is supposed to be the right version :O
//        ClipperLib::Paths lFloorPath = Dad()->getPathsForFloorType( FloorMatType::Quad );
//        ClipperLib::Path currPath = PolyServices::v2ListToClipperPath( mData->mPerimeterSegments );
//
//        GB{ mPrefs.sg, PolyServices::clipperToPolylines( lFloorPath, currPath, -Vector3f::UP_AXIS, ReverseFlag::True ), mData->height }
//        .inj( mRootH->addChildren()).g(GHType::Ceiling).build();

        // Place lights
//        for ( const auto& lfp : mData->mLightFittingsLocators ) {
//            GB{ mPrefs.sg, GeomBuilderType::file, "spotlight"}.g(GHType::LightFitting).at(XZY::C(lfp)).
//                        inj(mRootH->addChildren()).build();
//    }
}

    void make3dGeometry( SceneGraph& sg, RoomBSData* mData ) {
//    createFloor();
//        auto mRootH = sg.GC();
        createCovingSegments( sg, mData );
        createSkirtingSegments( sg, mData );
//        createCeiling(  sg, mRootH, mData, CeilingType::Flat, CeilingMatType::Plaster );
//    createSockets();
//    createFittedFurnitures();
//        mRootH->updateTransform();
//        sg.GC(mRootH);
}

}

//
//void RoomRender::mapWalls( WallMatType wmt ) {
//    switch ( wmt ) {
//        case WallMatType::Plaster:
//            break;
//        case WallMatType::Tiles: {
//            Vector2f tileDim{ 0.6f, 0.3f };
//            for ( auto& wsl : mData->mWallSegments ) {
//                for ( auto& ws : wsl ) {
//                    auto wd = Dad()->wallRendererAt( ws.iWall );
//                    if ( ws.iIndex != -1 ) {
//                        wd->splitWallForTiles( ws.iIndex, tileDim );
//                    }
//                }
//            }
//        }
//            break;
//        case WallMatType::TilesOffset:
//            break;
//        default:
//            break;
//    }
//}
//
//FloorRender *RoomRender::Dad() const {
//    return dynamic_cast<FloorRender *>( mPrefs.mDad );
//}
//
////void RoomRender::createFloorMat( const std::vector<Vector2f>& fpoints, const FloorMatTypeT _fmt ) {
////    GeomBuilder wb = GB{ mPrefs.sg,  GeomBuilderType::poly }.m( "parquet_european_ash" );
////
////    switch ( _fmt ) {
////        case FloorMatType::Wood: {
//////            floorSurfaceTop->setMaterial( "parquet_european_ash" );
//////            floorSurfaceTop->Bevel( { 0.003f, 0.003f, 0.005f } );
//////            floorSurfaceTop->pull( fpoints, 0.02f, -0.02f );
////        }
////            break;
////        case FloorMatType::Tiles: {
//////            floorSurfaceTop->setMaterial("marble_tiles_lobby");
//////            floorSurfaceTop->pull( fpoints, 0.02f, -.02f );
////        }
////            break;
////        case FloorMatType::Carpet: {
//////            floorSurfaceTop->setMaterial( "carpet_loop" );
//////            floorSurfaceTop->pull( fpoints, 0.02f, -.02f );
////        }
////        break;
////        default:
////            break;
////    }
////
////    wb.addPoly( { fpoints, Vector3f::Z_AXIS }, 0.0f );
////    wb.build();
////
//////    newFloorH->GHType( GHType::Ground );
////}
//
//void RoomRender::createCeilingMesh( const std::vector<Vector2f>& fpoints, const CeilingMatType /*_cmt*/ ) {
////    float ceilingSlabHeight = 0.02f;
////    auto floorSurfaceTop = GB{ mPrefs.sg,  fpoints, ceilingSlabHeight }.m("plaster_ultra_fine_spray").
////            at(Vector3f::Z_AXIS*mData->height - 0.0f).build();
////
////    GeomSP newCeilingH = mCeilingH->addChildren( floorSurfaceTop );
////    newCeilingH->GHType( GHType::Ceiling );
////    newCeilingH->CanBeReflected( true );
//}
//
//std::string RoomRender::floorMatName() const {
//    std::string lfloorMatName = "white";
//    switch ( mData->floorType ) {
//        case FloorMatType::Wood: {
//            lfloorMatName = "white";
//        }
//            break;
//        case FloorMatType::Tiles: {
//            lfloorMatName = "tiles";
//        }
//            break;
//        case FloorMatType::Carpet: {
//            lfloorMatName = "carpet";
//        }
//        default:
//            break;
//    }
//    return lfloorMatName;
//}
//
//void RoomRender::createFloor() {
//
//    ClipperLib::Paths lFloorPath = Dad()->getPathsForFloorType( mData->floorType );
//    ClipperLib::Path currPath = PolyServices::v2ListToClipperPath( mData->mPerimeterSegments );
//
//    GB{ mPrefs.sg,PolyServices::clipperToPolylines( lFloorPath, currPath, Vector3f::UP_AXIS ) }.m( S::SH, floorMatName() ).
//    inj(mRootH->addChildren()).g(GHType::Floor).build();
//}
//
//
//void RoomRender::createSockets() {
//
//    if ( RoomService::roomNeedsCoving( mData )) {
//        for ( auto& sl : mData->mSocketLocators ) {
//            GB{ mPrefs.sg,  GeomBuilderType::file, { "powersocket" } }
//                       .at( XZY::C({ sl.xy(), 0.35f })).r(XZY::C({0.0f, 0.0f, sl.z() }))
//                       .inj(mRootH->addChildren()).build();
//        }
//    }
//    for ( auto& sl : mData->mSwichesLocators ) {
//        GB{ mPrefs.sg,  GeomBuilderType::file, { "lightswitch" } }
//                .at(XZY::C({ sl.xy(), 1.15f })).r( XZY::C({0.0f, 0.0f, sl.z() }))
//                .inj(mRootH->addChildren()).build();
//
//    }
//}
//
//void RoomRender::startMake3d( const std::string& _roomName ) {
//    mRootH->Name( _roomName );
//    mRootH->GHType( GHType::Room );
//}
//
//void RoomRender::makeLights() {
////    for ( const auto& lfp : mData->mLightFittingsLocators ) {
////        LM.addPointLight( lfp + Vector3f::Z_AXIS_NEG * 0.15f, 1.0f, Vector3f( 5.0f ));
////    }
//}
//
//void RoomRender::createFittedFurnitures() {
//    if ( mData->mFittedFurniture.empty()) return;
//
//    for ( auto& f : mData->mFittedFurniture ) {
//        GB{ mPrefs.sg, GeomBuilderType::file, f.name }.at(XZY::C(f.position)).r(XZY::C(f.rotation))
//        .inj(mRootH->addChildren()).build();
//    }
//}
//
//void GenericRoomRender::make2dGeometry() {
//    RoomRender::make2dGeometry();
//}
//
//void GenericRoomRender::makeGeometryImpl() {
//    startMake3d( "Room" );
//}
//
//void GenericRoomRender::make3dGeometry() {
//    RoomRender::make3dGeometry();
//}
