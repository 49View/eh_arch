//
//  room_service.cpp
//  sixthview
//
//  Created by Dado on 16/7/2017.
//
//

#include "room_service.hpp"

#include <core/http/webclient.h>
#include <core/util.h>
#include <core/math/triangulator.hpp>
#include <core/service_factory.h>
#include <poly/scene_graph.h>
#include <poly/polyclipping/clipper.hpp>

#include "floor_service.hpp"
#include "arch_segment_service.hpp"
#include "room_service_furniture.hpp"
#include "kitchen_room_service.hpp"

bool FurnitureRuleScript::execute( FloorBSData *f, RoomBSData *r, FurnitureMapStorage& furns,
                                   const FurnitureRuleFunctionContainer& funcRules ) const {
    bool completed = true;
    for ( const auto& rule : rules ) {
        if ( rule.hasSomethingToDo() ) {
            completed &= funcRules[rule.getRuleFunctionIndex()](f, r, furns, rule);
        } else {
            completed = false;
        }
    }
    return completed;
}

void FurnitureRuleScript::clear() {
    rules.clear();
}

namespace RoomService {

    PivotPointPosition pivotPointPositionMirrorY( PivotPointPosition where ) {
        switch ( where ) {
            case PivotPointPosition::TopCenter:
                return PivotPointPosition::BottomCenter;
            case PivotPointPosition::TopLeft:
                return PivotPointPosition::TopRight;
            case PivotPointPosition::TopRight:
                return PivotPointPosition::TopLeft;
            case PivotPointPosition::LeftCenter:
                return PivotPointPosition::RightCenter;
            case PivotPointPosition::RightCenter:
                return PivotPointPosition::LeftCenter;
            default:
                return PivotPointPosition::Invalid;
        }
    }

    float furnitureAngleFromWall( const ArchSegment *ls ) {
        return atan2(ls->normal.y(), ls->normal.x()) - M_PI_2;
    }

    float furnitureAngleFromNormal( const Vector2f& normal ) {
        return atan2(normal.y(), normal.x()) - M_PI_2;
    }

    Vector2f furnitureNormalFromAngle( const float angle ) {
        return { cosf(angle + M_PI_2), sinf(angle + M_PI_2) };
    }

    const ArchSegment *longestSegment( const RoomBSData *r ) {
        return &r->mWallSegmentsSorted[0];
    }

    const ArchSegment *secondLongestSegment( const RoomBSData *r ) {
        if ( r->mWallSegmentsSorted.size() > 1 )
            return &r->mWallSegmentsSorted[1];
        return nullptr;
    }

    const ArchSegment *shortestSegment( const RoomBSData *r ) {
        return &r->mWallSegmentsSorted[r->mWallSegmentsSorted.size() - 1];
    }

    const ArchSegment *secondShortestSegment( const RoomBSData *r ) {
        if ( r->mWallSegmentsSorted.size() > 1 )
            return &r->mWallSegmentsSorted[r->mWallSegmentsSorted.size() - 2];
        return nullptr;
    }

    const ArchSegment *thirdShortestSegment( const RoomBSData *r ) {
        if ( r->mWallSegmentsSorted.size() > 2 )
            return &r->mWallSegmentsSorted[r->mWallSegmentsSorted.size() - 3];
        return nullptr;
    }

    ArchSegment *segmentAtIndex( RoomBSData *r, uint32_t _index ) {
        if ( r->mWallSegmentsSorted.size() <= _index ) return nullptr;
        return &r->mWallSegmentsSorted[_index];
    }

    const ArchSegment *segmentAtIndex( const RoomBSData *r, uint32_t _index ) {
        if ( r->mWallSegmentsSorted.size() <= _index ) return nullptr;
        return &r->mWallSegmentsSorted[_index];
    }

    const ArchSegment *segmentAt( const RoomBSData *r, roomTypeIndex rti ) {
        return &r->mWallSegments[rti.first][cai(rti.second, r->mWallSegments[rti.first].size())];
    }

    const ArchSegment *walkSegment( const RoomBSData *r, const ArchSegment *ls, WalkSegmentDirection wsd ) {
        auto rti = sortedSegmentToPairIndex(r, ls);
        rti.second += wsd == WalkSegmentDirection::Left ? -1 : 1;
        return segmentAt(r, rti);
    }

    void calcLongestWall( RoomBSData *r ) {
        float maxLength = 0.0f;

        r->mWallSegmentsSorted.clear();
        for ( auto& mw : r->mWallSegments ) {
            r->mWallSegmentsSorted.insert(std::end(r->mWallSegmentsSorted), std::begin(mw), std::end(mw));
        }

        std::sort(r->mWallSegmentsSorted.begin(), r->mWallSegmentsSorted.end(),
                  []( const ArchSegment& a, const ArchSegment& b ) -> bool {
                      return ArchSegmentService::length(a) > ArchSegmentService::length(b);
                  });

        for ( auto t = 0u; t < r->mWallSegmentsSorted.size(); t++ ) {
            float wlength = ArchSegmentService::length(r->mWallSegmentsSorted[t]);
            if ( wlength > maxLength ) {
                r->mLongestWall = t;
                maxLength = wlength;
            }
        }

        const ArchSegment *ls = longestSegment(r);
        Vector2f i{ V2fc::ZERO };
        r->mLongestWallOpposite = -1;
        // Find wall opposite
        for ( auto t = 0u; t < r->mWallSegmentsSorted.size(); t++ ) {
            if ( ( r->mWallSegmentsSorted[t].tag & WallFlags::WF_IsDoorPart ) > 0 ||
                 ( r->mWallSegmentsSorted[t].tag & WallFlags::WF_IsWindowPart ) > 0 ) {
                continue;
            }
            Vector2f normalOpposite = r->mWallSegmentsSorted[t].normal;
            if ( isScalarEqual(dot(normalOpposite, ls->normal), -1.0f) ) {
                if ( intersection(r->mWallSegmentsSorted[t].p1, r->mWallSegmentsSorted[t].p2, ls->middle,
                                  ls->middle + ( ls->normal * 1000.0f ), i) ) {
                    r->mLongestWallOpposite = t;
                    r->mLongestWallOppositePoint = i;
                    break;
                }
            }
        }
    }

