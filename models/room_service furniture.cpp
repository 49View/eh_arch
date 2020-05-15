//
//  room_service.cpp
//  sixthview
//
//  Created by Dado on 16/7/2017.
//
//

#include "room_service.hpp"

#include <poly/scene_graph.h>
#include <core/math/triangulator.hpp>
#include <core/service_factory.h>

#include "floor_service.hpp"
#include "arch_segment_service.hpp"
#include "room_service_furniture.hpp"

bool FurnitureRuleScript::execute( FloorBSData* f, RoomBSData *r, FurnitureMapStorage &furns,
                                   const FurnitureRuleFunctionContainer &funcRules ) const {
    bool completed = true;
    for ( const auto &rule : rules ) {
        if ( rule.hasSomethingToDo()) {
            completed &= funcRules[rule.getRuleFunctionIndex()]( f, r, furns, rule );
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
        return atan2( ls->normal.y(), ls->normal.x()) - M_PI_2;
    }

    float furnitureAngleFromNormal( const Vector2f &normal ) {
        return atan2( normal.y(), normal.x()) - M_PI_2;
    }

    Vector2f furnitureNormalFromAngle( const float angle ) {
        return { cosf( angle + M_PI_2 ), sinf( angle + M_PI_2 ) };
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

    const ArchSegment *segmentAtIndex( const RoomBSData *r, uint32_t _index ) {
        if ( r->mWallSegmentsSorted.size() <= _index ) return nullptr;
        return &r->mWallSegmentsSorted[_index];
    }

    void calcLongestWall( RoomBSData *r ) {
        float maxLength = 0.0f;

        r->mWallSegmentsSorted.clear();
        for ( auto &mw : r->mWallSegments ) {
            r->mWallSegmentsSorted.insert( std::end( r->mWallSegmentsSorted ), std::begin( mw ), std::end( mw ));
        }

        std::sort( r->mWallSegmentsSorted.begin(), r->mWallSegmentsSorted.end(),
                   []( const ArchSegment &a, const ArchSegment &b ) -> bool {
                       return ArchSegmentService::length( a ) > ArchSegmentService::length( b );
                   } );

        for ( auto t = 0u; t < r->mWallSegmentsSorted.size(); t++ ) {
            float wlength = ArchSegmentService::length( r->mWallSegmentsSorted[t] );
            if ( wlength > maxLength ) {
                r->mLongestWall = t;
                maxLength = wlength;
            }
        }

        const ArchSegment *ls = longestSegment( r );
        Vector2f i{ V2f::ZERO };
        r->mLongestWallOpposite = -1;
        // Find wall opposite
        for ( auto t = 0u; t < r->mWallSegmentsSorted.size(); t++ ) {
            if (( r->mWallSegmentsSorted[t].tag & WallFlags::WF_IsDoorPart ) > 0 ||
                ( r->mWallSegmentsSorted[t].tag & WallFlags::WF_IsWindowPart ) > 0 ) {
                continue;
            }
            Vector2f normalOpposite = r->mWallSegmentsSorted[t].normal;
            if ( isScalarEqual( dot( normalOpposite, ls->normal ), -1.0f )) {
                if ( intersection( r->mWallSegmentsSorted[t].p1, r->mWallSegmentsSorted[t].p2, ls->middle,
                                   ls->middle + ( ls->normal * 1000.0f ), i )) {
                    r->mLongestWallOpposite = t;
                    r->mLongestWallOppositePoint = i;
                    break;
                }
            }
        }
    }

    ArchSegment *longestSegmentCornerP1( RoomBSData *r ) {
        roomTypeIndex rti = sortedSegmentToPairIndex( r, r->mLongestWall );
        return &r->mWallSegments[rti.first][getCircularArrayIndex( rti.second - 1,
                                                                   static_cast<int32_t>( r->mWallSegments[rti.first].size()))];
    }

    ArchSegment *longestSegmentCornerP2( RoomBSData *r ) {
        roomTypeIndex rti = sortedSegmentToPairIndex( r, r->mLongestWall );
        return &r->mWallSegments[rti.first][getCircularArrayIndex( rti.second + 1,
                                                                   static_cast<int32_t>( r->mWallSegments[rti.first].size()))];
    }

    ArchSegment *longestSegmentOpposite( RoomBSData *r ) {
        if ( r->mLongestWallOpposite < 0 ) {
            return nullptr;
        }
        return &r->mWallSegmentsSorted[r->mLongestWallOpposite];
    }

    float middleHeightFromObject( RoomBSData *r, FittedFurniture &base, FittedFurniture &dec ) {
        return (( r->height - r->mBBoxCoving.height() * 0.01f - base.size.y() - dec.size.y()) / 2.0f );
    }

    const ArchSegment *getWallSegmentFor( RoomBSData *r, const WSLO wslo, uint32_t _exactIndex ) {
        const ArchSegment *ls = nullptr;

        switch ( wslo ) {
            case WSLOH::Longest():
                ls = RoomService::longestSegment( r );
                break;
            case WSLOH::LongestOpposite():
                ls = RoomService::longestSegmentOpposite( r );
                break;
            case WSLOH::SecondLongest():
                ls = RoomService::secondLongestSegment( r );
                break;
            case WSLOH::Shortest():
                ls = RoomService::shortestSegment( r );
                break;
            case WSLOH::SecondShortest():
                ls = RoomService::secondShortestSegment( r );
                break;
            case WSLOH::ExactIndex():
                ls = RoomService::segmentAtIndex( r, _exactIndex );
                break;
            default:
                ls = nullptr;
        }
        return ls;
    }

    void placeAroundInternal( FittedFurniture &dest, const FittedFurniture &source, PivotPointPosition where,
                              const V2f &slack, const float _height ) {
        switch ( where ) {
            case PivotPointPosition::TopLeft:
                dest.xyLocation =
                        source.widthNormal * ( source.size.width() * 0.5f + dest.size.width() * 0.5f + slack.x()) +
                        source.depthNormal * ( -source.size.depth() * 0.5f + dest.size.depth() * 0.5f + slack.y());
                break;
            case PivotPointPosition::TopRight:
                dest.xyLocation =
                        source.widthNormal * ( -source.size.width() * 0.5f - dest.size.width() * 0.5f + slack.x()) +
                        source.depthNormal * ( -source.size.depth() * 0.5f + dest.size.depth() * 0.5f + slack.y());
                break;
            case PivotPointPosition::TopCenter:
                dest.xyLocation =
                        source.depthNormal * ( -source.size.depth() * 0.5f + dest.size.depth() * 0.5f + slack.y());
                break;
            case PivotPointPosition::LeftCenter:
                dest.xyLocation =
                        source.widthNormal * -( source.size.width() * 0.5f + dest.size.width() * 0.5f + slack.x());
                break;
            case PivotPointPosition::RightCenter:
                dest.xyLocation =
                        source.widthNormal * ( source.size.width() * 0.5f + dest.size.width() * 0.5f + slack.x());
                break;
            case PivotPointPosition::BottomCenter:
                dest.xyLocation =
                        source.depthNormal * ( source.size.depth() * 0.5f + dest.size.depth() * 0.5f + slack.y());
                break;
            default:
                break;
        }
        dest.xyLocation += source.xyLocation;
        dest.heightOffset = _height;
        dest.rotation = source.rotation;
        dest.depthNormal = source.depthNormal;
        dest.widthNormal = source.widthNormal;
    }

    void placeAlongWallInternal( FittedFurniture &dest, const FittedFurniture &source,
                                 const ArchSegment *ls, WallSegmentCorner preferredCorner, const V2f &slack,
                                 const float _height ) {

        auto ln = preferredCorner == WSC_P2 ? ( ls->p1 - ls->p2 ) : ( ls->p2 - ls->p1 );
        ln = normalize( ln );
        dest.xyLocation = ln * ( source.size.width() * 0.5f + dest.size.width() * 0.5f + slack.x());
        dest.xyLocation += source.depthNormal * ( -source.size.depth() * 0.5f + dest.size.depth() * 0.5f + slack.y());
        dest.xyLocation += source.xyLocation;
        dest.heightOffset = _height;
        dest.rotation = source.rotation;
        dest.depthNormal = source.depthNormal;
        dest.widthNormal = source.widthNormal;
    }

//    void placeAroundInBetweenInternal(  FittedFurniture& dest, const FittedFurniture& source, PivotPointPosition where, const V2f& slack, const float _height ) {
//        switch ( where ) {
//            case PivotPointPosition::TopLeft:
//                dest.xyLocation = source.widthNormal * ( source.size.width() * 0.5f + dest.size.width()*0.5f + slack.x()) +
//                                  source.depthNormal * ( -source.size.depth() * 0.5f + dest.size.depth()*0.5f + slack.y());
//                break;
//            case PivotPointPosition::TopRight:
//                dest.xyLocation = source.widthNormal * ( -source.size.width() * 0.5f - dest.size.width()*0.5f + slack.x()) +
//                                  source.depthNormal * ( -source.size.depth() * 0.5f + dest.size.depth()*0.5f + slack.y());
//                break;
//            case PivotPointPosition::TopCenter:
//                dest.xyLocation = source.depthNormal * ( -source.size.depth() * 0.5f + dest.size.depth()*0.5f + slack.y());
//                break;
//            case PivotPointPosition::LeftCenter:
//                dest.xyLocation = source.widthNormal * -( source.size.width() * 0.5f + dest.size.width()*0.5f + slack.x());
//                break;
//            case PivotPointPosition::RightCenter:
//                dest.xyLocation = source.widthNormal * ( source.size.width() * 0.5f + dest.size.width()*0.5f + slack.x());
//                break;
//            default:
//                break;
//        }
//        dest.xyLocation += source.xyLocation;
//        dest.heightOffset = _height;
//        dest.rotation = source.rotation;
//        dest.depthNormal = source.depthNormal;
//        dest.widthNormal = source.widthNormal;
//    }

    bool placeAround( FloorBSData* f, RoomBSData *r, FittedFurniture &dest, const FittedFurniture &source, PivotPointPosition where,
                      const V2f &slack, const float _height ) {
        placeAroundInternal( dest, source, where, slack, _height );
        bool completed = RS::addFurniture( f, r, dest );

        // If the around area is not available, try the opposite direction if there's space
        if ( !completed ) {
            if ( auto newwhere = pivotPointPositionMirrorY( where ); newwhere != PivotPointPosition::Invalid ) {
                placeAroundInternal( dest, source, newwhere, slack, _height );
                completed = RS::addFurniture( f, r, dest );
            }
        }
        return completed;
    }

    bool placeWallAlong( FloorBSData* f, RoomBSData *r, FittedFurniture &dest, const FittedFurniture &source,
                         const ArchSegment *ls, WallSegmentCorner preferredCorner, const V2f &slack,
                         const float _height ) {
        placeAlongWallInternal( dest, source, ls, preferredCorner, slack, _height );
        return RS::addFurniture( f, r, dest );
    }
//    bool placeInBetween( RoomBSData* r, const std::vector<FittedFurniture>& fset, PivotPointPosition where,
//                      const V2f& slack, const float _height ) {
//        placeAroundInternal( dest, source, where, slack, _height );
//        bool completed = RS::addFurniture( r, dest );
//
//        // If the around area is not available, try the opposite direction if there's space
//        if ( !completed ) {
//            if ( auto newwhere = pivotPointPositionMirrorY( where ); newwhere != PivotPointPosition::Invalid ) {
//                placeAroundInternal( dest, source, newwhere, slack, _height );
//                completed = RS::addFurniture( r, dest );
//            }
//        }
//        return completed;
//    }

    bool placeWallCorner( FloorBSData* f, RoomBSData *r, FittedFurniture &_ff, const ArchSegment *ls,
                          const V2f &slack, WallSegmentCorner wsc, float _height ) {
        if ( !ls ) return false;
        if ( checkBitWiseFlag( ls->tag, WF_IsDoorPart ) || checkBitWiseFlag( ls->tag, WF_IsWindowPart )) return false;

        auto cornerPoint = wsc == WSC_P1 ? ls->p1 : ls->p2;
        Vector2f lpn = normalize( ls->middle - cornerPoint );
        Vector2f offset = ls->normal * (( _ff.size.depth() * 0.5f ) + skirtingDepth( r ) + slack.y()) +
                          ( lpn * ( _ff.size.width() * 0.5f + skirtingDepth( r ) + slack.x()));
        _ff.xyLocation = cornerPoint + offset;
        _ff.heightOffset = _height;
        _ff.rotation = Quaternion{ RoomService::furnitureAngleFromWall( ls ), V3f::UP_AXIS };
        _ff.widthNormal = ls->crossNormal;
        _ff.depthNormal = ls->normal;

        return RS::addFurniture( f, r, _ff );
    }

    bool placeWallAligned( FloorBSData* f, RoomBSData *r, FittedFurniture &_ff,
                           const WSLO wslo, float extraSlack, uint32_t _exactIndex ) {

        auto ls = getWallSegmentFor( r, wslo, _exactIndex );
        if ( !ls ) return false;
        if ( checkBitWiseFlag( ls->tag, WF_IsDoorPart ) || checkBitWiseFlag( ls->tag, WF_IsWindowPart )) return false;

        Vector2f offset = ls->normal * (( _ff.size.depth() * 0.5f ) + skirtingDepth( r ) + extraSlack);
        _ff.xyLocation = ls->middle + offset;
        _ff.rotation = Quaternion{ RoomService::furnitureAngleFromWall( ls ), V3f::UP_AXIS };
        _ff.widthNormal = ls->crossNormal;
        _ff.depthNormal = ls->normal;

        return addFurniture( f, r, _ff );
    }

    bool placeMiddle( FloorBSData* f, RoomBSData *r, FittedFurniture &_ff, const V2f &_center ) {

        _ff.xyLocation = _center;
        _ff.rotation = Quaternion{ 0.0f, V3f::UP_AXIS };
        _ff.widthNormal = V2f::X_AXIS;
        _ff.depthNormal = V2f::Y_AXIS;

        return addFurniture( f, r, _ff );
    }

    bool placeDecorations( FloorBSData* f, RoomBSData *r, FittedFurniture &mainF, FurnitureMapStorage &furns,
                           const FurniturePlacementRule &fpd ) {
        bool completed = true;
        if ( fpd.hasDecorations()) {
            for ( const auto &dec : fpd.getDecorations()) {
                auto decF = furns.spawn( dec );
                float h1 = checkBitWiseFlag(decF.flags, FF_CanBeHanged) ? middleHeightFromObject( r, mainF, decF ) : 0.00f;
                completed &= h1 >= 0.0f;
                if ( h1 >= 0.0f ) {
                    float h = mainF.size.y() + h1;
                    completed &= RS::placeAround( f, r, decF, mainF, PPP::TopCenter, V2f::ZERO, h );
                }
            }
        }
        return completed;
    }

    bool cplaceMainWith2Sides( FloorBSData* f, RoomBSData *r, FurnitureMapStorage &furns, const FurniturePlacementRule &fpd ) {
        FT main = fpd.getBase( 0 );
        WSLO refWall = fpd.getWallSegmentId().type;
        bool completed = true;

        auto mainF = furns.spawn( main );
        completed = RS::placeWallAligned( f, r, mainF, refWall );
        if ( completed ) {
            if ( fpd.hasBase( 1 )) {
                completed &= RS::placeAround( f, r, furns.spawn( fpd.getBase( 1 )), mainF, PPP::TopLeft );
            }
            if ( fpd.hasBase( 2 )) {
                completed &= RS::placeAround( f, r, furns.spawn( fpd.getBase( 2 )), mainF, PPP::TopRight );
            }
        }
        completed &= RS::placeDecorations( f, r, mainF, furns, fpd );
        return completed;
    }

    bool cplaceSetAlignedMiddle( FloorBSData* f, RoomBSData *r, FurnitureMapStorage &furns, const FurniturePlacementRule &fpd ) {
        FT main = fpd.getBase( 0 );
        WSLO refWall = fpd.getWallSegmentId().type;
        bool completed = true;
        auto mainF = furns.spawn( main );
        completed = RS::placeWallAligned( f, r, mainF, refWall);
        // If it has more than 1 furniture then place them in front of the main furniture, like a coffee table in front of a sofa
        if ( fpd.hasBase( 1 ) ) {
            completed &= RS::placeAround( f, r, furns.spawn( fpd.getBase( 1 )), mainF, PPP::BottomCenter, fpd.getSlack() );
        }
        completed &= RS::placeDecorations( f, r, mainF, furns, fpd );
        return completed;
    }

    bool cplaceSetBestFit( FloorBSData* f, RoomBSData *r, FurnitureMapStorage &furns, const FurniturePlacementRule &fpd ) {
        FT main = fpd.getBase( 0 );
        WSLO refWall = fpd.getWallSegmentId().type;
        bool completed = true;
        auto mainF = furns.spawn( main );
        for ( auto rwIndex = 0UL; rwIndex < r->mWallSegmentsSorted.size(); rwIndex++ ) {
            completed = RS::placeWallAligned( f, r, mainF, refWall, fpd.getSlack(0).x(), rwIndex );
            if ( completed ) break;
        }
        // If it has more than 1 furniture then place them in front of the main furniture, like a coffee table in front of a sofa
        if ( fpd.hasBase( 1 ) ) {
            completed &= RS::placeAround( f, r, furns.spawn( fpd.getBase( 1 )), mainF, PPP::BottomCenter, fpd.getSlack() );
        }
        completed &= RS::placeDecorations( f, r, mainF, furns, fpd );
        return completed;
    }

    bool cplaceSetAlignedAtCorner( FloorBSData* f, RoomBSData *r, FurnitureMapStorage &furns, const FurniturePlacementRule &fpd ) {
        const std::vector<FT> &fset = fpd.getBases();
        WSLO refWall = fpd.getWallSegmentId().type;

        bool completed = true;

        auto ls = getWallSegmentFor( r, refWall, fpd.getWallSegmentId().index );
        if ( !ls ) return false;
        if ( checkBitWiseFlag( ls->tag, WF_IsDoorPart ) || checkBitWiseFlag( ls->tag, WF_IsWindowPart )) return false;
        auto prevFurn = furns.spawn( fset.front());
        completed = RS::placeWallCorner( f, r, prevFurn, ls, fpd.getSlack().xy(), fpd.getPreferredCorner());
        if ( !completed ) return false;

        for ( size_t t = 1; t < fset.size(); t++ ) {
            auto currFurn = furns.spawn( fset[t] );
            completed &= RS::placeWallAlong( f, r, currFurn, prevFurn, ls, fpd.getPreferredCorner(),
                                             fpd.getSlack( t ).xy());
            if ( !completed ) break;
            prevFurn = currFurn;
        }
        return completed;
    }

    bool cplaceCornerWithDec( FloorBSData* f, RoomBSData *r, FurnitureMapStorage &furns, const FurniturePlacementRule &fpd ) {
        FT main = fpd.getBase( 0 );
        WSLO refWall = fpd.getWallSegmentId().type;

        bool completed = true;
        auto mainF = furns.spawn( main );
        auto ls = getWallSegmentFor( r, refWall, fpd.getWallSegmentId().index );
        completed &= RS::placeWallCorner( f, r, mainF, ls, fpd.getSlack().xy(), WSC_P2 );
        if ( !completed ) return false;

        if ( fpd.hasDecorations()) {
            const std::vector<FT> &decorations = fpd.getDecorations();
            auto dec = furns.spawn( decorations.front());
            float h1 = middleHeightFromObject( r, mainF, dec );
            completed &= h1 >= 0.0f;
            if ( h1 >= 0.0f ) {
                float h = mainF.size.y() + h1;
                completed &= RS::placeWallCorner( f, r, dec, ls, fpd.getSlack().xy(), WSC_P2, h );
            }
        } else {
            completed = false;
        }
        return completed;
    }

    bool cplacedFirstAvailableCorner( FloorBSData* f, RoomBSData *r, FurnitureMapStorage &furns, const FurniturePlacementRule &fpd ) {
        const std::vector<FT> &fset = fpd.getBases();
        bool completed = true;

        for ( const auto& wss : r->mWallSegments ) {
            for ( auto i = 0u; i < wss.size(); ++i ) {
                auto ls = &wss[i];
                if ( checkBitWiseFlag( ls->tag, WF_IsDoorPart ) || checkBitWiseFlag( ls->tag, WF_IsWindowPart )) continue;
                auto ls2 = &wss[cai(i+1, wss.size())];
                auto p1 = ls->p1;
                auto p2 = ls->p2;
                auto p3 = ls2->p2;
                if ( detectWindingOrder(p1, p2, p3) == WindingOrder::CCW ) {
                    auto prevFurn = furns.spawn(fset.front());
                    completed = RS::placeWallCorner( f, r, prevFurn, ls2, fpd.getSlack().xy(), WSC_P1);
                    if ( completed ) break;
                }
            }
        }

        return completed;
    }

    bool cplaceMiddleOfRoom( FloorBSData* f, RoomBSData *r, FurnitureMapStorage &furns, const FurniturePlacementRule &fpd ) {
        auto center = RS::maxEnclsingBoundingBoxCenter( r );

        return placeMiddle( f, r, furns.spawn( fpd.getBase( 0 )), center );
    }

    bool runRuleScript( FloorBSData* f, RoomBSData *r, FurnitureMapStorage &furns, const FurnitureRuleScript &fs ) {
        return fs.execute( f, r, furns, RS::functionRules );
    }

    bool addFurniture( FloorBSData* f, RoomBSData *r, FittedFurniture &_ff ) {
        _ff.position3d = XZY::C( _ff.xyLocation, _ff.heightOffset );
        _ff.bbox3d = AABB{ _ff.position3d - ( _ff.size * 0.5f ), _ff.position3d + ( _ff.size * 0.5f ) };
        _ff.bbox3d = _ff.bbox3d.rotate( _ff.rotation );

        ClipperLib::Clipper c;
        ClipperLib::Paths solution;
        c.AddPath( ClipperLib::V2fToPath( _ff.bbox3d.topDown().points()), ClipperLib::ptSubject, true );
        for ( const auto& door : f->doors ) {
            c.AddPath( ClipperLib::V2fToPath( door->bbox.squared().points()), ClipperLib::ptClip, true );
        }
        c.AddPath( ClipperLib::V2fToPath( r->mPerimeterSegments ), ClipperLib::ptClip, true );

        c.Execute( ClipperLib::ctDifference, solution, ClipperLib::pftNonZero, ClipperLib::pftNonZero );

        if ( solution.empty() ) {
            if ( !_ff.checkIf( FF_CanOverlap )) {
                for ( const auto &furn : r->mFittedFurniture ) {
                    if ( _ff.heightOffset != furn.heightOffset ) {
                        if ( _ff.heightOffset > furn.heightOffset + furn.bbox3d.calcHeight() ||
                             _ff.heightOffset + _ff.bbox3d.calcHeight() < furn.heightOffset ) {
                            continue;
                        }
                    }
                    if ( _ff.bbox3d.topDown().intersect( furn.bbox3d.topDown(), 0.001f )) {
                        return false;
                    }
                }
            }
            r->mFittedFurniture.push_back( _ff );
            return true;
        }
        return false;
    }

    void clearFurniture( RoomBSData *r ) {
        r->mFittedFurniture.clear();
    }

    void furnishBedroom( FloorBSData* f, RoomBSData *r, FurnitureMapStorage &furns ) {
        r->floorMaterial = "carpet,grey";
        FurnitureRuleScript ruleScript;
        V3f cornerSlack{ 0.05f, 0.0f, 0.0f };
        ruleScript.clear();
        ruleScript.addRule( FurniturePlacementRule{ FurnitureRuleIndex( RS::MainWith2Sides ),
                                                    FurnitureRefs{{ FTH::Bed(), FTH::Bedside(), FTH::Bedside() },
                                                                  { FTH::Picture() }}, WSLOH::SecondLongest() } );
        ruleScript.addRule( FurniturePlacementRule{ FurnitureRuleIndex( RS::CornerWithDec ),
                                                    FurnitureRefs{{ FTH::Bedside() },
                                                                  { FTH::Picture() }}, WSLOH::Longest(),
                                                    FurnitureSlacks{cornerSlack} } );
        ruleScript.addRule( FurniturePlacementRule{ FurnitureRuleIndex( RS::SetAlignedAtCorner ),
                                                    FurnitureRefs{{ FTH::Shelf(), FTH::Shelf(), FTH::Sofa() }},
                                                    WSLOH::Longest(),
                                                    FurnitureSlacks{ cornerSlack, V3f::ZERO, cornerSlack }} );
        ruleScript.addRule( FurniturePlacementRule{ FurnitureRuleIndex( RS::SetAlignedAtCorner ),
                                                    FurnitureRefs{{ FTH::Wardrobe(), FTH::Wardrobe() }},
                                                    WSLOH::LongestOpposite(), WSC_P2 } );
        ruleScript.addRule( FurniturePlacementRule{ FurnitureRuleIndex( RS::SetAlignedAtCorner ),
                                                    FurnitureRefs{{ FTH::Shelf(), FTH::Wardrobe(), FTH::Armchair() }},
                                                    WallSegmentIdentifier{ WSLOH::ExactIndex(), 3 },
                                                    FurnitureSlacks{ V3f::ZERO, V3f::ZERO, cornerSlack * 0.5f },
                                                    WSC_P2 } );
        ruleScript.addRule( FurniturePlacementRule{ FurnitureRuleIndex( RS::SetAlignedAtCorner ),
                                                    FurnitureRefs{{ FTH::Armchair() }},
                                                    WallSegmentIdentifier{ WSLOH::ExactIndex(), 4 },
                                                    WSC_P2 } );
        ruleScript.addRule( FurniturePlacementRule{ FurnitureRuleIndex( RS::SetAlignedAtCorner ),
                                                    FurnitureRefs{{ FTH::Armchair() }},
                                                    WallSegmentIdentifier{ WSLOH::ExactIndex(), 4 },
                                                    WSC_P1 } );
        ruleScript.addRule(
                FurniturePlacementRule{ FurnitureRuleIndex( RS::MiddleOfRoom ), FurnitureRefs{{ FTH::Carpet() }}} );
        RS::runRuleScript( f, r, furns, ruleScript );
    }

    void furnishLiving( FloorBSData* f, RoomBSData *r, FurnitureMapStorage &furns ) {
        FurnitureRuleScript ruleScript;
        V3f inFrontOfSlack{ 0.0f, 0.2f, 0.0f };
        ruleScript.addRule( FurniturePlacementRule{ FurnitureRuleIndex( RS::SetAlignedMiddle ),
                                                    FurnitureRefs{{ FTH::Sofa(), FTH::CoffeeTable() }},
                                                    WallSegmentIdentifier{ WSLOH::Longest() },
                                                    FurnitureSlacks{ inFrontOfSlack }
        } );
        ruleScript.addRule( FurniturePlacementRule{ FurnitureRuleIndex( RS::SetAlignedMiddle ),
                                                    FurnitureRefs{{ FTH::SideBoard()},{ FTH::TVWithStand() }},
                                                    WallSegmentIdentifier{ WSLOH::LongestOpposite() }
        } );
        ruleScript.addRule( FurniturePlacementRule{ FurnitureRuleIndex( RS::CornerWithDec ),
                                                    FurnitureRefs{{ FTH::Plant()},{ FTH::Picture() }},
                                                    WallSegmentIdentifier{ WSLOH::Longest() }
        } );
        ruleScript.addRule(
                FurniturePlacementRule{ FurnitureRuleIndex( RS::MiddleOfRoom ), FurnitureRefs{{ FTH::Carpet() }}} );
        RS::runRuleScript( f, r, furns, ruleScript );
    }

    void furnishDining( FloorBSData* f, RoomBSData *r, FurnitureMapStorage &furns ) {
        FurnitureRuleScript ruleScript;
        V3f tableSlack{ 0.5f, 0.5f, 0.0f };
        ruleScript.addRule( FurniturePlacementRule{ FurnitureRuleIndex( RS::FRBestFit ),
                                                    FurnitureRefs{{ FTH::DiningTable() }},
                                                    WallSegmentIdentifier{ WSLOH::ExactIndex() },
                                                    FurnitureSlacks{ tableSlack }
        } );
        RS::runRuleScript( f, r, furns, ruleScript );
    }

    void furnishBathroom( FloorBSData* f, RoomBSData *r, FurnitureMapStorage &furns ) {
        FurnitureRuleScript ruleScript;
        r->floorMaterial = "pavonazzo,tiles";
        r->wallMaterial = "yule,tiles";
        r->wallColor = C4f::WHITE;
        ruleScript.addRule( FurniturePlacementRule{ FurnitureRuleIndex( RS::FRFirstAvailableCorner ),
                                                    FurnitureRefs{{ FTH::Shower() }}
        } );
        ruleScript.addRule( FurniturePlacementRule{ FurnitureRuleIndex( RS::FRBestFit ),
                                                    FurnitureRefs{{ FTH::Toilet() }},
                                                    WallSegmentIdentifier{ WSLOH::ExactIndex() }
        } );
        ruleScript.addRule( FurniturePlacementRule{ FurnitureRuleIndex( RS::FRBestFit ),
                                                    FurnitureRefs{{ FTH::BathroomSink() }},
                                                    WallSegmentIdentifier{ WSLOH::ExactIndex() }
        } );
        ruleScript.addRule( FurniturePlacementRule{ FurnitureRuleIndex( RS::FRBestFit ),
                                                    FurnitureRefs{{ FTH::BathroomTowerRadiator() }},
                                                    WallSegmentIdentifier{ WSLOH::ExactIndex() }
        } );
        RS::runRuleScript( f, r, furns, ruleScript );
    }

    void furnishKitchen( FloorBSData* f, RoomBSData *r, FurnitureMapStorage &furns ) {
        r->floorMaterial = "yule,tiles";
        r->wallMaterial = "terrazzo,tiles";

    }

    void furnish( FloorBSData* f, RoomBSData *r, FurnitureMapStorage &furns ) {

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

        for ( const auto&  prioritySort : prioritySorted ) {
            for ( const auto rtype : prioritySort ) {
                switch ( rtype ) {
                    case ASType::GenericRoom:
                        break;
                    case ASType::Bathroom:
                        furnishBathroom( f, r, furns );
                        break;
                    case ASType::Kitchen:
                        furnishKitchen( f, r, furns );
                        break;
                    case ASType::BedroomDouble:
                    case ASType::BedroomMaster:
                    case ASType::BedroomSingle:
                        furnishBedroom( f, r, furns );
                        break;
                    case ASType::LivingRoom:
                    case ASType::Studio:
                        furnishLiving( f, r, furns );
                        break;
                    case ASType::DiningRoom:
                        furnishDining( f, r, furns );
                        break;
                    default:
                        break;
                }
            }
        }
    }

}

FittedFurniture &FurnitureMapStorage::spawn( FT _ft ) {
    auto f = index[_ft];
    storage.emplace( _ft, f );

    auto range = storage.equal_range( _ft );
    auto i = range.first;
    auto ret = range.first;
    for ( ; i != range.second; ++i ) {
        ret = i;
    }
    return ret->second;
}

void initializeDefaultFurnituresFlags( FT _ft, FittedFurniture &_ff ) {
    if ( _ft == FTH::Carpet() || _ft == FTH::TVWithStand()) {
        orBitWiseFlag( _ff.flags, FF_CanOverlap );
    }
    if ( _ft == FTH::Picture()) {
        orBitWiseFlag( _ff.flags, FF_CanBeHanged | FF_isDecoration );
    }
}

void FurnitureMapStorage::addIndex( FT _ft, FittedFurniture &_ff ) {
    initializeDefaultFurnituresFlags( _ft, _ff );
    index.emplace( _ft, _ff );
}

void FurnitureMapStorage::addIndex( const FurnitureSet& f ) {
    auto ff = FittedFurniture{ {f.name, f.bboxSize }, f.symbol };
    this->addIndex( FT(f.ftype), ff );
}

FurniturePlacementRule FurniturePlacementRule::randomGeneration() {
    FurniturePlacementRule ret{ unitRandI( static_cast<int>(RS::functionRules.size()) - 1 ) };

    ret.addBase( FTH::at( unitRandI( FTH::count() - 1 )));
    ret.addBase( FTH::at( unitRandI( FTH::count() - 1 )));
    ret.addBase( FTH::at( unitRandI( FTH::count() - 1 )));
    ret.setWallSegmentId( { WSLOH::at( unitRandI( WSLOH::count() - 1 )), 0 } );
    ret.setPreferredCorner( WallSegmentCorner( unitRandI( 2 )));

    return ret;
}
void RoomServiceFurniture::addDefaultFurnitureSet( const std::string &_name ) {
    std::string brimnes_bed = "Brimnes";
    std::string lauter_selije = "lauter_selije";
    std::string hemnes_shelf = "hemnes_shelf";
    std::string hemnes_drawer = "hemnes_drawer";
    std::string soderhamn = "soderhamn";
    std::string carpet_flottebo = "carpet";
    std::string Strandmon = "strandmon";
    std::string pictures_set_3 = "pictures2";
    std::string coffeeTable = "noguchi";
    std::string diningTable = "ktc,table,round";
    std::string sideBoard = "sideboard";
    std::string tv = "sony,tv";
    std::string plant1 = "plant,spider";
    std::string toilet = "urby,toilet";
    std::string shower = "generic,shower,round";
    std::string bathroomSink = "urby,sink";
    std::string bathroomTowerRadiator = "bathroom,tower,radiator";

    std::string queenBedIcon = "fia,queen,icon";
    std::string bedSideIcon = "fia,bedside";
    std::string sofaIcon = "fia,sofa,3seaters";
    std::string wardrobeIcon = "fia,wardrobe";
    std::string shelfIcon = "fia,shelf";
    std::string armchairIcon = "fia,armchair";
    std::string coffeeTableIcon = "fia,coffee,table";
    std::string toiletIcon = "fia,toilet";
    std::string bathroomSinkIcon = "fia,double,sink";

    FurnitureSetContainer furnitureSet{};
    furnitureSet.name = _name;
    furnitureSet.set.emplace_back( FTH::Bed(), brimnes_bed, V3f::ZERO, queenBedIcon );
    furnitureSet.set.emplace_back( FTH::Bedside(), lauter_selije, V3f::ZERO, bedSideIcon );
    furnitureSet.set.emplace_back( FTH::Shelf(), hemnes_shelf, V3f::ZERO, shelfIcon );
    furnitureSet.set.emplace_back( FTH::Wardrobe(), hemnes_drawer, V3f::ZERO, wardrobeIcon );
    furnitureSet.set.emplace_back( FTH::Drawer(), hemnes_drawer, V3f::ZERO, wardrobeIcon );
    furnitureSet.set.emplace_back( FTH::Sofa(), soderhamn, V3f::ZERO, sofaIcon );
    furnitureSet.set.emplace_back( FTH::Picture(), pictures_set_3, V3f::ZERO, S::SQUARE );
    furnitureSet.set.emplace_back( FTH::Carpet(), carpet_flottebo, V3f::ZERO, S::SQUARE );
    furnitureSet.set.emplace_back( FTH::Armchair(), Strandmon, V3f::ZERO, armchairIcon );
    furnitureSet.set.emplace_back( FTH::CoffeeTable(), coffeeTable, V3f::ZERO, coffeeTableIcon );
    furnitureSet.set.emplace_back( FTH::DiningTable(), diningTable, V3f::ZERO, coffeeTableIcon );
    furnitureSet.set.emplace_back( FTH::SideBoard(), sideBoard, V3f::ZERO, S::SQUARE );
    furnitureSet.set.emplace_back( FTH::TVWithStand(), tv, V3f::ZERO, S::SQUARE );
    furnitureSet.set.emplace_back( FTH::Plant(), plant1, V3f::ZERO, S::SQUARE );
    furnitureSet.set.emplace_back( FTH::Toilet(), toilet, V3f::ZERO, toiletIcon );
    furnitureSet.set.emplace_back( FTH::Shower(), shower, V3f::ZERO, S::SQUARE );
    furnitureSet.set.emplace_back( FTH::BathroomSink(), bathroomSink, V3f::ZERO, bathroomSinkIcon );
    furnitureSet.set.emplace_back( FTH::BathroomTowerRadiator(), bathroomTowerRadiator, V3f::ZERO, S::SQUARE );

    Http::post( Url{ "/furnitureset" }, furnitureSet.serialize(), []( HttpResponeParams &res ) {
        LOGRS( res.bufferString );
    } );
}
