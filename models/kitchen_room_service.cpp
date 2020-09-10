//
// Created by dado on 17/05/2020.
//

#include "kitchen_room_service.hpp"

#include <core/math/path_util.h>
#include <core/util.h>
#include "../models/house_bsdata.hpp"
#include "../models/room_service.hpp"
#include "../models/room_service_furniture.hpp"
#include "arch_segment_service.hpp"

namespace KitchenRoomService {

    namespace KitchenDrawerCreateFlags {
        static const uint64_t None = 0;
        static const uint64_t NoDepth = 1 << 0;
        static const uint64_t HasFakeFiller = 1 << 1;
    };

    using KitchenDrawerCreateFlagsT = uint64_t;

    void calcLShapeOffset( const V3f& p1, const V3f& p2, const V3f& p3, const V2f& n1, const V2f& n2, float depth,
                           float offset, std::vector<Vector3f>& path ) {
        Vector2f p1s = ( p1 + ( Vector3f(n1, 0.0f) * ( depth - offset ) ) ).xy();
        Vector2f p3s = ( p3 + ( Vector3f(n2, 0.0f) * ( depth - offset ) ) ).xy();
        Vector2f p2s = V2fc::ZERO;
        if ( !intersection(p1s, p1s + normalize(p2.xy() - p1.xy()) * 10000.0f, p3s,
                           p3s + normalize(p2.xy() - p3.xy()) * 10000.0f, p2s) ) {
            ASSERT(0);
        }
        path.emplace_back(p1s);
        path.emplace_back(p2s);
        path.emplace_back(p3s);
    }

    void addWorktopSegment( FloorBSData *f, RoomBSData *w, FurnitureMapStorage& furns, KitchenData& kd, const V2f& p1,
                            const V2f& p2, const V2f& normal, bool isMain ) {
        float sho = kd.skirtingThickness * 0.5f;
        float dho = kd.drawersThickness * 0.5f;
        float skirtingOffset = ( kd.kitchenWorktopDepth - kd.kitchenSkirtingRecess - sho );
        float unitOffset = ( kd.kitchenWorktopDepth - kd.kitchenUnitsRecess - dho );
        float masterOffset = kd.kitchenWorktopDepth * 0.5f;
        V2f inward = ( normal * masterOffset );
        V2f inwardSkirting = ( normal * skirtingOffset );
        V2f inwardUnits = ( normal * unitOffset );
        V2f crossNormal = normalize(p2 - p1);

        kd.kitchenWorktopPath.emplace_back(p1 + inward, p2 + inward, normal, crossNormal, masterOffset);
        auto middle = lerp(0.5f, p1, p2);
        V2f lp1Dir = normalize(middle - p1);
        V2f lp2Dir = normalize(middle - p2);

        bool needsSpaceForUnitsOnLeft = isMain & ( kd.kitchenShape == KS_UShape );
        bool needsSpaceForUnitsOnRight =
                isMain & ( ( kd.kitchenShape == KS_UShape ) || ( kd.kitchenShape == KS_LShape ) );

        V2f inW1 = needsSpaceForUnitsOnLeft ? ( lp1Dir * ( skirtingOffset + sho ) ) : lp1Dir *
                                                                                      -( kd.kitchenSkirtingRecess );
        V2f inW2 = needsSpaceForUnitsOnRight ? ( lp2Dir * ( skirtingOffset + sho ) ) : V2fc::ZERO;

        kd.kitchenSkirtingPath.emplace_back(p1 + inwardSkirting + inW1, p2 + inwardSkirting + inW2, normal,
                                            crossNormal, skirtingOffset);

        inW1 = needsSpaceForUnitsOnLeft ? ( lp1Dir * ( unitOffset + dho ) )
                                        : V2fc::ZERO;// lp1Dir * -kd.kitchenUnitsRecess;
        inW2 = needsSpaceForUnitsOnRight ? ( lp2Dir * ( unitOffset + dho ) ) : V2fc::ZERO;

        kd.kitchenUnitsPath.emplace_back(p1 + inwardUnits + inW1, p2 + inwardUnits + inW2, normal, crossNormal,
                                         unitOffset);
    }