    const ArchSegment *longestSegmentCornerP1( const RoomBSData *r ) {
        roomTypeIndex rti = sortedSegmentToPairIndex(r, r->mLongestWall);
        return &r->mWallSegments[rti.first][getCircularArrayIndex(rti.second - 1,
                                                                  static_cast<int32_t>( r->mWallSegments[rti.first].size()))];
    }

    const ArchSegment *longestSegmentCornerP2( const RoomBSData *r ) {
        roomTypeIndex rti = sortedSegmentToPairIndex(r, r->mLongestWall);
        return &r->mWallSegments[rti.first][getCircularArrayIndex(rti.second + 1,
                                                                  static_cast<int32_t>( r->mWallSegments[rti.first].size()))];
    }

    ArchSegment *longestSegmentOpposite( RoomBSData *r ) {
        if ( r->mLongestWallOpposite < 0 ) {
            return nullptr;
        }
        return &r->mWallSegmentsSorted[r->mLongestWallOpposite];
    }

    std::vector<const ArchSegment *>
    walkAlongWallsUntilCornerChanges( const RoomBSData *r, const ArchSegment *ls, WalkSegmentDirection wsd,
                                      IncludeWindowsOrDoors bwd ) {
        std::vector<const ArchSegment *> ret;
        bool bNormalsSimilar = true;
        bool bwdFlag = true;
        const ArchSegment *lswp = nullptr;
        while ( bNormalsSimilar && bwdFlag ) {
            const auto *lswn = walkSegment(r, lswp ? lswp : ls, wsd);
            if ( lswp ) bNormalsSimilar = isVerySimilar(lswp->normal, lswn->normal, 0.05f);
            bwdFlag = RS::checkIncludeDoorsWindowsFlag(lswn, bwd);
            if ( bNormalsSimilar && bwdFlag ) {
                ret.emplace_back(lswn);
                lswp = lswn;
            }
        }
        return ret;
    }

    float lengthOfArchSegments( const std::vector<const ArchSegment *>& input ) {
        float ret = 0.0f;
        for ( const auto& as : input ) {
            ret += as->length();
        }
        return ret;
    }

    float middleHeightFromObject( RoomBSData *r, FittedFurniture *base, FittedFurniture *dec ) {
        constexpr float sensibleMaxCovingHeight = 0.15f;
        return ( ( r->Height() - sensibleMaxCovingHeight - base->Height() - dec->Height() ) / 2.0f );
    }

    const ArchSegment *getWallSegmentFor( RoomBSData *r, const WSLO wslo, uint32_t _exactIndex ) {
        const ArchSegment *ls = nullptr;

        switch ( wslo ) {
            case WSLOH::Longest():
                ls = RoomService::longestSegment(r);
                break;
            case WSLOH::LongestOpposite():
                ls = RoomService::longestSegmentOpposite(r);
                break;
            case WSLOH::SecondLongest():
                ls = RoomService::secondLongestSegment(r);
                break;
            case WSLOH::Shortest():
                ls = RoomService::shortestSegment(r);
                break;
            case WSLOH::SecondShortest():
                ls = RoomService::secondShortestSegment(r);
                break;
            case WSLOH::ExactIndex():
                ls = RoomService::segmentAtIndex(r, _exactIndex);
                break;
            default:
                ls = nullptr;
        }
        return ls;
    }

    void placeAroundInternal( std::shared_ptr<FittedFurniture> dest, FittedFurniture *source, PivotPointPosition where,
                              const V2f& slack, const float _height ) {

        V2f pivotOffset{ V3f::ZERO };
        switch ( where ) {
            case PivotPointPosition::TopLeft:
                pivotOffset = source->widthNormal * ( source->HalfWidth() + dest->HalfWidth() + slack.x() );
                dest->position(source->Center() + pivotOffset);
                dest->move(V3f::UP_AXIS * source->Height());
                break;
            case PivotPointPosition::TopRight:
                pivotOffset = source->widthNormal * ( -source->HalfWidth() - dest->HalfWidth() + slack.x() );
                dest->position(source->Center() + pivotOffset);
                dest->move(V3f::UP_AXIS * source->Height());
                break;
            case PivotPointPosition::TopCenter:
                dest->position(source->Center() + pivotOffset);
                dest->move(V3f::UP_AXIS * source->Height());
                break;
            case PivotPointPosition::LeftCenter:
                pivotOffset = source->widthNormal * -( source->HalfWidth() + dest->HalfWidth() + slack.x() );
                dest->position(source->Center() + pivotOffset);
                break;
            case PivotPointPosition::RightCenter:
                pivotOffset = source->widthNormal * ( source->HalfWidth() + dest->HalfWidth() + slack.x() );
                dest->position(source->Center() + pivotOffset);
                break;
            case PivotPointPosition::BottomCenter:
                pivotOffset = source->depthNormal * ( source->HalfDepth() + dest->HalfDepth() + slack.y() );
                dest->position(source->Center() + pivotOffset);
                break;
            default:
                break;
        }
        dest->rotate(source->Rotation());
        dest->depthNormal = source->depthNormal;
        dest->widthNormal = source->widthNormal;
    }

