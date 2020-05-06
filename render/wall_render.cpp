//
//  wall_render.cpp
//  sixthmaker
//
//  Created by Dado on 19/03/2017.
//
//

#include "wall_render.hpp"
#include <poly/scene_graph.h>
#include <core/v_data.hpp>
#include <core/resources/resource_builder.hpp>
#include <graphics/renderer.h>
#include "../models/house_bsdata.hpp"
#include "../models/wall_service.hpp"

namespace WallRender {

    void drawWalls2d( Renderer &rr, const WallBSData *wall, float width, Use2dDebugRendering bDrawDebug,
                      const RDSPreMult &_pm ) {
        if ( !checkBitWiseFlag( wall->wallFlags, WallFlags::WF_IsDoorPart ) &&
             !checkBitWiseFlag( wall->wallFlags, WallFlags::WF_IsWindowPart )) {
            auto wc = bDrawDebug == Use2dDebugRendering::True ? Color4f::RANDA1() : C4f::BLACK;
            rr.draw<DLine2d>( wall->epoints, width, wc, true, _pm );
        }
    }

    void drawIncrementalAlphaWalls2d( Renderer &rr, const WallBSData *wall, float width,
                                      const RDSPreMult &_pm ) {
        auto wc = Color4f::RANDA1();
        auto wps = wall->epoints.size();
        for ( size_t t = 0; t < wps - 1 * !wall->wrapLastPoint; t++ ) {
            float incRatio = float( t ) / float( wps );
            auto wca = Color4f::WHITE * V4f{ V3f::ONE, 0.5f + ( incRatio * 0.5f ) };
            auto p1 = wall->epoints[t];
            auto p2 = wall->epoints[cai( t + 1, wps )];
            auto pm = lerp( 0.5f, p1, p2 );
            rr.draw<DLine2d>( p1, p2, wc * wca, width, _pm );
            rr.draw<DLine2d>( pm, pm + wall->enormals[t] * width * 3.0f, Color4f::PASTEL_CYAN, width * 0.05f,
                            true, _pm );
        }
    }

    void drawUShapes2d( Renderer &rr, const WallBSData *wall, float width,
                        const RDSPreMult &_pm ) {
        std::array<Color4f, 3> usc = { Color4f::PASTEL_YELLOW, Color4f::PASTEL_CYAN, Color4f::PASTEL_GREEN };
        for ( const auto &us : wall->mUShapes ) {
            for ( int t = 0; t < 3; t++ ) {
                rr.draw<DLine2d>( us.points[t], us.points[t + 1], usc[t], width * 1.2f, _pm );
            }
            rr.draw<DCircleFilled2d>( us.middle, Color4f::ORANGE_SCHEME1_1, width * 5.0f, _pm );
        }
    }

    void drawWallNormals2d( Renderer &rr, const WallBSData *wall, float width,
                            const RDSPreMult &_pm ) {
        auto wps = wall->epoints.size();
        for ( size_t t = 0; t < wps - 1 * !wall->wrapLastPoint; t++ ) {
            auto p1 = wall->epoints[t];
            auto p2 = wall->epoints[cai( t + 1, wps )];
            auto pm = lerp( 0.5f, p1, p2 );
            rr.draw<DLine2d>( pm, pm + wall->enormals[t] * width * 3.0f, Color4f::PASTEL_CYAN, width * 0.1f,
                            true, _pm );
        }
    }

    void make2dGeometry( Renderer &rr, SceneGraph &sg, const WallBSData *wall, Use2dDebugRendering bDrawDebug,
                         const RDSPreMult &_pm ) {
        float width = 0.015f;
        float lineWidth = 0.0015f;
        drawWalls2d( rr, wall, lineWidth, bDrawDebug, _pm );
        bool drawDebug = bDrawDebug == Use2dDebugRendering::True;
        if ( drawDebug ) {
            drawWallNormals2d( rr, wall, width, _pm );
            drawUShapes2d( rr, wall, width, _pm );
        }
    }

    GeomSPContainer make3dGeometry( SceneGraph &sg, const WallBSData *mWall,
                                    const V3fVectorOfVector &ceilingContours ) {

        GeomSPContainer ret;

        size_t csize = mWall->epoints.size();
//        bool isPartOfDoorOrWindow = false;
        size_t wrapExtraVert = mWall->wrapLastPoint != 0 ? 0 : 1;

        std::vector<QuadVector3fNormal> wallQuads;
        for ( auto t = 0u; t < csize - wrapExtraVert; t++ ) {
            if ( WallService::checkUShapeIndexStartIsDoorOrWindow( mWall, t )) {
//                isPartOfDoorOrWindow = true;
                continue;
            }

            for ( const auto &quad : WallService::vertsForWallAt( mWall, t, ceilingContours )) {
                wallQuads.emplace_back( QuadVector3fNormal{ quad, XZY::C( mWall->enormals[t], 0.0f ) } );
            }
        }

        auto mainWall = sg.GB<GT::Mesh>( wallQuads, GT::M( "plaster_spanish" ), C4f::XTORGBA( "#84cef9" ),
                                         GeomMappingData{ V2f{ 2.0f }}, GT::Tag( ArchType::WallT ));
        ret.emplace_back( mainWall );

        // caps
        if ( csize > 2 ) {
//        sg.GB<GT::Poly>( mWall->epoints, GT::M("plaster_ultra_fine_spray"), C4f{C4f::PEACH.xyz(), 1.0f},
//                         V3f::UP_AXIS * ( mWall->z + mWall->height) );
//        sg.GB<GT::Poly>( mWall->epoints, GT::M("plaster_ultra_fine_spray"), C4f{C4f::PEACH.xyz(), 1.0f},
//                         ReverseFlag::True,
//                         V3f::UP_AXIS * mWall->z );
//        if ( isPartOfDoorOrWindow ) {
//            sg.GB<GT::Poly>( mWall->epoints, GT::M( "plaster_ultra_fine_spray" ), C4f{ C4f::PEACH.xyz(), 1.0f },
//                             ReverseFlag::True,
//                             V3f::UP_AXIS * ( mWall->z ));
//        }
        }

        return ret;
    }

}