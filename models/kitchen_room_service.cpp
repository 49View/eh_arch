//
// Created by dado on 17/05/2020.
//

#include "kitchen_room_service.hpp"

#include <core/math/path_util.h>
#include <core/resources/profile.hpp>
#include "../models/house_bsdata.hpp"
#include "../models/room_service.hpp"

namespace KitchenRoomService {
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
        float sho = kd.skirtingThickness * 0.5f;
        float dho = kd.drawersThickness * 0.5f;
        float skirtingOffset = ( kd.kitchenWorktopDepth - kd.kitchenSkirtingRecess - sho );
        float unitOffset = ( kd.kitchenWorktopDepth - kd.kitchenUnitsRecess - dho );
        V2f inward = ( normal * kd.kitchenWorktopDepth * 0.5f );
        V2f inwardSkirting = ( normal * skirtingOffset );
        V2f inwardUnits = ( normal * unitOffset );
        V2f crossNormal = normalize(p2 - p1);

        kd.kitchenWorktopPath.emplace_back(p1 + inward, p2 + inward, normal, crossNormal);
        auto middle = lerp(0.5f, p1, p2);
        V2f lp1Dir = normalize(middle - p1);
        V2f lp2Dir = normalize(middle - p2);

        V2f inW1 = isMain ? ( lp1Dir * ( skirtingOffset + sho ) ) : lp1Dir * -( kd.kitchenSkirtingRecess );
        V2f inW2 = isMain ? ( lp2Dir * ( skirtingOffset + sho ) ) : V2f::ZERO;

        kd.kitchenSkirtingPath.emplace_back(p1 + inwardSkirting + inW1, p2 + inwardSkirting + inW2, normal,
                                            crossNormal);

        inW1 = isMain ? ( lp1Dir * ( unitOffset + dho ) ) : lp1Dir * -kd.kitchenUnitsRecess;
        inW2 = isMain ? ( lp2Dir * ( unitOffset + dho ) ) : V2f::ZERO;

        kd.kitchenUnitsPath.emplace_back(p1 + inwardUnits + inW1, p2 + inwardUnits + inW2, normal, crossNormal);
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

    void addDrawersSequencially( KitchenData& kd, const V2f& p1s, const V2f& p2s, const V2f& normal ) {
        float dp = kd.drawersPadding.x();
        float drawW = kd.longDrawersSize.x();

        V3f p1 = { p1s, kd.kitchenWorktopHeight - kd.worktopThickness * 0.5f };
        V3f p2 = { p2s, kd.kitchenWorktopHeight - kd.worktopThickness * 0.5f };
        V3f pDir = normalize(p2 - p1);

        float widthOfSegment = distance(p1, p2);
        float widthForUnitsAvailable = widthOfSegment;
        int numDrawers = (int) ( widthForUnitsAvailable / drawW );
        float finalPadding = fmodf(widthForUnitsAvailable, drawW);
        for ( int q = 0; q < numDrawers + 1; q++ ) {
            if ( ( q == ( numDrawers ) ) && ( finalPadding < 0.01f ) ) continue;
            float drawerWidth = ( q == numDrawers ) ? finalPadding : drawW;
            Vector3f a = p1 + ( pDir * drawW * (float) q ) + ( pDir * dp );
            Vector3f b = a + ( pDir * ( drawerWidth - dp * 2.0f ) );
            kd.kitchenDrawers.emplace_back(a, b, normal, KitchenDrawerType::FlatBasic);
        }
    }

    void addDrawersFiller( KitchenData& kd, const V2f& p1s, const V2f& p2s, const V2f& normal, const C4f& color ) {
        V3f p1 = { p1s, kd.kitchenWorktopHeight - kd.worktopThickness * 0.5f };
        V3f p2 = { p2s, kd.kitchenWorktopHeight - kd.worktopThickness * 0.5f };
        kd.kitchenDrawers.emplace_back(p1, p2, normal, KitchenDrawerType::Filler, color);
    }

    void addDrawersFromPoint( KitchenData& kd, float pointDelta, float gapWidth, const KitchenPath& kup ) {
        V2f pd = lerp(pointDelta, kup.p1, kup.p2);
        V2f pd1 = pd - kup.crossNormal * gapWidth;
        V2f pd2 = pd + kup.crossNormal * gapWidth;
        addDrawersSequencially(kd, kup.p1, pd1, kup.normal);
        addDrawersSequencially(kd, pd2, kup.p2, kup.normal);
        // Filler
        addDrawersFiller(kd, pd1, pd2, kup.normal, C4f::DARK_GRAY);
    }