    void
    addTopWorktopSegment( FloorBSData *f, RoomBSData *w, FurnitureMapStorage& furns, KitchenData& kd, const V2f& p1,
                          const V2f& p2, const V2f& normal, bool isMain ) {
        float dho = kd.drawersThickness * 0.5f;
        V2f crossNormal = normalize(p2 - p1);

        float topUnitOffset = ( kd.kitchenWorktopDepth - kd.kitchenTopUnitsRecess - dho );
        V2f inwardTopUnits = ( normal * topUnitOffset );

        auto middle = lerp(0.5f, p1, p2);
        V2f lp1Dir = normalize(middle - p1);
        V2f lp2Dir = normalize(middle - p2);

        bool needsSpaceForUnitsOnLeft = isMain & ( kd.kitchenShape == KS_UShape );
        bool needsSpaceForUnitsOnRight =
                isMain & ( ( kd.kitchenShape == KS_UShape ) || ( kd.kitchenShape == KS_LShape ) );

        V2f inW1 = needsSpaceForUnitsOnLeft ? ( lp1Dir * ( topUnitOffset + dho ) ) : V2fc::ZERO;
        V2f inW2 = needsSpaceForUnitsOnRight ? ( lp2Dir * ( topUnitOffset + dho ) ) : V2fc::ZERO;

        kd.kitchenTopUnitsPath.emplace_back(p1 + inwardTopUnits + inW1, p2 + inwardTopUnits + inW2, normal,
                                            crossNormal, topUnitOffset);
    }

    bool middleDrawerIndex( int index, int range ) {
        return ( range % 2 == 0 ) ? index == range / 2 - 1 : index == ( range - 1 ) / 2;
    }

    void addDrawer( KitchenData& kd, const V2f& p1s, const V2f& p2s, float z, float unitHeight, float depth,
                    const V2f& normal, KitchenDrawerTypeT drawerType,
                    KitchenDrawerCreateFlagsT hff = KitchenDrawerCreateFlags::None, const C4f& color = C4fc::WHITE ) {
        float computedDepth = checkBitWiseFlag(hff, KitchenDrawerCreateFlags::NoDepth) ? 0.0f : depth;
        kd.kitchenDrawers.emplace_back(p1s, p2s, z, unitHeight, computedDepth, normal, KitchenDrawerShape{drawerType}, color);
    }