    void placeAlongWallInternal( std::shared_ptr<FittedFurniture> dest, FittedFurniture *source,
                                 const ArchSegment *ls, WallSegmentCorner preferredCorner, const V2f& slack,
                                 const float _height ) {

        auto ln = preferredCorner == WSC_P2 ? ( ls->p1 - ls->p2 ) : ( ls->p2 - ls->p1 );
        ln = normalize(ln);
        V2f pos2d = ln * ( source->HalfWidth() + dest->HalfWidth() + slack.x() );
        dest->position(source->Center2d() + pos2d); //, _height));
        dest->move(V3f::UP_AXIS * _height);
        dest->rotate(source->Rotation());
        dest->depthNormal = source->depthNormal;
        dest->widthNormal = source->widthNormal;
    }

//    void placeAroundInBetweenInternal(  std::shared_ptr<FittedFurniture> dest, std::shared_ptr<FittedFurniture> source, PivotPointPosition where, const V2f& slack, const float _height ) {
//        switch ( where ) {
//            case PivotPointPosition::TopLeft:
//                dest->xyLocation = source->widthNormal * ( source->HalfWidth() + dest->size.width()*0.5f + slack.x()) +
//                                  source->depthNormal * ( -source->HalfDepth() + dest->size.depth()*0.5f + slack.y());
//                break;
//            case PivotPointPosition::TopRight:
//                dest->xyLocation = source->widthNormal * ( -source->HalfWidth() - dest->size.width()*0.5f + slack.x()) +
//                                  source->depthNormal * ( -source->HalfDepth() + dest->size.depth()*0.5f + slack.y());
//                break;
//            case PivotPointPosition::TopCenter:
//                dest->xyLocation = source->depthNormal * ( -source->HalfDepth() + dest->size.depth()*0.5f + slack.y());
//                break;
//            case PivotPointPosition::LeftCenter:
//                dest->xyLocation = source->widthNormal * -( source->HalfWidth() + dest->size.width()*0.5f + slack.x());
//                break;
//            case PivotPointPosition::RightCenter:
//                dest->xyLocation = source->widthNormal * ( source->HalfWidth() + dest->size.width()*0.5f + slack.x());
//                break;
//            default:
//                break;
//        }
//        dest->move( source->xyLocation;
//        dest->heightOffset = _height;
//        dest->rotation = source->rotation;
//        dest->depthNormal = source->depthNormal;
//        dest->widthNormal = source->widthNormal;
//    }

    bool placeAround( FurnitureRuleParams params ) {
        placeAroundInternal(params.ff, params.source, params.where, params.slack, params.heightOffset);
        bool completed = RS::addFurniture(params);

        // If the around area is not available, try the opposite direction if there's space
        if ( !completed ) {
            if ( auto newwhere = pivotPointPositionMirrorY(params.where); newwhere != PivotPointPosition::Invalid ) {
                placeAroundInternal(params.ff, params.source, newwhere, params.slack, params.heightOffset);
                completed = RS::addFurniture(params);
            }
        }
        return completed;
    }

    bool placeWallAlong( FurnitureRuleParams params ) {
        placeAlongWallInternal(params.ff, params.source, params.ls, params.preferredCorner, params.slack,
                               params.heightOffset);
        return RS::addFurniture(params);
    }

    bool placeInBetween( FurnitureRuleParams params ) {
        placeAroundInternal(params.ff, params.source, params.where, params.slack, params.heightOffset);
        bool completed = RS::addFurniture(params);

        // If the around area is not available, try the opposite direction if there's space
        if ( !completed ) {
            if ( auto newwhere = pivotPointPositionMirrorY(params.where); newwhere != PivotPointPosition::Invalid ) {
                placeAroundInternal(params.ff, params.source, newwhere, params.slack, params.heightOffset);
                completed = RS::addFurniture(params);
            }
        }
        return completed;
    }

    bool placeWallCorner( FurnitureRuleParams params ) {
        if ( !params.ls ) return false;
        if ( checkBitWiseFlag(params.ls->tag, WF_IsDoorPart) ||
             checkBitWiseFlag(params.ls->tag, WF_IsWindowPart) )
            return false;

        V2f cornerPoint = params.preferredCorner == WSC_P1 ? params.ls->p1 : params.ls->p2;
        V2f lpn = normalize(params.ls->middle - cornerPoint);
        V2f offset = params.ls->normal *
                     ( ( params.ff->HalfDepth() ) + skirtingDepth(params.r) + params.slack.y() ) +
                     ( lpn * ( params.ff->HalfWidth() + skirtingDepth(params.r) + params.slack.x() ) );
        params.ff->position(cornerPoint + offset); //, params.heightOffset)
        params.ff->move(V3f::UP_AXIS * params.heightOffset);
        params.ff->rotate(Quaternion{ RoomService::furnitureAngleFromWall(params.ls), V3f::UP_AXIS });
        params.ff->widthNormal = params.ls->crossNormal;
        params.ff->depthNormal = params.ls->normal;

        return RS::addFurniture(params);
    }

    bool placeWallAligned( FurnitureRuleParams params ) {
        auto ls = getWallSegmentFor(params.r, params.wslo, params.exactIndex);
        if ( !ls ) return false;
        if ( checkBitWiseFlag(ls->tag, WF_IsDoorPart) || checkBitWiseFlag(ls->tag, WF_IsWindowPart) ) return false;

        V2f offset = ls->normal * ( ( params.ff->HalfDepth() ) + skirtingDepth(params.r) + params.slackScalar );
        params.ff->position(ls->middle + offset);
        params.ff->move(V3f::UP_AXIS * params.heightOffset);
        params.ff->rotate(Quaternion{ RoomService::furnitureAngleFromWall(ls), V3f::UP_AXIS });
        params.ff->widthNormal = ls->crossNormal;
        params.ff->depthNormal = ls->normal;

        return addFurniture(params);
    }

    bool placeManually( FurnitureRuleParams params ) {
        params.ff->position(params.pos);
        params.ff->move(V3f::UP_AXIS * params.heightOffset);
        params.ff->rotate(params.rot);
        params.ff->widthNormal = params.widthNormal;
        params.ff->depthNormal = params.depthNormal;
        return addFurniture(params);
    }

    bool
    placeDecorations( FloorBSData *f, RoomBSData *r, FittedFurniture *mainF, FurnitureMapStorage& furns,
                      const FurniturePlacementRule& fpd ) {
        bool completed = true;
        if ( fpd.hasDecorations() ) {
            for ( const auto& dec : fpd.getDecorations() ) {
                auto decF = furns.spawn(dec);
                float h1 = checkBitWiseFlag(decF->flags, FittedFurnitureFlags::FF_CanBeHanged) ? middleHeightFromObject(
                        r, mainF, decF.get()) : 0.00f;
                completed &= h1 >= 0.0f;
                if ( h1 >= 0.0f ) {
                    completed &= RS::placeAround(FurnitureRuleParams{ f, r, decF, FRPSource{ mainF }, PPP::TopCenter });
                }
            }
        }
        return completed;
    }

