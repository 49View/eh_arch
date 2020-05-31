//
//  wall_render.cpp
//  sixthmaker
//
//  Created by Dado on 19/03/2017.
//
//

#include "wall_render.hpp"

#include <core/resources/resource_builder.hpp>
#include <poly/scene_graph.h>
#include <graphics/renderer.h>

#include <eh_arch/controller/arch_render_controller.hpp>

#include "house_render.hpp"
#include "../models/house_bsdata.hpp"
#include "../models/wall_service.hpp"

namespace WallRender {

    void drawWalls2d( Renderer& rr, const WallBSData *wall, DShaderMatrix sm, const IMHouseRenderSettings& ims ) {
        auto color = ims.getFillColor(wall->hash, C4f::BLACK);
        rr.draw<DPoly>(sm, wall->mTriangles2d, color, ims.pm());
    }

//    void drawIncrementalAlphaWalls2d( Renderer& rr, const WallBSData *wall, float width, DShaderMatrix sm,
//                                      const RDSPreMult& _pm ) {
//        auto wc = Color4f::RANDA1();
//        auto wps = wall->epoints.size();
//        for ( size_t t = 0; t < wps - 1 * !wall->wrapLastPoint; t++ ) {
//            float incRatio = float(t) / float(wps);
//            auto wca = Color4f::WHITE * V4f{ V3f::ONE, 0.5f + ( incRatio * 0.5f ) };
//            auto p1 = wall->epoints[t];
//            auto p2 = wall->epoints[cai(t + 1, wps)];
//            auto pm = lerp(0.5f, p1, p2);
//            rr.draw<DLine>(p1, p2, wc * wca, width, sm, _pm);
//            rr.draw<DLine>(pm, pm + wall->enormals[t] * width * 3.0f, Color4f::PASTEL_CYAN, width * 0.05f,
//                           true, sm, _pm);
//        }
//    }

    void drawUShapes2d( Renderer& rr, const WallBSData *wall, float width, DShaderMatrix sm,
                        const IMHouseRenderSettings& ims ) {
        std::array<Color4f, 3> usc = { Color4f::PASTEL_YELLOW, Color4f::PASTEL_CYAN, Color4f::PASTEL_GREEN };
        for ( const auto& us : wall->mUShapes ) {
            for ( int t = 0; t < 3; t++ ) {
                rr.draw<DLine>(us.points[t], us.points[t + 1], usc[t], width * 0.2f, sm, ims.pm());
            }
            rr.draw<DCircleFilled>(us.middle, Color4f::DARK_BLUE, 0.025f, sm, ims.pm());
        }
    }

    void drawWallNormals2d( Renderer& rr, const WallBSData *wall, float width, DShaderMatrix sm,
                            const IMHouseRenderSettings& ims ) {
        auto wps = wall->epoints.size();
        for ( size_t t = 0; t < wps - 1 * !wall->wrapLastPoint; t++ ) {
            auto p1 = wall->epoints[t];
            auto p2 = wall->epoints[cai(t + 1, wps)];
            auto pm = lerp(0.5f, p1, p2);
            rr.draw<DLine>(pm, pm + wall->enormals[t] * 0.15f, Color4f::PASTEL_CYAN, width, sm, true, ims.pm());
        }
    }

    void drawWallPoints2d( Renderer& rr, const WallBSData *wall, float width, DShaderMatrix sm,
                           const IMHouseRenderSettings& ims ) {
        for ( auto t = 0u; t < wall->epoints.size(); t++ ) {
            auto p1 = wall->epoints[t];
            ArchStructuralFeatureDescriptor asf{ ArchStructuralFeature::ASF_Point, t, wall->hash };
            auto color = ims.getFillColor(asf, C4f::RED);
            float radius = 0.03f;
            if ( color != C4f::RED ) {
                radius *= 2.0f;
            }
            rr.draw<DCircleFilled>(p1, color, radius, sm, ims.pm());
        }
    }

    void IMHouseRender( Renderer& rr, SceneGraph& sg, const WallBSData *wall, const IMHouseRenderSettings& ims ) {
        auto sm = ims.floorPlanShader();
        auto width = ims.floorPlanScaler(0.05f);
        drawWalls2d(rr, wall, sm, ims);
        bool drawDebug = ims.isFloorPlanRenderModeDebug();
        if ( drawDebug ) {
            auto lineWidth = ims.floorPlanScaler(0.01f);
            drawWallNormals2d(rr, wall, lineWidth, sm, ims);
            drawUShapes2d(rr, wall, width, sm, ims);
            drawWallPoints2d(rr, wall, width, sm, ims);
        }
    }

    GeomSPContainer make3dGeometry( SceneGraph& sg, const WallBSData *mWall,
                                    const V3fVectorOfVector& ceilingContours ) {
        GeomSPContainer ret;
        size_t csize = mWall->epoints.size();
        size_t wrapExtraVert = mWall->wrapLastPoint != 0 ? 0 : 1;

        std::vector<QuadVector3fNormal> wallQuads;
        for ( auto t = 0u; t < csize - wrapExtraVert; t++ ) {
            if ( WallService::checkUShapeIndexStartIsDoorOrWindow(mWall, t) ) {
                continue;
            }
            for ( const auto& quad : WallService::vertsForWallAt(mWall, t, ceilingContours) ) {
                wallQuads.emplace_back(QuadVector3fNormal{ quad, XZY::C(mWall->enormals[t], 0.0f) });
            }
        }

        auto mainWall = sg.GB<GT::Mesh>(wallQuads, GeomMappingData{ V2f{ 1.0f } }, GT::Tag(ArchType::WallT));
        ret.emplace_back(mainWall);

        return ret;
    }

    std::vector<QuadVector3fNormal> createArchSegmentQuads( const std::vector<ArchSegment>& wss ) {
        std::vector<QuadVector3fNormal> wallQuads;
        for ( const auto& ws : wss ) {
            for ( const auto zh : ws.zHeights ) {
                V3f v1 = XZY::C(ws.p1, zh.x());
                V3f v2 = XZY::C(ws.p2, zh.x());
                V3f v3 = v1 + V3f::UP_AXIS * zh.y();
                V3f v4 = v2 + V3f::UP_AXIS * zh.y();
                auto quad = QuadVector3f{ { v1, v2, v4, v3 } };
                wallQuads.emplace_back(QuadVector3fNormal{ quad, XZY::C(ws.normal, 0.0f) });
            }
        }
        return wallQuads;
    }

    GeomSPContainer renderWalls( SceneGraph& sg, const std::vector<ArchSegment>& wss, const std::string& wallMaterial,
                                 const C4f& wallColor ) {
        GeomSPContainer ret;
        GeomMappingData mapping{ V2f{ 1.0f } };
        mapping.direction = MappingDirection::Y_POS;
        std::vector<QuadVector3fNormal> wallQuads = createArchSegmentQuads(wss);
        auto mainWall = sg.GB<GT::Mesh>(wallQuads, GT::M(wallMaterial), wallColor,
                                        mapping, GT::Tag(ArchType::WallT));
        ret.emplace_back(mainWall);
        return ret;
    }

}