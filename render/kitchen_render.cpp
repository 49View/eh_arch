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

    void addWorktopSegment( KitchenData& kd, const V2f& p1, const V2f& p2, const V2f& normal, bool isMain ) {
        V2f inward = ( normal * kd.kitchenWorktopDepth * 0.5f );
        V2f inwardSkirting = ( normal * ( kd.kitchenWorktopDepth - kd.kitchenSkirtingRecess ) );
        V2f inwardUnits = ( normal * ( kd.kitchenWorktopDepth - kd.kitchenUnitsRecess ) );

        kd.kitchenWorktopPath.emplace_back(p1 + inward, p2 + inward, normal);
        auto middle = lerp(0.5f, p1, p2);
        V2f lp1Dir = normalize(middle - p1);
        V2f lp2Dir = normalize(middle - p2);
        V2f inW1 = isMain ? ( lp1Dir * kd.kitchenWorktopDepth ) : V2f::ZERO;
        V2f inW2 = isMain ? ( lp2Dir * kd.kitchenWorktopDepth ) : V2f::ZERO;
        kd.kitchenSkirtingPath.emplace_back(p1 + inwardSkirting + inW1, p2 + inwardSkirting + inW2, normal);
        kd.kitchenUnitsPath.emplace_back(p1 + inwardUnits + inW1, p2 + inwardUnits + inW2, normal);
    }

    void createMasterPath( RoomBSData *w, KitchenData& kd ) {
        for ( const auto& lsref : w->mWallSegmentsSorted ) {
            const auto* ls = &lsref;
            auto df = RS::walkAlongWallsUntilCornerChanges(w, ls, WalkSegmentDirection::Left, IncludeWindowsOrDoors::WindowsOnly);
            auto dfr = RS::walkAlongWallsUntilCornerChanges(w, ls, WalkSegmentDirection::Right, IncludeWindowsOrDoors::WindowsOnly);

            if ( df.distance <= kd.kitchenWorktopDepth || dfr.distance <= kd.kitchenWorktopDepth ) {
                continue;
            }

            addWorktopSegment(kd, ls->p1, ls->p2, ls->normal, true);

            V2f inward = ( ls->normal * kd.kitchenWorktopDepth);
            V2f lsp1 = ls->p1 + inward;
            V2f lsp2 = ls->p2 + inward;

            addWorktopSegment(kd, lsp2, lsp2 + ls->normal * (dfr.distance - (kd.kitchenWorktopDepth)), dfr.normal, false);
            addWorktopSegment(kd, lsp1, lsp1 + ls->normal * (df.distance - (kd.kitchenWorktopDepth)), df.normal, false);
            break;
        }
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

    void createUnits( SceneGraph& sg, RoomBSData *w, KitchenData& kd ) {

        float dp = kd.drawersPadding.x();
        float drawW = kd.longDrawersSize.x();
        float unitEdgeEnd = kd.drawersThickness;
        float topOfWorktop = kd.kitchenWorktopHeight + kd.worktopThickness;

        std::pair<size_t, size_t> targetWall;
        V2f hittingPoint;
        auto cookerHalfWidth = sg.GM(kd.extractorHoodModel)->BBox3d()->calcWidth() * 0.5f;
        auto sinkHalfWidth = sg.GM(kd.sinkModel)->BBox3d()->calcWidth() * 0.5f;
        auto carryingIndex = 0u;
        for ( auto t = 0u; t < kd.kitchenUnitsPath.size(); t++ ) {
            auto& kup = kd.kitchenUnitsPath[t];
            float posDelta = 0.5f;
            auto mp = lerp(posDelta, kup.p1, kup.p2);
            if ( distance(mp, kup.p1) > cookerHalfWidth && distance(mp, kup.p2) > cookerHalfWidth ) {
                bool hit = RoomService::findOppositeWallFromPoint(w, mp, -kup.normal,
                                                                  targetWall,
                                                                  hittingPoint);
                if ( hit ) {
                    kup.cookerPos = hittingPoint;
                    kup.hasCooker = true;
                    carryingIndex = t + 1;
                    break;
                }
            }
        }
        for ( auto t = 0u; t < kd.kitchenUnitsPath.size(); t++ ) {
            auto& kup = kd.kitchenUnitsPath[cai(t + carryingIndex, kd.kitchenUnitsPath.size())];
            float posDelta = 0.5f;
            auto mp = lerp(posDelta, kup.p1, kup.p2);
            if ( distance(mp, kup.p1) > sinkHalfWidth && distance(mp, kup.p2) > sinkHalfWidth ) {
                if ( !kup.hasCooker || distance(mp, kup.cookerPos) > cookerHalfWidth + sinkHalfWidth ) {
                    bool hit = RoomService::findOppositeWallFromPoint(w, mp, -kup.normal, targetWall,
                                                                      hittingPoint, IncludeWindowsOrDoors::WindowsOnly);
                    if ( hit ) {
                        kup.sinkPos = hittingPoint;
                        kup.hasSink = true;
                        break;
                    }
                }
            }
        }

        for ( const auto kup : kd.kitchenUnitsPath ) {
            V3f p1 = { kup.p1, kd.kitchenWorktopHeight - kd.worktopThickness * 0.5f };
            V3f p2 = { kup.p2, kd.kitchenWorktopHeight - kd.worktopThickness * 0.5f };
            V3f pDir = normalize(p2 - p1);
            auto rotation = Quaternion{ RoomService::furnitureAngleFromNormal(kup.normal), V3f::UP_AXIS };
            if ( kup.hasCooker ) {
                auto mp = kup.cookerPos;
                sg.GB<GT::Asset>(kd.cooktopModel,
                                 XZY::C(mp + kup.normal * kd.kitchenWorktopDepth * 0.5f, topOfWorktop),
                                 GT::Rotate(rotation));
                sg.GB<GT::Asset>(kd.extractorHoodModel,
                                 XZY::C(mp + kup.normal * kd.kitchenWorktopDepth * 0.5f, topOfWorktop + 0.5f),
                                 GT::Rotate(rotation));
                sg.GB<GT::Asset>(kd.ovenPanelModel,
                                 XZY::C(mp + kup.normal * ( kd.kitchenWorktopDepth - kd.kitchenUnitsRecess ),
                                        topOfWorktop - kd.worktopThickness), GT::Rotate(rotation));
            }
            if ( kup.hasSink ) {
                auto mp = kup.sinkPos;
                auto middleP = oppositePointOnWallFor(w, mp, -kup.normal);
                sg.GB<GT::Asset>(kd.sinkModel,
                                 XZY::C(middleP + kup.normal * kd.kitchenWorktopDepth * 0.5f, topOfWorktop),
                                 GT::Rotate(rotation));
            }

            float widthOfSegment = distance(p1, p2);
            float widthForUnitsAvailable = ( widthOfSegment - unitEdgeEnd * 2.0f );
            float widthOfDrawersWithGap = ( kd.longDrawersSize.x() + dp * 2.0f );
            int numDrawers = (int) ( widthForUnitsAvailable / widthOfDrawersWithGap );
            float finalPadding = fmodf(widthForUnitsAvailable, widthOfDrawersWithGap);
            for ( int q = 0; q < numDrawers + 1; q++ ) {
                if ( ( q == ( numDrawers ) ) && ( finalPadding < 0.01f ) ) continue;
                float drawerWidth = ( q == numDrawers ) ? finalPadding : drawW;
                Vector3f a = ( p1 + pDir * unitEdgeEnd ) +
                             ( ( pDir * ( kd.longDrawersSize.x() + dp ) ) * (float) q );
                Vector3f b = a + ( pDir * ( drawerWidth - dp ) );

                float unitYSizeAvailable = kd.kitchenWorktopHeight - kd.skirtingHeight + ( dp * 2 );
                for ( int m = 0; m < 2; m++ ) {
                    float unitHeight = m == 0 ? unitYSizeAvailable * 0.75f : unitYSizeAvailable * .25f;
                    float uz = kd.skirtingHeight + ( m == 0 ? 0.0f : unitYSizeAvailable * 0.75f + dp );
                    auto linex = FollowerService::createLinePath(a, b, kd.drawersThickness, uz);
                    sg.GB<GT::Extrude>(PolyOutLine{ linex, V3f::UP_AXIS, unitHeight },
                                       GT::M(kd.unitsMaterial));
                    auto handleWidth = sg.GM(kd.drawersHandleModel)->BBox3d()->calcWidth();
                    if ( distance(a,b) > handleWidth * 1.2f) {
                        sg.GB<GT::Asset>(kd.drawersHandleModel,
                                         XZY::C(lerp(0.5f, a, b) + kup.normal * kd.drawersThickness * 0.5f,
                                                uz + unitHeight * 0.5f), GT::Rotate(rotation));
                    }
                }
            }
        }
    }

    void render( SceneGraph& sg, RoomBSData *w, HouseRenderContainer& ret ) {
        KitchenData kd = w->kitchenData;
        createMasterPath(w, kd);
        createUnits(sg, w, kd);

        for ( const auto kwp : kd.kitchenWorktopPath ) {
            auto linex = FollowerService::createLinePath(kwp.p1, kwp.p2, kd.kitchenWorktopDepth,
                                                         kd.kitchenWorktopHeight);
            sg.GB<GT::Extrude>(PolyOutLine{ linex, V3f::UP_AXIS, kd.worktopThickness }, GT::M(kd.worktopMaterial));
        }

        for ( const auto kwp : kd.kitchenSkirtingPath ) {
            auto linex = FollowerService::createLinePath(kwp.p1, kwp.p2, 0.02f, 0.0f);
            sg.GB<GT::Extrude>(PolyOutLine{ linex, V3f::UP_AXIS, kd.skirtingHeight }, GT::M(kd.unitsMaterial));
        }

//        auto linex = FollowerService::createLinePath(kp[2], kp[3], kd.kitchenWorktopDepth, kd.kitchenWorktopHeight);
//        sg.GB<GT::Extrude>(PolyOutLine{ linex, V3f::UP_AXIS, kd.worktopThickness }, GT::M(kd.worktopMaterial));

//        auto wtop2 = std::make_shared<Profile>();
//        wtop2->createRect(V2f{ 0.02f, kitchenData.skirtingHeight });
//        sg.GB<GT::Follower>(XZY::C(kitchenData.kitchenSkirtingPath, kitchenData.skirtingHeight * 0.5f), wtop2);

    }

}