    bool cplaceMainWith2Sides( FloorBSData *f, RoomBSData *r, FurnitureMapStorage& furns,
                               const FurniturePlacementRule& fpd ) {
        FT main = fpd.getBase(0);
        WSLO refWall = fpd.getWallSegmentId().type;
        bool completed = true;

        auto mainF = furns.spawn(main);
        completed = RS::placeWallAligned(FurnitureRuleParams{ f, r, mainF, FRPWSLO{ refWall } });
        if ( completed ) {
            if ( fpd.hasBase(1) ) {
                completed &= RS::placeAround(
                        FurnitureRuleParams{ f, r, furns.spawn(fpd.getBase(1)), FRPSource{ mainF.get() },
                                             PPP::TopLeft });
            }
            if ( fpd.hasBase(2) ) {
                completed &= RS::placeAround(
                        FurnitureRuleParams{ f, r, furns.spawn(fpd.getBase(2)), FRPSource{ mainF.get() },
                                             PPP::TopRight });
            }
            completed &= RS::placeDecorations(f, r, mainF.get(), furns, fpd);
        }
        return completed;
    }

    bool cplaceSetAlignedMiddle( FloorBSData *f, RoomBSData *r, FurnitureMapStorage& furns,
                                 const FurniturePlacementRule& fpd ) {
        FT main = fpd.getBase(0);
        WSLO refWall = fpd.getWallSegmentId().type;
        bool completed = true;
        auto mainF = furns.spawn(main);
        completed = RS::placeWallAligned(FurnitureRuleParams{ f, r, mainF, FRPWSLO{ refWall } });
        // If it has more than 1 furniture then place them in front of the main furniture, like a coffee table in front of a sofa
        if ( fpd.hasBase(1) ) {
            completed &= RS::placeAround(
                    FurnitureRuleParams{ f, r, furns.spawn(fpd.getBase(1)), FRPSource{ mainF.get() }, PPP::BottomCenter,
                                         FRPSlack{ fpd.getSlack() } });
        }
        completed &= RS::placeDecorations(f, r, mainF.get(), furns, fpd);
        return completed;
    }

    bool
    cplaceSetBestFit( FloorBSData *f, RoomBSData *r, FurnitureMapStorage& furns, const FurniturePlacementRule& fpd ) {
        FT main = fpd.getBase(0);
        WSLO refWall = fpd.getWallSegmentId().type;
        bool completed = true;
        auto mainF = furns.spawn(main);
        for ( uint32_t rwIndex = 0UL; rwIndex < r->mWallSegmentsSorted.size(); rwIndex++ ) {
            completed = RS::placeWallAligned(
                    FurnitureRuleParams{ f, r, mainF, FRPWSLO{ refWall }, FRPSlackScalar{ fpd.getSlack(0).x() },
                                         rwIndex });
            if ( completed ) break;
        }
        // If it has more than 1 furniture then place them in front of the main furniture, like a coffee table in front of a sofa
        if ( fpd.hasBase(1) ) {
            completed &= RS::placeAround(
                    FurnitureRuleParams{ f, r, furns.spawn(fpd.getBase(1)), FRPSource{ mainF.get() }, PPP::BottomCenter,
                                         FRPSlack{ fpd.getSlack() } });
        }
        completed &= RS::placeDecorations(f, r, mainF.get(), furns, fpd);
        return completed;
    }

    bool cplaceSetBestFitHanged( FloorBSData *f, RoomBSData *r, FurnitureMapStorage& furns,
                                 const FurniturePlacementRule& fpd ) {
        FT main = fpd.getBase(0);
        WSLO refWall = fpd.getWallSegmentId().type;
        bool completed = true;
        auto mainF = furns.spawn(main);
        for ( uint32_t rwIndex = 0UL; rwIndex < r->mWallSegmentsSorted.size(); rwIndex++ ) {
            completed = RS::placeWallAligned(
                    FurnitureRuleParams{ f, r, mainF, FRPWSLO{ refWall }, FRPSlackScalar{ fpd.getSlack(0).x() },
                                         rwIndex });
            if ( completed ) break;
        }
        return completed;
    }

    bool cplaceSetAlignedAtCorner( FloorBSData *f, RoomBSData *r, FurnitureMapStorage& furns,
                                   const FurniturePlacementRule& fpd ) {
        const std::vector<FT>& fset = fpd.getBases();
        WSLO refWall = fpd.getWallSegmentId().type;

        bool completed = true;

        auto ls = getWallSegmentFor(r, refWall, fpd.getWallSegmentId().index);
        if ( !ls ) return false;
        if ( checkBitWiseFlag(ls->tag, WF_IsDoorPart) || checkBitWiseFlag(ls->tag, WF_IsWindowPart) ) return false;
        auto prevFurn = furns.spawn(fset.front());
        completed = RS::placeWallCorner(FurnitureRuleParams{ f, r, prevFurn, ls, FRPSlack{ fpd.getSlack().xy() },
                                                             FRPWallSegmentCorner{ fpd.getPreferredCorner() } });
        if ( !completed ) return false;

        for ( size_t t = 1; t < fset.size(); t++ ) {
            auto currFurn = furns.spawn(fset[t]);
            completed &= RS::placeWallAlong(FurnitureRuleParams{ f, r, currFurn, FRPSource{ prevFurn.get() }, ls,
                                                                 FRPWallSegmentCorner{ fpd.getPreferredCorner() },
                                                                 FRPSlack{ fpd.getSlack(t).xy() } });
            if ( !completed ) break;
            prevFurn = currFurn;
        }
        return completed;
    }

    bool cplaceCornerWithDec( FloorBSData *f, RoomBSData *r, FurnitureMapStorage& furns,
                              const FurniturePlacementRule& fpd ) {
        FT main = fpd.getBase(0);
        WSLO refWall = fpd.getWallSegmentId().type;

        bool completed = true;
        auto mainF = furns.spawn(main);
        auto ls = getWallSegmentFor(r, refWall, fpd.getWallSegmentId().index);
        completed &= RS::placeWallCorner(FurnitureRuleParams{ f, r, mainF, ls, FRPSlack{ fpd.getSlack().xy() },
                                                              FRPWallSegmentCorner{ WSC_P2 } });
        if ( !completed ) return false;

        if ( fpd.hasDecorations() ) {
            const std::vector<FT>& decorations = fpd.getDecorations();
            auto dec = furns.spawn(decorations.front());
            float h1 = middleHeightFromObject(r, mainF.get(), dec.get());
            completed &= h1 >= 0.0f;
            if ( h1 >= 0.0f ) {
                float h = mainF->Height() + h1;
                completed &= RS::placeWallCorner(FurnitureRuleParams{ f, r, dec, ls, FRPSlack{ fpd.getSlack().xy() },
                                                                      FRPWallSegmentCorner{ WSC_P2 }, h });
            }
        } else {
            completed = false;
        }
        return completed;
    }