    void createUnits( RoomBSData *w, FurnitureMapStorage &furns ) {
        KitchenData& kd = w->kitchenData;
        std::pair<size_t, size_t> targetWall;
        V2f hittingPoint;
        auto cookerHalfWidth = sg.GM(kd.extractorHoodModel)->BBox3d()->calcWidth() * 0.5f;
        auto ovenHalfWidth = sg.GM(kd.ovenPanelModel)->BBox3d()->calcWidth() * 0.5f;
        auto sinkHalfWidth = sg.GM(kd.sinkModel)->BBox3d()->calcWidth() * 0.5f;
        auto carryingIndex = 0u;
        for ( auto t = 0u; t < kd.kitchenWorktopPath.size(); t++ ) {
            auto& kup = kd.kitchenWorktopPath[t];
            float posDelta = 0.5f;
            auto mp = lerp(posDelta, kup.p1, kup.p2);
            if ( distance(mp, kup.p1) > cookerHalfWidth && distance(mp, kup.p2) > cookerHalfWidth ) {
                bool hit = RoomService::findOppositeWallFromPoint(w, mp, -kup.normal,
                                                                  targetWall,
                                                                  hittingPoint);
                if ( hit ) {
                    kup.cookerPos = hittingPoint;
                    kup.cookerPosDelta = posDelta;
                    kup.flags.hasCooker = true;
                    carryingIndex = t + 1;
                    break;
                }
            }
        }
        for ( auto t = 0u; t < kd.kitchenWorktopPath.size(); t++ ) {
            auto& kup = kd.kitchenWorktopPath[cai(t + carryingIndex, kd.kitchenUnitsPath.size())];
            float posDelta = 0.5f;
            auto mp = lerp(posDelta, kup.p1, kup.p2);
            if ( distance(mp, kup.p1) > sinkHalfWidth && distance(mp, kup.p2) > sinkHalfWidth ) {
                if ( !kup.flags.hasCooker || distance(mp, kup.cookerPos) > cookerHalfWidth + sinkHalfWidth ) {
                    bool hit = RoomService::findOppositeWallFromPoint(w, mp, -kup.normal, targetWall,
                                                                      hittingPoint, IncludeWindowsOrDoors::WindowsOnly);
                    if ( hit ) {
                        kup.sinkPos = hittingPoint;
                        kup.flags.hasSink = true;
                        break;
                    }
                }
            }
        }

        for ( auto t = 0u; t < kd.kitchenWorktopPath.size(); t++ ) {
            auto& kup = kd.kitchenUnitsPath[t];
            auto& kwp = kd.kitchenWorktopPath[t];
            if ( kwp.flags.hasCooker ) {
                addDrawersFromPoint(kd, kwp.cookerPosDelta, ovenHalfWidth, kup);
            } else {
                addDrawersSequencially(kd, kup.p1, kup.p2, kup.normal);
            }
        }
    }

    void createMasterPath( RoomBSData *w, FurnitureMapStorage &furns ) {
        KitchenData& kd = w->kitchenData;
        for ( const auto& lsref : w->mWallSegmentsSorted ) {
            const auto *ls = &lsref;
            auto df = RS::walkAlongWallsUntilCornerChanges(w, ls, WalkSegmentDirection::Left,
                                                           IncludeWindowsOrDoors::WindowsOnly);
            auto dfr = RS::walkAlongWallsUntilCornerChanges(w, ls, WalkSegmentDirection::Right,
                                                            IncludeWindowsOrDoors::WindowsOnly);

            if ( df.distance <= kd.kitchenWorktopDepth || dfr.distance <= kd.kitchenWorktopDepth ) {
                continue;
            }

            addWorktopSegment(kd, ls->p1, ls->p2, ls->normal, true);

//            auto fridgeWidth = sg.GM(kd.fridgeModel)->BBox3d()->calcWidth();

            V2f inward = ( ls->normal * kd.kitchenWorktopDepth );
            V2f lsp1 = ls->p1 + inward;
            V2f lsp2 = ls->p2 + inward;

            addWorktopSegment(kd, lsp2, lsp2 + ls->normal * ( dfr.distance - ( kd.kitchenWorktopDepth ) ), dfr.normal,
                              false);
            addWorktopSegment(kd, lsp1, lsp1 + ls->normal * ( df.distance - ( kd.kitchenWorktopDepth ) ), df.normal,
                              false);
            break;
        }
    }

}