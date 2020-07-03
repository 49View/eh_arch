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

    void drawWalls2d( Renderer& rr, const WallBSData *wall, DShaderMatrix sm, const ArchRenderController& arc ) {
        auto color = arc.getFillColor(wall, C4f::BLACK);
        rr.draw<DPoly>(sm, wall->mTriangles2d, color, arc.pm(), wall->hashFeature("poly"+color.toString()+sm.hash(), 0));
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

    void drawUShapes2d( Renderer& rr, const WallBSData *wall, float lineWidth, DShaderMatrix sm,
                        const ArchRenderController& arc ) {
//        std::array<Color4f, 3> usc = { Color4f::PASTEL_YELLOW, Color4f::PASTEL_CYAN, Color4f::PASTEL_GREEN };
        int uShapeRC = 0;
        for ( const auto& us : wall->mUShapes ) {
//            for ( int t = 0; t < 3; t++ ) {
//                rr.draw<DLine>(us.points[t], us.points[t + 1], usc[t], lineWidth, sm, arc.pm());
//            }
            rr.draw<DCircleFilled>(us.middle, Color4f::DARK_BLUE, 0.035f, sm, arc.pm(), wall->hashFeature("w2dUShape"+sm.hash(),uShapeRC));
        }
    }

    void drawWallNormals2d( Renderer& rr, const WallBSData *wall, float width, DShaderMatrix sm,
                            const ArchRenderController& arc ) {
        auto wps = wall->epoints.size();
        for ( size_t t = 0; t < wps - 1 * !wall->wrapLastPoint; t++ ) {
            auto p1 = wall->epoints[t];
            auto p2 = wall->epoints[cai(t + 1, wps)];
            auto pm = lerp(0.5f, p1, p2);
            rr.draw<DLine>(pm, pm + wall->enormals[t] * 0.15f, Color4f::PASTEL_CYAN, width, sm, true, arc.pm(), wall->hashFeature("w2dNormal"+sm.hash(),t));
        }
    }

    void drawWallPoints2d( Renderer& rr, const WallBSData *wall, float width, DShaderMatrix sm,
                           const ArchRenderController& arc ) {
        for ( auto t = 0u; t < wall->epoints.size(); t++ ) {
            auto p1 = wall->epoints[t];
//            ArchStructuralFeatureDescriptor asf{ ArchStructuralFeature::ASF_Point, t, wall };
            auto color = arc.getFillColor(ArchStructuralFeature::ASF_Point, t, wall, C4f::RED);
            rr.draw<DCircleFilled>(p1, color, width, sm, arc.pm(), wall->hashFeature("w2dPoint"+color.toString()+sm.hash(),t));
        }
    }

    void drawWallContours2d( Renderer& rr, const WallBSData *wall, float width, DShaderMatrix sm,
                             const ArchRenderController& arc ) {
        for ( auto t = 0u; t < wall->epoints.size(); t++ ) {
            auto p1 = wall->epoints[t];
            auto p2 = wall->epoints[cai(t+1, wall->epoints.size())];
            auto color = arc.getFillColor(ArchStructuralFeature::ASF_Edge, t, wall, Color4f::PASTEL_GREEN);
            rr.draw<DLine>(p1, p2, color, width*3.f, sm, arc.pm(), wall->hashFeature("w2dEdge"+color.toString()+sm.hash(),t));
        }
//        if ( wall->wrapLastPoint ) vlist.emplace_back(wall->epoints[0]);
    }

    void IMHouseRender( Renderer& rr, SceneGraph& sg, const WallBSData *wall, const ArchRenderController& arc ) {

        // If a wall is part of a door or windows I think it's best to not to render it in IM 2d/3d as it will overlap
        // with the door/window render and also have extra duplicate vertices in common with the adjacent wall, hard
        // to select correctly
        if ( !WallService::isWindowOrDoorPart(wall) ) {
            auto sm = arc.floorPlanShader();
            auto width = arc.floorPlanScaler(0.05f);
            drawWalls2d(rr, wall, sm, arc);
            bool drawDebug = arc.isFloorPlanRenderModeDebug();
            if ( drawDebug ) {
                auto lineWidth = arc.floorPlanScaler(0.01f);
                drawWallContours2d(rr, wall, lineWidth*0.3f, sm, arc);
//                drawWallNormals2d(rr, wall, lineWidth*0.5f, sm, arc);
//                drawUShapes2d(rr, wall, lineWidth*0.5f, sm, arc);
                drawWallPoints2d(rr, wall, width*0.5f, sm, arc);
            }
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

    using WallMapper = std::map<MaterialAndColorProperty, std::vector<QuadVector3fNormal>>;

    WallMapper createArchSegmentQuads( const std::vector<ArchSegment>& wss, const MaterialAndColorProperty &wallMaterial ) {
        WallMapper wallQuads;
        for ( const auto& ws : wss ) {
            for ( const auto zh : ws.zHeights ) {
                V3f v1 = XZY::C(ws.p1, zh.x());
                V3f v2 = XZY::C(ws.p2, zh.x());
                V3f v3 = v1 + V3f::UP_AXIS * zh.y();
                V3f v4 = v2 + V3f::UP_AXIS * zh.y();
                auto quad = QuadVector3f{ { v1, v2, v4, v3 } };
                auto wallMat = ws.wallMaterial.materialHash.empty() ? wallMaterial : ws.wallMaterial;
                wallQuads[wallMat].emplace_back(QuadVector3fNormal{ quad, XZY::C(ws.normal, 0.0f) });
            }
        }
        return wallQuads;
    }

    GeomSPContainer renderWalls( SceneGraph& sg, const std::vector<ArchSegment>& wss, const MaterialAndColorProperty &wallMaterial ) {
        GeomSPContainer ret;
        GeomMappingData mapping{ V2f{ 1.0f } };
        mapping.direction = MappingDirection::Y_POS;
        WallMapper wallQuads = createArchSegmentQuads(wss, wallMaterial);
        for ( const auto& [mat, quads] : wallQuads ) {
            auto mainWall = sg.GB<GT::Mesh>(quads, mat, mapping, GT::Tag(ArchType::WallT));
            ret.emplace_back(mainWall);
        }
        return ret;
    }

}