    bool cplacedFirstAvailableCorner( FloorBSData *f, RoomBSData *r, FurnitureMapStorage& furns,
                                      const FurniturePlacementRule& fpd ) {
        const std::vector<FT>& fset = fpd.getBases();
        bool completed = true;

        for ( const auto& wss : r->mWallSegments ) {
            for ( auto i = 0u; i < wss.size(); ++i ) {
                auto ls = &wss[i];
                if ( checkBitWiseFlag(ls->tag, WF_IsDoorPart) || checkBitWiseFlag(ls->tag, WF_IsWindowPart) ) continue;
                const ArchSegment *ls2 = &wss[cai(i + 1, wss.size())];
                auto p1 = ls->p1;
                auto p2 = ls->p2;
                auto p3 = ls2->p2;
                if ( !isCollinear(p1, p2, p3) && detectWindingOrder(p1, p2, p3) == WindingOrder::CCW ) {
                    auto prevFurn = furns.spawn(fset.front());
                    completed = RS::placeWallCorner(
                            FurnitureRuleParams{ f, r, prevFurn, ls2, FRPSlack{ fpd.getSlack().xy() },
                                                 FRPWallSegmentCorner{ WSC_P1 } });
                    if ( completed ) break;
                }
            }
        }

        return completed;
    }

    bool
    cplaceMiddleOfRoom( FloorBSData *f, RoomBSData *r, FurnitureMapStorage& furns, const FurniturePlacementRule& fpd ) {
        auto center = RS::maxEnclsingBoundingBoxCenter(r);
        auto ff = furns.spawn(fpd.getBase(0));
        return placeManually(FurnitureRuleParams{ f, r, ff, center, FRPFurnitureRuleFlags{ fpd.getFlags() } });
    }

    bool runRuleScript( FloorBSData *f, RoomBSData *r, FurnitureMapStorage& furns, const FurnitureRuleScript& fs ) {
        return fs.execute(f, r, furns, RS::functionRules);
    }

    bool checkBBoxInsideRoom( const RoomBSData *r, const Rect2f& bbox ) {
        ClipperLib::Clipper c;
        ClipperLib::Path pathSubject = ClipperLib::V2fToPath(bbox.points());
        V2f subjectPathSize = ClipperLib::pathSize(pathSubject);
        c.AddPath(pathSubject, ClipperLib::ptSubject, true);
        c.AddPath(ClipperLib::V2fToPath(r->mPerimeterSegments), ClipperLib::ptClip, true);
        ClipperLib::Paths solution;
        c.Execute(ClipperLib::ctIntersection, solution, ClipperLib::pftNonZero, ClipperLib::pftNonZero);

        if ( ( !solution.empty() && solution.size() == 1 && solution[0].size() == 4 ) ) {
            V2f solutionPathSize = ClipperLib::pathSize(solution[0]);
            if ( isVerySimilar(solutionPathSize, subjectPathSize) ) {
                return true;
            }
        }
        return false;
    }

    void addFurniture( RoomBSData *r, std::shared_ptr<FittedFurniture> ff ) {
        r->mFittedFurniture.push_back(ff);
    }

    bool addFurniture( FurnitureRuleParams& params ) {
        // If it has a source furniture add it to the dependant list, IE a table (params.source) and a glass (params.ff)
        if ( params.source ) {
            params.source->dependantHashList.emplace_back(params.ff->hash);
        }

        Rect2f subjectBBox = params.ff->BBox();
        if ( !checkBitWiseFlag(params.flags, FurnitureRuleFlags::IgnoreDoorClipping) ) {
            for ( const auto& door : params.f->doors ) {
                auto dbbox = door->BBox().squaredBothSides();
                if ( dbbox.contains(subjectBBox) ||
                     subjectBBox.intersect(dbbox, 0.001f, EdgeTouchingIsIntersecting::Yes) ) {
                    return false;
                }
            }
        }

        if ( checkBitWiseFlag(params.flags, FurnitureRuleFlags::DoNotClipAgainstRoom) ) {
            addFurniture(params.r, params.ff);
            return true;
        }

        if ( checkBBoxInsideRoom(params.r, params.ff->BBox()) ) {
            if ( !params.ff->checkIf(FittedFurnitureFlags::FF_CanOverlap) &&
                 !checkBitWiseFlag(params.flags, FurnitureRuleFlags::ForceCanOverlap) ) {
                for ( const auto& furn : params.r->mFittedFurniture ) {
                    if ( params.ff->PositionY() != furn->PositionY() ) {
                        if ( params.ff->PositionY() > furn->PositionY() + furn->Height() ||
                             params.ff->PositionY() + params.ff->Height() < furn->PositionY() ) {
                            continue;
                        }
                    }
                    if ( params.ff->BBox().intersect(furn->BBox(), 0.001f) ) {
                        return false;
                    }
                }
            }
            addFurniture(params.r, params.ff);
            return true;
        }

        return false;
    }

    void clearFurniture( RoomBSData *r ) {
        r->mFittedFurniture.clear();
    }

    void furnishHallway( FloorBSData *f, RoomBSData *r, FurnitureMapStorage& furns ) {
        FurnitureRuleScript ruleScript;
        V3f cornerSlack{ 0.05f, 0.0f, 0.0f };
        ruleScript.clear();
        ruleScript.addRule(FurniturePlacementRule{ FurnitureRuleIndex(RS::FRBestFitHanged),
                                                   FurnitureRefs{ { FTH::FT_Picture } } });
        RS::runRuleScript(f, r, furns, ruleScript);
    }