    V2f oppositePointOnWallFor( RoomBSData *w, const V2f& input, const V2f& direction ) {
        V2f ln1 = direction;
        V2f ln2 = -ln1;
        V2f ln = V2fc::ZERO;
        std::pair<size_t, size_t> targetWall[2];
        V2f hittingPoint[2];
        V2f ret = V2fc::ZERO;
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

    void addDrawersSequencially( KitchenData& kd, const V2f& p1s, const V2f& p2s, float z, float unitHeight,
                                 float depth, const V2f& normal, KitchenDrawerTypeT drawerType,
                                 KitchenDrawerCreateFlagsT hff = KitchenDrawerCreateFlags::None ) {
        float dp = kd.drawersPadding.x();
        float drawW = kd.longDrawersSize.x();

        V2f p1 = p1s;
        V2f p2 = p2s;
        V2f pDir = normalize(p2 - p1);

        float widthOfSegment = distance(p1, p2);
        float widthForUnitsAvailable = widthOfSegment;
        int numDrawers = (int) ( widthForUnitsAvailable / drawW );
        float finalPadding = fmodf(widthForUnitsAvailable, drawW);
        for ( int q = 0; q < numDrawers + 1; q++ ) {
            if ( ( q == ( numDrawers ) ) && ( finalPadding < 0.01f ) ) continue;
            float drawerWidth = ( q == numDrawers ) ? finalPadding : drawW;
            Vector3f a = p1 + ( pDir * drawW * (float) q ) + ( pDir * dp );
            Vector3f b = a + ( pDir * ( drawerWidth - dp * 2.0f ) );
            addDrawer(kd, a, b, z, unitHeight, depth, normal, drawerType, hff);
        }
    }

    void addDrawersFromPoint( KitchenData& kd, float z, float unitHeight, float pointDelta, float gapWidth,
                              const KitchenPath& kup, KitchenDrawerCreateFlagsT hff = KitchenDrawerCreateFlags::None ) {
        V2f pd = lerp(pointDelta, kup.p1, kup.p2);
        V2f pd1 = pd - kup.crossNormal * gapWidth;
        V2f pd2 = pd + kup.crossNormal * gapWidth;
        addDrawersSequencially(kd, kup.p1, pd1, z, unitHeight, kup.depth, kup.normal, KitchenDrawerType::FlatBasic,
                               hff);
        addDrawersSequencially(kd, pd2, kup.p2, z, unitHeight, kup.depth, kup.normal, KitchenDrawerType::FlatBasic,
                               hff);
        // Filler
        if ( checkBitWiseFlag(hff, KitchenDrawerCreateFlags::HasFakeFiller) ) {
            addDrawer(kd, pd1, pd2, z, unitHeight, kup.depth, kup.normal, KitchenDrawerType::Filler, hff,
                      C4fc::DARK_GRAY);
        }
    }


    void createWorktopAppliancies( FloorBSData *f, RoomBSData *w, FurnitureMapStorage& furns ) {
        KitchenData& kd = w->kitchenData;
        auto kwp = kd.kitchenWorktopPath[0];
        auto coffeeMachine = furns.spawn(FTH::FT_CoffeeMachine);
        auto rot = Quaternion{ RoomService::furnitureAngleFromNormal(kwp.normal), V3fc::UP_AXIS };
        V2f pos = kwp.p1 + (kwp.crossNormal * coffeeMachine->Width());
        float height = kd.kitchenWorktopHeight + kd.worktopThickness;
        RS::placeManually(FurnitureRuleParams{ f, w, coffeeMachine, pos, rot,
                            height,
                            FRPWidthNormal{ kwp.crossNormal },
                            FRPDepthNormal{ kwp.normal },
                            FRPFurnitureRuleFlags{forceManualFurnitureFlags}
                          });
    }

    void createUnits( FloorBSData *f, RoomBSData *w, FurnitureMapStorage& furns ) {
        KitchenData& kd = w->kitchenData;
        std::pair<size_t, size_t> targetWall;
        V2f hittingPoint;
        auto cooker = furns.spawn(FTH::FT_Cooktop);
        auto oven = furns.spawn(FTH::FT_OvenPanel);
        auto sink = furns.spawn(FTH::FT_Sink);
        auto cookerHalfWidth = cooker->HalfWidth();
        auto ovenHalfWidth = oven->HalfWidth();
        auto sinkHalfWidth = sink->HalfWidth();
        auto carryingIndex = 0u;
        float floorLevel = kd.skirtingHeight;// kd.kitchenWorktopHeight - kd.worktopThickness * 0.5f;
        float unitHeight = kd.kitchenWorktopHeight - ( kd.skirtingHeight + ( kd.drawersPadding.x() * 2 ) );
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
                        kup.sinkPos = oppositePointOnWallFor(w, hittingPoint, -kup.normal);
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
                addDrawersFromPoint(kd, floorLevel, unitHeight, kwp.cookerPosDelta, ovenHalfWidth, kup,
                                    KitchenDrawerCreateFlags::HasFakeFiller | KitchenDrawerCreateFlags::NoDepth);
            } else {
                addDrawersSequencially(kd, kup.p1, kup.p2, floorLevel, unitHeight, kup.depth, kup.normal,
                                       KitchenDrawerType::FlatBasic, KitchenDrawerCreateFlags::NoDepth);
            }
        }
    }

    void createTopUnits( FloorBSData *f, RoomBSData *w, FurnitureMapStorage& furns ) {
        KitchenData& kd = w->kitchenData;

        for ( auto t = 0u; t < kd.kitchenWorktopPath.size(); t++ ) {
            auto& kup = kd.kitchenTopUnitsPath[t];
            auto& kwp = kd.kitchenWorktopPath[t];
            float z = w->Height() - kd.longDrawersSize.y() - kd.topUnitsCeilingGap;
            if ( kwp.flags.hasCooker ) {
                auto extractorHood = furns.spawn(FTH::FT_ExtractorHood);
                auto extractorHoodHalfWidth = extractorHood->HalfWidth();
                addDrawersFromPoint(kd, z, kd.longDrawersSize.y(), kwp.cookerPosDelta, extractorHoodHalfWidth, kup);
            } else {
                addDrawersSequencially(kd, kup.p1, kup.p2, z, kd.longDrawersSize.y(), kup.depth, kup.normal,
                                       KitchenDrawerType::FlatBasic);
            }
        }
    }

    V2f addFridge( FloorBSData *f, RoomBSData *w, FurnitureMapStorage& furns, const V2f& p1, const V2f& p2,
                   const V2f& normal,
                   const V2f& crossNormal ) {
        auto fridge = furns.spawn(FTH::FT_Fridge);
        auto fridgeWidth = fridge->Width();
        auto fridgeDepth = fridge->Depth();
        if ( distance(p1, p2) > fridgeWidth * 1.2f ) { // giving it a bit of slack (*1.2f)
            std::pair<size_t, size_t> targetWall;
            float fridgeSlackGap = 0.15f;
            V2f hittingPoint;
            V2f mp = p2 - ( crossNormal * ( fridgeWidth * 0.5f + fridgeSlackGap ) );
            bool hit = RoomService::findOppositeWallFromPoint(w, mp, -normal, targetWall,
                                                              hittingPoint, IncludeWindowsOrDoors::None);
            if ( hit ) {
                auto rot = Quaternion{ RoomService::furnitureAngleFromNormal(normal), V3fc::UP_AXIS };
                RS::placeManually(FurnitureRuleParams{ f, w, fridge, hittingPoint + ( normal * fridgeDepth * 0.5f ), rot,
                                    FRPWidthNormal{ crossNormal },
                                    FRPDepthNormal{ normal } });
                return mp - ( crossNormal * ( fridgeWidth * 0.5f ) );
            }
        }
        return p2;
    }

    void createMasterPath( FloorBSData *f, RoomBSData *w, FurnitureMapStorage& furns ) {
        KitchenData& kd = w->kitchenData;
        for ( const auto& lsref : w->mWallSegmentsSorted ) {
            const auto *ls = &lsref;
            auto dfl = RS::walkAlongWallsUntilCornerChanges(w, ls, WalkSegmentDirection::Left,
                                                            IncludeWindowsOrDoors::WindowsOnly);
            auto dfr = RS::walkAlongWallsUntilCornerChanges(w, ls, WalkSegmentDirection::Right,
                                                            IncludeWindowsOrDoors::WindowsOnly);

            float dflDistance = RS::lengthOfArchSegments(dfl);
            float dfrDistance = RS::lengthOfArchSegments(dfr);

            if ( dflDistance <= kd.kitchenWorktopDepth || dfrDistance <= kd.kitchenWorktopDepth ) {
                continue;
            }

            addWorktopSegment(f, w, furns, kd, ls->p1, ls->p2, ls->normal, true);
            addTopWorktopSegment(f, w, furns, kd, ls->p1, ls->p2, ls->normal, true);

            V2f inward = ( ls->normal * kd.kitchenWorktopDepth );

            V2f lsp1 = ls->p2 + inward;
            V2f lsp2 = lsp1 + ls->normal * ( dfrDistance - ( kd.kitchenWorktopDepth ) );
            addWorktopSegment(f, w, furns, kd, lsp1, lsp2, dfr[0]->normal, false);

            V2f lspl1 = ls->p1 + inward;
            V2f lspl2s = lspl1 + ls->normal * ( dflDistance - ( kd.kitchenWorktopDepth ) );
            // Place the fridge somewhere around the end of the secondary segment
            V2f lspl2 = addFridge(f, w, furns, lspl1, lspl2s, dfl[0]->normal, ls->normal);
            float fridgeCutDownDistance = distance(lspl2s, lspl2);

            addWorktopSegment(f, w, furns, kd, lspl1, lspl2, dfl[0]->normal, false);

            // Top units, they are detached from the worktop ones
            const auto *tr = RS::walkSegment(w, ls, WalkSegmentDirection::Right);
            const auto *tl = RS::walkSegment(w, ls, WalkSegmentDirection::Left);

            V2f lspt1 = ls->p2 + inward;
            V2f lspt2 = lspt1 + ls->normal * ( tr->length() - ( kd.kitchenWorktopDepth ) );
            addTopWorktopSegment(f, w, furns, kd, lspt1, lspt2, tr->normal, false);
            V2f lsplt1 = ls->p1 + inward;
            V2f lsplt2 = lsplt1 + ls->normal * ( tl->length() - ( kd.kitchenWorktopDepth ) - fridgeCutDownDistance );
            addTopWorktopSegment(f, w, furns, kd, lsplt1, lsplt2, tl->normal, false);

            break;
        }
    }

    void createMasterPathSingle( FloorBSData *f, RoomBSData *w, FurnitureMapStorage& furns ) {
        KitchenData& kd = w->kitchenData;
        if ( kd.kitchenIndexMainWorktop == -1 ) {
            setNextMainWorktopIndexCandidate(w);
        }
        auto ls = RoomService::segmentAtIndex(w, kd.kitchenIndexMainWorktop);
        if ( ls ) {
            ls->wallMaterial = kd.backSplashMaterial;
            addWorktopSegment(f, w, furns, kd, ls->p1, ls->p2, ls->normal, true);
            addTopWorktopSegment(f, w, furns, kd, ls->p1, ls->p2, ls->normal, true);
        }
    }

    void createKitchen( FloorBSData *f, RoomBSData *w, FurnitureMapStorage& furns ) {
        KitchenData& kd = w->kitchenData;
        switch ( kd.kitchenShape ) {
            case KS_Straight:
                KitchenRoomService::createMasterPathSingle(f, w, furns);
                break;
            case KS_LShape:
                KitchenRoomService::createMasterPath(f, w, furns);
                break;
            case KS_UShape:
                KitchenRoomService::createMasterPath(f, w, furns);
                break;
            case KS_Custom:
                KitchenRoomService::createMasterPath(f, w, furns);
                break;
        }

        if( !kd.kitchenWorktopPath.empty() ) {
            KitchenRoomService::createWorktopAppliancies(f, w, furns);
            KitchenRoomService::createUnits(f, w, furns);
            KitchenRoomService::createTopUnits(f, w, furns);
        }
    }

    void clear( RoomBSData *w ) {
        w->kitchenData.kitchenWorktopPath.clear();
        w->kitchenData.kitchenSkirtingPath.clear();
        w->kitchenData.kitchenUnitsPath.clear();
        w->kitchenData.kitchenTopUnitsPath.clear();
        w->kitchenData.kitchenDrawers.clear();
    }

    bool hasKitchen( const RoomBSData *w ) {
        return !w->kitchenData.kitchenWorktopPath.empty();
    }

    void setNextMainWorktopIndexCandidate( RoomBSData *room, GenericCallback ccf ) {
        size_t candidate = room->kitchenData.kitchenIndexMainWorktop + 1;
        for ( auto t = 0u; t < room->mWallSegmentsSorted.size(); t++ ) {
            if ( candidate >= room->mWallSegmentsSorted.size() ) {
                candidate = 0;
            }
            if ( !ArchSegmentService::isSegmentPartOfWindowOrDoor(room->mWallSegmentsSorted[candidate]) &&
                 room->mWallSegmentsSorted[candidate].length() > 1.5f ) {
                // All this massive code bit is to check that the edges of the worktop do not align with a door
                // If they do we allow a mininum safe distance (gap) to be taken into account.
                // (IE a long wall with a door on it's end that's quite far from the worktop end will be allowed)
                auto& ls = room->mWallSegmentsSorted[candidate];
                V2f hit = V2fc::ZERO;
                std::pair<size_t, size_t> targetWall;
                float gap = ls.length() * 0.5f + 0.6f;
                V2f p1 = ls.middle + ls.normal * room->kitchenData.kitchenWorktopDepth * 0.8f;
                if ( RoomService::findOppositeWallFromPointAllowingGap(room, p1, ls.crossNormal, targetWall,
                                                                       hit, IncludeWindowsOrDoors::WindowsOnly, gap) &&
                     RoomService::findOppositeWallFromPointAllowingGap(room, p1, -ls.crossNormal, targetWall,
                                                                       hit, IncludeWindowsOrDoors::WindowsOnly, gap) ) {
                    // Is all conditions suggested above are met, set the candidate index

                    // If the worktop index was already assigned, then revert the wall type to normal on the old one
                    if ( room->kitchenData.kitchenIndexMainWorktop != -1 ) {
                        room->mWallSegmentsSorted[room->kitchenData.kitchenIndexMainWorktop].wallMaterial = room->wallsMaterial;
                    }
                    room->kitchenData.kitchenIndexMainWorktop = candidate;
                    if ( ccf ) ccf();
                    break;
                }
            }
            ++candidate;
        }
    }
}