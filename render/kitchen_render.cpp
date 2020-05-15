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

    void calcLShapeOffset( const V3f& p1, const V3f& p2, const V3f& p3, const V2f& n1, const V2f& n2, float depth,
                           float offset, std::vector<Vector3f>& path ) {
        Vector2f p1s = ( p1 + ( Vector3f(n1, 0.0f) * ( depth - offset ) ) ).xy();
        Vector2f p3s = ( p3 + ( Vector3f(n2, 0.0f) * ( depth - offset ) ) ).xy();
        Vector2f p2s = Vector2f::ZERO;
        if ( !intersection(p1s, p1s + normalize(p2.xy() - p1.xy()) * 10000.0f, p3s,
                           p3s + normalize(p2.xy() - p3.xy()) * 10000.0f, p2s) ) {
            ASSERT(0);
        }
        path.emplace_back(p1s);
        path.emplace_back(p2s);
        path.emplace_back(p3s);
    }

    void createMasterPath( RoomBSData *w, KitchenData& _kitchenData ) {
        const ArchSegment *ls = RoomService::longestSegment(w);
        std::pair<size_t, size_t> oppositeWallForLShape;
        Vector2f iPointlShape = Vector2f::ZERO;

        bool foundOpposite = RoomService::findOppositeWallFromPoint(w, ls->p2, ls->normal, oppositeWallForLShape,
                                                                    iPointlShape);
        if ( !foundOpposite ) {
            _kitchenData.kitchenWorktopPath.emplace_back(ls->p1);
            _kitchenData.kitchenWorktopPath.emplace_back(ls->p2);
            _kitchenData.kitchenSkirtingPath.emplace_back(ls->p1);
            _kitchenData.kitchenSkirtingPath.emplace_back(ls->p2);
        } else {
            ArchSegment *lso = &w->mWallSegments[oppositeWallForLShape.first][oppositeWallForLShape.second];
            Vector2f iPoint = Vector2f::ZERO;
            if ( distance(lso->p1, iPointlShape) < distance(lso->p2, iPointlShape) ) {
                iPoint = lso->p1 + ( lso->crossNormal * _kitchenData.kitchenWorktopDepth * 0.5f );
            } else {
                iPoint = lso->p2 + ( -lso->crossNormal * _kitchenData.kitchenWorktopDepth * 0.5f );
            }
            if ( RoomService::findOppositeWallFromPoint(w, iPoint, lso->normal, oppositeWallForLShape,
                                                        iPointlShape) ) {
                Vector3f p1 = { ( ls->p1 + ls->normal * _kitchenData.kitchenWorktopDepth * 0.5f ),
                                _kitchenData.kitchenWorktopHeight };
                Vector3f p2 = { ( iPointlShape +
                                  w->mWallSegments[oppositeWallForLShape.first][oppositeWallForLShape.second].normal *
                                  _kitchenData.kitchenWorktopDepth * 0.5f ), _kitchenData.kitchenWorktopHeight };
                Vector3f p3 = { iPoint, _kitchenData.kitchenWorktopHeight };
                _kitchenData.kitchenWorktopPath.push_back(p1);
                _kitchenData.kitchenWorktopPath.push_back(p2);
                _kitchenData.kitchenWorktopPath.push_back(p3);
                _kitchenData.kitchenUnitsNormals.emplace_back(lso->normal);
                _kitchenData.kitchenUnitsNormals.emplace_back(lso->normal);
                _kitchenData.kitchenUnitsNormals.emplace_back(ls->normal);

                // Skirting
                calcLShapeOffset(p1, p2, p3, ls->normal, lso->crossNormal, _kitchenData.kitchenWorktopDepth * 0.5f,
                                 _kitchenData.kitchenSkirtingRecess, _kitchenData.kitchenSkirtingPath);
                calcLShapeOffset(p1, p2, p3, ls->normal, lso->crossNormal, _kitchenData.kitchenWorktopDepth * 0.5f,
                                 _kitchenData.kitchenUnitsRecess, _kitchenData.kitchenUnitsPath);
            }
        }

//        return std::make_shared<Follower>( mPrefs.mAccuracy, FollowerFlags::Defaults, "KitchenWorktop",
//                                           MappingDirection::Y_POS, true );
        ASSERT(_kitchenData.kitchenWorktopPath.size() > 0);
    }

    bool middleDrawerIndex( int index, int range ) {
        return ( range % 2 == 0 ) ? index == range / 2 - 1 : index == ( range - 1 ) / 2;
    }

    V2f oppositePointOnWallFor( RoomBSData *w, const V2f& input, const V2f& direction ) {
        V2f ln1 = direction;
        V2f ln2 = -ln1;
        V2f ln = V2f::ZERO;
        std::pair<size_t, size_t> targetWall[2];
        V2f hittingPoint[2];
        V2f ret = V2f::ZERO;
        bool fo1 = RoomService::findOppositeWallFromPoint(w, input, ln1, targetWall[0],
                                                          hittingPoint[0], IncludeWindowsOrDoors::WindowsOnly);
        bool fo2 = RoomService::findOppositeWallFromPoint(w, input, ln2, targetWall[1],
                                                          hittingPoint[1], IncludeWindowsOrDoors::WindowsOnly);
        ASSERT(fo1 || fo2);
        if ( distance(hittingPoint[0], input) < distance(hittingPoint[1], input) || ( fo1 && !fo2 ) ) {
            ln = -ln1;
            ret = hittingPoint[0];
        } else if ( distance(hittingPoint[0], input) > distance(hittingPoint[1], input) || ( fo2 && !fo1 ) ) {
            ln = -ln2;
            ret = hittingPoint[1];
        }
        return ret;
    }

    void createUnits( SceneGraph& sg, RoomBSData *w, KitchenData& _kitchenData ) {

        std::vector<Vector3f> wcoords = _kitchenData.kitchenUnitsPath;
        float dp = _kitchenData.drawersPadding.x();
        float drawW = _kitchenData.longDrawersSize.x();
        float unitEdgeEnd = _kitchenData.drawersThickness;
//        float topOfWorktop = _kitchenData.kitchenWorktopHeight + _kitchenData.worktopThickness * 0.5f;

        for ( size_t t = 0; t < wcoords.size() - 1; t++ ) {
            Vector3f p1 = { wcoords[t], _kitchenData.kitchenWorktopHeight - _kitchenData.worktopThickness * 0.5f };
            Vector3f p2 = { wcoords[cai(t + 1, wcoords.size())],
                            _kitchenData.kitchenWorktopHeight - _kitchenData.worktopThickness * 0.5f };
            Vector3f pDir = normalize(p2 - p1);

            float widthOfSegment = distance(p1, p2);
            float widthForUnitsAvailable = ( widthOfSegment - unitEdgeEnd * 2.0f );
            float widthOfDrawersWithGap = ( _kitchenData.longDrawersSize.x() + dp * 2.0f );
            int numDrawers = (int) ( widthForUnitsAvailable / widthOfDrawersWithGap );
            float finalPadding = fmodf(widthForUnitsAvailable, widthOfDrawersWithGap);
            for ( int q = 0; q < numDrawers + 1; q++ ) {
                if ( ( q == ( numDrawers ) ) && ( finalPadding < 0.01f ) ) continue;
                float drawerWidth = ( q == numDrawers ) ? finalPadding : drawW;
                Vector3f a = ( p1 + pDir * unitEdgeEnd ) +
                             ( ( pDir * ( _kitchenData.longDrawersSize.x() + dp ) ) * (float) q );
                Vector3f b = a + ( pDir * ( drawerWidth - dp ) );

                float unitYSizeAvailable = _kitchenData.kitchenWorktopHeight - _kitchenData.skirtingHeight + (dp*2);
                for ( int m = 0; m < 2; m++ ) {
                    float unitHeight = m == 0 ? unitYSizeAvailable*0.75f : unitYSizeAvailable*.25f;
                    float uz = _kitchenData.skirtingHeight + ( m == 0 ? 0.0f : unitYSizeAvailable*0.75f+dp );
                    auto linex = FollowerService::createLinePath(a, b, _kitchenData.drawersThickness, uz);
                    sg.GB<GT::Extrude>(PolyOutLine{ linex, V3f::UP_AXIS, unitHeight },
                                       GT::M(_kitchenData.unitsMaterial));
                    sg.GB<GT::Asset>(_kitchenData.drawersHandleModel, XZY::C(lerp(0.5f, a, b), uz));
                }

//                if ( t == 0 && middleDrawerIndex(q, numDrawers + 1) ) {
//                    auto middleP = oppositePointOnWallFor(w, lerp(0.5f, a, b), rotate90(pDir.xy()));
//                    sg.GB<GT::Asset>(_kitchenData.cooktopModel, XZY::C(middleP, topOfWorktop));
//                    sg.GB<GT::Asset>(_kitchenData.ovenPanelModel, XZY::C(lerp(0.5f, a, b)));
//                }
//                if ( t == 1 && middleDrawerIndex(q, numDrawers + 1) ) {
//                    auto middleP = oppositePointOnWallFor(w, lerp(0.5f, a, b), rotate90(pDir.xy()));
//                    sg.GB<GT::Asset>(_kitchenData.sinkModel, XZY::C(middleP, topOfWorktop));
//                }
            }
        }
    }

    void render( SceneGraph& sg, RoomBSData *w, HouseRenderContainer& ret ) {
        KitchenData kitchenData = w->kitchenData;
        createMasterPath(w, kitchenData);
        createUnits(sg, w, kitchenData);

        auto wtop = std::make_shared<Profile>();
        wtop->createRect(V2f{ kitchenData.kitchenWorktopDepth, kitchenData.worktopThickness });
        sg.GB<GT::Follower>(XZY::C(kitchenData.kitchenWorktopPath), wtop, GT::M(kitchenData.worktopMaterial));

        auto wtop2 = std::make_shared<Profile>();
        wtop2->createRect(V2f{ 0.02f, kitchenData.skirtingHeight });
        sg.GB<GT::Follower>(XZY::C(kitchenData.kitchenSkirtingPath, kitchenData.skirtingHeight * 0.5f), wtop2);

    }

}