    void furnishBedroom( FloorBSData *f, RoomBSData *r, FurnitureMapStorage& furns ) {
        FurnitureRuleScript ruleScript;
        V3f cornerSlack{ 0.05f, 0.0f, 0.0f };
        ruleScript.clear();
        ruleScript.addRule(FurniturePlacementRule{ FurnitureRuleIndex(RS::MainWith2Sides),
                                                   FurnitureRefs{ { FTH::FT_Bed, FTH::FT_Bedside, FTH::FT_Bedside },
                                                                  { FTH::FT_Picture } }, WSLOH::SecondLongest() });
        ruleScript.addRule(FurniturePlacementRule{ FurnitureRuleIndex(RS::CornerWithDec),
                                                   FurnitureRefs{ { FTH::FT_Bedside },
                                                                  { FTH::FT_Picture } }, WSLOH::Longest(),
                                                   FurnitureSlacks{ cornerSlack } });
        ruleScript.addRule(FurniturePlacementRule{ FurnitureRuleIndex(RS::SetAlignedAtCorner),
                                                   FurnitureRefs{ { FTH::FT_Shelf, FTH::FT_Shelf, FTH::FT_Sofa } },
                                                   WSLOH::Longest(),
                                                   FurnitureSlacks{ cornerSlack, V3f::ZERO, cornerSlack } });
        ruleScript.addRule(FurniturePlacementRule{ FurnitureRuleIndex(RS::SetAlignedAtCorner),
                                                   FurnitureRefs{ { FTH::FT_Wardrobe, FTH::FT_Wardrobe } },
                                                   WSLOH::LongestOpposite(), WSC_P2 });
        ruleScript.addRule(FurniturePlacementRule{ FurnitureRuleIndex(RS::SetAlignedAtCorner),
                                                   FurnitureRefs{ { FTH::FT_Shelf, FTH::FT_Wardrobe, FTH::FT_Armchair } },
                                                   WallSegmentIdentifier{ WSLOH::ExactIndex(), 3 },
                                                   FurnitureSlacks{ V3f::ZERO, V3f::ZERO, cornerSlack * 0.5f },
                                                   WSC_P2 });
        ruleScript.addRule(FurniturePlacementRule{ FurnitureRuleIndex(RS::SetAlignedAtCorner),
                                                   FurnitureRefs{ { FTH::FT_Armchair } },
                                                   WallSegmentIdentifier{ WSLOH::ExactIndex(), 4 },
                                                   WSC_P2 });
        ruleScript.addRule(FurniturePlacementRule{ FurnitureRuleIndex(RS::SetAlignedAtCorner),
                                                   FurnitureRefs{ { FTH::FT_Armchair } },
                                                   WallSegmentIdentifier{ WSLOH::ExactIndex(), 4 },
                                                   WSC_P1 });
        ruleScript.addRule(
                FurniturePlacementRule{ FurnitureRuleIndex(RS::MiddleOfRoom), FurnitureRefs{ { FTH::FT_Carpet } } });
        RS::runRuleScript(f, r, furns, ruleScript);
    }

    void furnishLiving( FloorBSData *f, RoomBSData *r, FurnitureMapStorage& furns ) {
        FurnitureRuleScript ruleScript;
        V3f inFrontOfSlack{ 0.0f, 0.2f, 0.0f };
        ruleScript.addRule(FurniturePlacementRule{ FurnitureRuleIndex(RS::SetAlignedMiddle),
                                                   FurnitureRefs{ { FTH::FT_Sofa, FTH::FT_CoffeeTable } },
                                                   WallSegmentIdentifier{ WSLOH::Longest() },
                                                   FurnitureSlacks{ inFrontOfSlack }
        });
        ruleScript.addRule(FurniturePlacementRule{ FurnitureRuleIndex(RS::SetAlignedMiddle),
                                                   FurnitureRefs{ { FTH::FT_SideBoard },
                                                                  { FTH::FT_TVWithStand } },
                                                   WallSegmentIdentifier{ WSLOH::LongestOpposite() }
        });
        ruleScript.addRule(FurniturePlacementRule{ FurnitureRuleIndex(RS::CornerWithDec),
                                                   FurnitureRefs{ { FTH::FT_Plant },
                                                                  { FTH::FT_Picture } },
                                                   WallSegmentIdentifier{ WSLOH::Longest() }
        });
        ruleScript.addRule(
                FurniturePlacementRule{ FurnitureRuleIndex(RS::MiddleOfRoom), FurnitureRefs{ { FTH::FT_Carpet } } });
        RS::runRuleScript(f, r, furns, ruleScript);
    }

    void furnishDining( FloorBSData *f, RoomBSData *r, FurnitureMapStorage& furns ) {
        FurnitureRuleScript ruleScript;
        V3f tableSlack{ 0.5f, 0.5f, 0.0f };
        ruleScript.addRule(FurniturePlacementRule{ FurnitureRuleIndex(RS::FRBestFit),
                                                   FurnitureRefs{ { FTH::FT_DiningTable } },
                                                   WallSegmentIdentifier{ WSLOH::ExactIndex() },
                                                   FurnitureSlacks{ tableSlack }
        });
        RS::runRuleScript(f, r, furns, ruleScript);
    }

    void furnishBathroom( FloorBSData *f, RoomBSData *r, FurnitureMapStorage& furns ) {
        FurnitureRuleScript ruleScript;

        ruleScript.addRule(FurniturePlacementRule{ FurnitureRuleIndex(RS::FRFirstAvailableCorner),
                                                   FurnitureRefs{ { FTH::FT_Shower } }
        });
        ruleScript.addRule(FurniturePlacementRule{ FurnitureRuleIndex(RS::FRBestFit),
                                                   FurnitureRefs{ { FTH::FT_Toilet } },
                                                   WallSegmentIdentifier{ WSLOH::ExactIndex() }
        });
        ruleScript.addRule(FurniturePlacementRule{ FurnitureRuleIndex(RS::FRBestFit),
                                                   FurnitureRefs{ { FTH::FT_BathroomSink } },
                                                   WallSegmentIdentifier{ WSLOH::ExactIndex() }
        });
        ruleScript.addRule(FurniturePlacementRule{ FurnitureRuleIndex(RS::FRBestFit),
                                                   FurnitureRefs{ { FTH::FT_BathroomTowerRadiator } },
                                                   WallSegmentIdentifier{ WSLOH::ExactIndex() }
        });
        RS::runRuleScript(f, r, furns, ruleScript);
    }

    void furnishKitchen( FloorBSData *f, RoomBSData *r, FurnitureMapStorage& furns ) {
        KitchenRoomService::createKitchen(f, r, furns);
    }

    void furnish( FloorBSData *f, RoomBSData *r, FurnitureMapStorage& furns ) {

        RoomService::clearFurniture(r);
        KitchenRoomService::clear(r);
        std::vector<ASTypeT> prioritySorted[2];
        for ( const auto rtype : r->roomTypes ) {
            switch ( rtype ) {
                case ASType::DiningRoom:
                    prioritySorted[1].emplace_back(rtype);
                    break;
                default:
                    prioritySorted[0].emplace_back(rtype);
                    break;
            }
        }

        for ( const auto& prioritySort : prioritySorted ) {
            for ( const auto rtype : prioritySort ) {
                switch ( rtype ) {
                    case ASType::GenericRoom:
                        break;
                    case ASType::Hallway:
                        furnishHallway(f, r, furns);
                        break;
                    case ASType::Bathroom:
                        furnishBathroom(f, r, furns);
                        break;
                    case ASType::Kitchen:
                        furnishKitchen(f, r, furns);
                        break;
                    case ASType::BedroomDouble:
                    case ASType::BedroomMaster:
                    case ASType::BedroomSingle:
                        furnishBedroom(f, r, furns);
                        break;
                    case ASType::LivingRoom:
                    case ASType::Studio:
                        furnishLiving(f, r, furns);
                        break;
                    case ASType::DiningRoom:
                        furnishDining(f, r, furns);
                        break;
                    default:
                        break;
                }
            }
        }
    }
}

std::shared_ptr<FittedFurniture> FurnitureMapStorage::spawn( FT _ft ) {
    auto frange = index.equal_range(_ft);
    storage.emplace(_ft, frange.first->second);

    auto range = storage.equal_range(_ft);
    auto i = range.first;
    auto ret = range.first;
    for ( ; i != range.second; ++i ) {
        ret = i;
    }
    return EntityFactory::cloneHashed(ret->second);
}

void initializeDefaultFurnituresFlags( FT _ft, FittedFurniture& _ff ) {
    if ( _ft == FTH::FT_Carpet || _ft == FTH::FT_TVWithStand ) {
        orBitWiseFlag(_ff.flags, FittedFurnitureFlags::FF_CanOverlap);
    }
    if ( _ft == FTH::FT_Picture ) {
        orBitWiseFlag(_ff.flags, FittedFurnitureFlags::FF_CanBeHanged | FittedFurnitureFlags::FF_isDecoration);
    }
}

void FurnitureMapStorage::addIndex( FT _ft, FittedFurniture& _ff ) {
    initializeDefaultFurnituresFlags(_ft, _ff);
    index.emplace(_ft, _ff);
}

void FurnitureMapStorage::addIndex( const FurnitureSet& f ) {
    auto ff = FittedFurniture{ { f.name, f.bbox3d }, FurnitureTypeHandler::name(FT(f.ftype)), f.symbol };
    this->addIndex(FT(f.ftype), ff);
}

FurniturePlacementRule FurniturePlacementRule::randomGeneration() {
    FurniturePlacementRule ret{ unitRandI(static_cast<int>(RS::functionRules.size()) - 1) };

    ret.addBase(FTH::at(unitRandI(FTH::count() - 1)));
    ret.addBase(FTH::at(unitRandI(FTH::count() - 1)));
    ret.addBase(FTH::at(unitRandI(FTH::count() - 1)));
    ret.setWallSegmentId({ WSLOH::at(unitRandI(WSLOH::count() - 1)), 0 });
    ret.setPreferredCorner(WallSegmentCorner(unitRandI(2)));

    return ret;
}

void RoomServiceFurniture::addDefaultFurnitureSet( const std::string& _name ) {
    std::string brimnes_bed = "Brimnes";
    std::string lauter_selije = "lauter_selije";
    std::string hemnes_shelf = "hemnes_shelf";
    std::string hemnes_drawer = "hemnes_drawer";
    std::string sofahoop = "sofa,hoop";
    std::string soderhamn = "soderhamn";
    std::string carpet_flottebo = "carpet";
    std::string Strandmon = "strandmon";
    std::string pictures_set_3 = "pictures2";
    std::string coffeeTable = "noguchi";
    std::string diningTable = "ktc,table,round";
    std::string sideBoard = "gallotti,radice,tama";
    std::string tv = "smart,tv,with,stand";
    std::string plant1 = "plant,spider";
    std::string coffeeMachine = "coffee,cups";

    std::string toilet = "urby,toilet";
    std::string shower = "shower,tresse";
    std::string bathTub = "bath,tub,standard";
    std::string bathroomSink = "urby,sink";
    std::string bathroomTowerRadiator = "bathroom,tower,radiator";

    std::string sinkModel = "ktc,sink,double,chrome";
    std::string ovenPanelModel = "ktc,oven,flat";
    std::string microwaveModel = "ktc,microwave";
    std::string cooktopModel = "ktc,cooktop";
    std::string fridgeModel = "ktc,fridge,single";
    std::string extractorHoodModel = "ktc,extractor,hood";
    std::string drawersHandleModel = "ktc,handle,long,contemporary";

    std::string queenBedIcon = "fia,queen,icon";
    std::string bedSideIcon = "fia,bedside";
    std::string sofaIcon = "fia,sofa,3seaters";
    std::string wardrobeIcon = "fia,wardrobe";
    std::string shelfIcon = "fia,shelf";
    std::string armchairIcon = "fia,armchair";
    std::string coffeeTableIcon = "fia,coffee,table";
    std::string toiletIcon = "fia,toilet";
    std::string bathroomSinkIcon = "fia,double,sink";
    std::string neutralSquareIcon = "neutral,ff,square";

    FurnitureSetContainer furnitureSet{};
    furnitureSet.name = _name;
    furnitureSet.set.emplace_back(FTH::FT_Bed, brimnes_bed, AABB::MINVALID(), queenBedIcon);
    furnitureSet.set.emplace_back(FTH::FT_Bedside, lauter_selije, AABB::MINVALID(), bedSideIcon);
    furnitureSet.set.emplace_back(FTH::FT_Shelf, hemnes_shelf, AABB::MINVALID(), shelfIcon);
    furnitureSet.set.emplace_back(FTH::FT_Wardrobe, hemnes_drawer, AABB::MINVALID(), wardrobeIcon);
    furnitureSet.set.emplace_back(FTH::FT_Drawer, hemnes_drawer, AABB::MINVALID(), wardrobeIcon);
    furnitureSet.set.emplace_back(FTH::FT_Sofa, soderhamn, AABB::MINVALID(), sofaIcon);
    furnitureSet.set.emplace_back(FTH::FT_Sofa, sofahoop, AABB::MINVALID(), sofaIcon);
    furnitureSet.set.emplace_back(FTH::FT_Picture, pictures_set_3, AABB::MINVALID(), neutralSquareIcon);
    furnitureSet.set.emplace_back(FTH::FT_Carpet, carpet_flottebo, AABB::MINVALID(), neutralSquareIcon);
    furnitureSet.set.emplace_back(FTH::FT_Armchair, Strandmon, AABB::MINVALID(), armchairIcon);
    furnitureSet.set.emplace_back(FTH::FT_CoffeeTable, coffeeTable, AABB::MINVALID(), coffeeTableIcon);
    furnitureSet.set.emplace_back(FTH::FT_CoffeeMachine, coffeeMachine, AABB::MINVALID(), coffeeTableIcon);
    furnitureSet.set.emplace_back(FTH::FT_DiningTable, diningTable, AABB::MINVALID(), coffeeTableIcon);
    furnitureSet.set.emplace_back(FTH::FT_SideBoard, sideBoard, AABB::MINVALID(), neutralSquareIcon);
    furnitureSet.set.emplace_back(FTH::FT_TVWithStand, tv, AABB::MINVALID(), neutralSquareIcon);
    furnitureSet.set.emplace_back(FTH::FT_Plant, plant1, AABB::MINVALID(), neutralSquareIcon);
    furnitureSet.set.emplace_back(FTH::FT_Toilet, toilet, AABB::MINVALID(), toiletIcon);
    furnitureSet.set.emplace_back(FTH::FT_Shower, shower, AABB::MINVALID(), neutralSquareIcon);
    furnitureSet.set.emplace_back(FTH::FT_BathTub, bathTub, AABB::MINVALID(), neutralSquareIcon);
    furnitureSet.set.emplace_back(FTH::FT_BathroomSink, bathroomSink, AABB::MINVALID(), bathroomSinkIcon);
    furnitureSet.set.emplace_back(FTH::FT_BathroomTowerRadiator, bathroomTowerRadiator, AABB::MINVALID(),
                                  neutralSquareIcon);


    furnitureSet.set.emplace_back(FTH::FT_Sink, sinkModel, AABB::MINVALID(), neutralSquareIcon);
    furnitureSet.set.emplace_back(FTH::FT_OvenPanel, ovenPanelModel, AABB::MINVALID(), neutralSquareIcon);
    furnitureSet.set.emplace_back(FTH::FT_Microwave, microwaveModel, AABB::MINVALID(), neutralSquareIcon);
    furnitureSet.set.emplace_back(FTH::FT_Cooktop, cooktopModel, AABB::MINVALID(), neutralSquareIcon);
    furnitureSet.set.emplace_back(FTH::FT_Fridge, fridgeModel, AABB::MINVALID(), neutralSquareIcon);
    furnitureSet.set.emplace_back(FTH::FT_ExtractorHood, extractorHoodModel, AABB::MINVALID(), neutralSquareIcon);
    furnitureSet.set.emplace_back(FTH::FT_DrawersHandle, drawersHandleModel, AABB::MINVALID(), neutralSquareIcon);

    Http::post(Url{ "/furnitureset" }, furnitureSet.serialize(), []( HttpResponeParams& res ) {
        LOGRS(res.BufferString())
    });
}

void
dependantFurnitureCallback( RoomBSData *r, FittedFurniture *ff, std::function<void( FittedFurniture * )> callback ) {
    for ( auto& hash : ff->dependantHashList ) {
        auto dependantFurn = RoomService::findFurniture(r, hash);
        if ( dependantFurn ) {
            callback(dependantFurn);
        }
    }
}

void RoomServiceFurniture::rotateFurniture( FittedFurniture *ff, const Quaternion& rot ) {
    Quaternion newRot = ff->Rotation() * rot;
    newRot.normalise();
    ff->rotate(newRot);
}

void rotateFurniture( FittedFurniture *ff, const Quaternion& rot, SceneGraph& sg ) {
    RoomServiceFurniture::rotateFurniture(ff, rot);
    auto node = sg.Nodes().find(ff->linkedUUID);
    if ( node->second ) {
        node->second->updateTransform(ff->Rotation());
    }
}

void
RoomServiceFurniture::rotateFurniture( RoomBSData *r, FittedFurniture *ff, const Quaternion& rot, SceneGraph& sg ) {
    rotateFurniture(ff, rot, sg);
    dependantFurnitureCallback(r, ff, [rot, &sg]( FittedFurniture *_ff ) {
        rotateFurniture(_ff, rot, sg);
    });
}

void RoomServiceFurniture::moveFurniture( FittedFurniture *ff, const V3f& off ) {
    ff->move(off);
}

void moveFurniture( FittedFurniture *ff, const V3f& off, SceneGraph& sg ) {
    RoomServiceFurniture::moveFurniture(ff, off);
    auto node = sg.Nodes().find(ff->linkedUUID);
    if ( node->second ) {
        node->second->move(off);
    }
}

void RoomServiceFurniture::moveFurniture( RoomBSData *r, FittedFurniture *ff, const V3f& off, SceneGraph& sg ) {
    moveFurniture(ff, off, sg);
    dependantFurnitureCallback(r, ff, [off, &sg]( FittedFurniture *_ff ) {
        moveFurniture(_ff, off, sg);
    });
}

void removeFurnitureInternal( RoomBSData *r, FittedFurniture *ff, SceneGraph& sg ) {
    sg.removeNode(sg.getNode(ff->linkedUUID));

    for ( auto f = r->mFittedFurniture.begin(); f < r->mFittedFurniture.end(); ++f ) {
        if ( ( *f )->hash == ff->hash ) {
            r->mFittedFurniture.erase(f);
            break;
        }
    }
}

void RoomServiceFurniture::removeFurniture( RoomBSData *r, FittedFurniture *ff, SceneGraph& sg ) {
    dependantFurnitureCallback(r, ff, [r, &sg]( FittedFurniture *_ff ) {
        removeFurnitureInternal(r, _ff, sg);
    });
    removeFurnitureInternal(r, ff, sg);
}
