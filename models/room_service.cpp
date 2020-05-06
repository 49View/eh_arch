//
//  room_service.cpp
//  sixthview
//
//  Created by Dado on 16/7/2017.
//
//

#include "room_service.hpp"

#include "floor_service.hpp"
#include "core/math/triangulator.hpp"
#include "core/service_factory.h"
#include "arch_creator.hpp"
#include "arch_segment_service.hpp"
#include "arch_structural_service.hpp"
#include "wall_service.hpp"

namespace RoomService {

    std::shared_ptr<RoomBSData> createRoom( const RoomPreData& _preData, const float _floorHeight, const float _z ) {
        auto w = ServiceFactory::create<RoomBSData>();
        w->type = ArchType::RoomT;
        w->asType = _preData.rtype;
        w->height = _floorHeight;
        w->width = _preData.bboxInternal.calcWidth();
        w->depth = _preData.bboxInternal.calcHeight();
        w->z = _z;

        switch ( w->asType ) {
            case ASType::Kitchen:
                w->floorType = FloorMatType::Tiles;
                break;

            case ASType::BedroomSingle:
            case ASType::BedroomDouble:
            case ASType::BedroomMaster:
                w->floorType = FloorMatType::Carpet;
                break;

            case ASType::Bathroom:
            case ASType::Ensuite:
            case ASType::ShowerRoom:
            case ASType::ToiletRoom:
                w->floorType = FloorMatType::Tiles;
                break;

            case ASType::GenericRoom:
            case ASType::LivingRoom:
            case ASType::Studio:
            case ASType::Conservatory:
            case ASType::DiningRoom:
            case ASType::GamesRoom:
            case ASType::Laundry:
            case ASType::Hallway:
            case ASType::Garage:
            default:
                w->floorType = FloorMatType::Wood;
        }

        return w;
    }

    void
    addSocketIfSafe( RoomBSData *r, const std::vector<Vector2f>& cov, size_t indexSkirting,
                     float safeSocketBoxWidth ) {
        if ( distance( cov[indexSkirting], cov[indexSkirting + 1] ) > safeSocketBoxWidth ) {
            Vector2f snormal = normalize( cov[indexSkirting] - cov[indexSkirting + 1] );
            Vector2f sanePos = cov[indexSkirting] + ( -snormal * safeSocketBoxWidth * 0.5f );
            float fa = furnitureAngleFromNormal( rotate90( snormal ));
            r->mSocketLocators.emplace_back( sanePos, fa );
            if ( indexSkirting == 0 ) {
                r->mSwichesLocators.emplace_back( sanePos, fa );
            }
        }
    }

    void calcOptimalLightingFittingPositions( RoomBSData *r ) {
        JMATH::Rect2f br( r->mMaxEnclsingBoundingBox );
        Vector2f bs = br.size();
        bool bCenterRoomOnly = false;

        r->mLightFittingsLocators.clear();

        int numX = static_cast<int>( bs.x() / r->minLightFittingDistance );
        int numY = static_cast<int>( bs.y() / r->minLightFittingDistance );

        if ( numX == 0 || numY == 0 || bCenterRoomOnly ) {
            r->mLightFittingsLocators.emplace_back( br.centre(), r->height - r->defaultCeilingThickness );
            return;
        }

        for ( auto t = 1; t < numY + 1; t++ ) {
            float ly = static_cast<float>( t ) / static_cast<float>( numY + 1 );
            float lvy = lerp( ly, br.bottom(), br.top());
            Vector2f lv1 = { br.left(), lvy };
            Vector2f lv2 = { br.right(), lvy };
            for ( auto m = 1; m < numX + 1; m++ ) {
                float lx = static_cast<float>( m ) / static_cast<float>( numX + 1 );
                Vector2f i = lerp( lx, lv1, lv2 );
                r->mLightFittingsLocators.emplace_back( i, r->height - r->defaultCeilingThickness );
            }
        }
    }

    void addSocketsAndSwitches( RoomBSData *r ) {
        r->mSwichesLocators.clear();
        r->mSocketLocators.clear();

        // add locators for sockets and switches
        float safeSingleSocketBoxWidth = 0.12f * 1.5f;
        for ( const auto& cov : r->mvSkirtingSegments ) {
            size_t csize = cov.size();
            if ( csize > 1 ) {
                addSocketIfSafe( r, cov, 0, safeSingleSocketBoxWidth );
            }
            if ( csize > 2 ) {
                addSocketIfSafe( r, cov, csize - 2, safeSingleSocketBoxWidth );
            }
        }
    }

    void calcSkirtingSegments( RoomBSData *r ) {
        // Skirting
        r->mvSkirtingSegments.clear();
        for ( auto& rws : r->mWallSegments ) {
            int csize = static_cast<int>( rws.size());
            Vector2f p1 = Vector2f::HUGE_VALUE_POS;
            std::vector<Vector2f> points;
            int startIndex = 0;
            for ( int q = 0; q < csize; q++ ) {
                if ( checkBitWiseFlag( rws[getCircularArrayIndex( q - 1, csize )].tag, WallFlags::WF_IsDoorPart ) &&
                     !checkBitWiseFlag( rws[q].tag, WallFlags::WF_IsDoorPart )) {
                    startIndex = q;
                    break;
                }
            }
            for ( int q = startIndex; q < startIndex + csize; q++ ) {
                int qc = getCircularArrayIndex( q, csize );
                if ( checkBitWiseFlag( rws[qc].tag, WallFlags::WF_IsDoorPart | WallFlags::WF_IsWindowPart )) {
                    continue;
                }
                int qcm1 = getCircularArrayIndex( q - 1, csize );
                int qcp1 = getCircularArrayIndex( q + 1, csize );
                if (( rws[qcm1].tag & WallFlags::WF_IsDoorPart ) > 0 ) {
                    Vector2f p2mp1 = rws[qc].p2 - rws[qc].p1;
                    if ( length( p2mp1 ) > r->mArchiTravesWidth ) {
                        p1 = rws[qc].p1 + ( normalize( p2mp1 ) * r->mArchiTravesWidth );
                        points.push_back( p1 );
                    }
                }
                p1 = rws[qc].p2;
                points.push_back( p1 );
                if ((( rws[qcp1].tag & WallFlags::WF_IsDoorPart ) > 0 )) {
                    Vector2f p2mp1 = points[points.size() - 2] - p1;
                    if ( length( p2mp1 ) > r->mArchiTravesWidth ) {
                        p1 += normalize( p2mp1 ) * r->mArchiTravesWidth;
                        points[points.size() - 1] = p1;
                    } else {
                        points.pop_back();
                    }
                    //removeCollinear( points, accuracy1mm );
                    if ( points.size() >= 2 ) r->mvSkirtingSegments.push_back( points );
                    points.clear();
                }
            }
            if ( r->mvSkirtingSegments.size() == 0 && points.size() > 1 ) {
                removeCollinear( points, accuracy1mm );
                if ( points.size() >= 2 ) r->mvSkirtingSegments.push_back( points );
            }
        }
    }

    bool checkMaxSizeEnclosure( RoomBSData *r, Vector2f& ep1, Vector2f& ep2,
                                const Vector2f& ncheck ) {
        bool foundAny = false;
        Vector2f i;
        float maxDist = 0.0f;
        float maxLengths = 0.0f;
        size_t csize = r->mPerimeterSegments.size();
        for ( size_t q = 0; q < csize; q++ ) {
            auto qp = r->mPerimeterSegments[q];
            auto qp1 = r->mPerimeterSegments[getCircularArrayIndexUnsigned( q + 1, csize )];
            for ( size_t m = 0; m < csize; m++ ) {
                if ( m == q ) continue;
                auto mp = r->mPerimeterSegments[m];
                auto mp1 = r->mPerimeterSegments[getCircularArrayIndexUnsigned( m + 1, csize )];
                auto mpmiddle = lerp( 0.5f, mp, mp1 );
                auto mpn = normalize( mp1 - mp );
                auto mnr = rotate90( mpn ) * 1000.0f;
                if ( intersection( qp, qp1, mpmiddle + mnr, mpmiddle - mnr, i )) {
                    float d = distance( mpmiddle, i );
                    if ( ncheck.x() != Vector2f::HUGE_VALUE_POS.x()) {
                        float dn = fabs( dot( ncheck, normalize( mpmiddle - i )));
                        if ( dn > 0.95f ) continue;
                    }
                    float l = distance( qp, qp1 ) + distance( mp, mp1 );
                    if (( d - SMALL_EPSILON > maxDist ) || (( d >= maxDist - SMALL_EPSILON ) && ( l > maxLengths ))) {
                        maxDist = d;
                        maxLengths = l;
                        ep1 = i;
                        ep2 = mpmiddle;
                        foundAny = true;
                    }
                }
            }
        }
        return foundAny;
    }

    void WallSegments( RoomBSData *r, const std::vector<std::vector<ArchSegment>>& val ) {
        r->mWallSegments = val;

        // Perimeter
        WallService::perimeterFromSegments( val, r->mPerimeterSegments, r->mPerimeter );

        // Find max room span points
        Vector2f ep1, ep2;
        Vector2f ncheck = Vector2f::HUGE_VALUE_POS;
        bool bEncloseFound = checkMaxSizeEnclosure( r, ep1, ep2, ncheck );
        if ( bEncloseFound ) {
            r->maxSizeEnclosedHP1 = ep1;
            r->maxSizeEnclosedHP2 = ep2;
            ncheck = normalize( ep1 - ep2 );
            bEncloseFound = checkMaxSizeEnclosure( r, ep1, ep2, ncheck );
            if ( bEncloseFound ) {
                r->maxSizeEnclosedWP1 = ep1;
                r->maxSizeEnclosedWP2 = ep2;
            }
        }

        // Coving
        r->mvCovingSegments = calcCovingSegments( r->mWallSegments );
        r->mCovingPerimeter = calculatePerimeterOf( r->mvCovingSegments );

        calcSkirtingSegments( r );

        // Max bounding box
        // ### This has to be done AFTER coving calculations are it uses the coving segment to determine a "sane" max bbox
        r->mBBoxCoving = getContainingBBox( r->mvCovingSegments );
        calclMaxBoundingBox( r );
        makeTriangles2d( r );
    }

    void makeTriangles2d( RoomBSData *r ) {
        r->mTriangles2d.clear();
        Triangulator tri( r->mPerimeterSegments );
        r->mTriangles2d = tri.get2dTrianglesTuple();
    }

    void calclMaxBoundingBox( RoomBSData *r ) {
        float maxDist = 0.0f;
        float minDist = std::numeric_limits<float>::max();
        Vector2f v1;
        Vector2f v2;
        Vector2f v3;
        Vector2f v4;
        for ( auto& covs : r->mvCovingSegments ) {
            size_t csize = covs.size();
            for ( size_t q = 0; q < csize; q++ ) {
                size_t q1 = getCircularArrayIndexUnsigned( q + 1, csize );
                float dist = distance( covs[q], covs[q1] );
                if ( dist > maxDist ) {
                    v1 = covs[q];
                    v2 = covs[q1];
                    maxDist = dist;
                }
            }
        }

        // right we have the longest coving segment now, in v1, v2, now trace a ray from middle of v1,v2 to see where it hits
        Vector2f vn = normalize( rotate90( v2 - v1 ));
        Vector2f vm = lerp( 0.5f, v1, v2 );
        Vector2f vi1 = vm + vn * 10000.0f;
        Vector2f vi2 = vm - vn * 10000.0f;
        Vector2f vi;
        for ( auto& covs : r->mvCovingSegments ) {
            size_t csize = covs.size();
            for ( size_t q = 0; q < csize; q++ ) {
                size_t q1 = getCircularArrayIndexUnsigned( q + 1, csize );
                if ( covs[q] == v1 && covs[q1] == v2 ) continue;
                if ( intersection( vi1, vi2, covs[q], covs[q1], vi )) {
                    float dist = distance( vm, vi );
                    if ( dist > VERY_SMALL_EPSILON && dist < minDist ) {
                        minDist = dist;
                        v3 = vi;
                    }
                }
            }
        }
        // Tentative bbox
        Vector2f vnnr = normalize( v2 - v1 );
        vi1 = v3 + vnnr * 10000.0f;
        vi2 = v3 - vnnr * 10000.0f;

        intersection( v1 - vn * 10000.0f, v1 + vn * 10000.0f, vi1, vi2, v3 );
        intersection( v2 - vn * 10000.0f, v2 + vn * 10000.0f, vi1, vi2, v4 );

        r->mMaxEnclsingBoundingBox.clear();
        r->mMaxEnclsingBoundingBox.push_back( v1 );
        r->mMaxEnclsingBoundingBox.push_back( v2 );
        r->mMaxEnclsingBoundingBox.push_back( v3 );
        r->mMaxEnclsingBoundingBox.push_back( v4 );
    }

    void updateFromArchSegments( RoomBSData *r,
                                 const std::vector<std::vector<ArchSegment>>& ws ) {
        //	r->SkirtingProfile( loadSkirtingProfile() );
        //	r->CovingProfile( loadCovingProfile() );

        WallSegments( r, ws );

        calcLongestWall( r );

        calcBBox( r );
    }

    std::vector<std::vector<Vector2f>>
    calcCovingSegments( const std::vector<std::vector<ArchSegment>>& ws ) {
        std::vector<std::vector<Vector2f>> lCovingSegments;

        for ( auto& rws : ws ) {
            std::vector<Vector2f> points;
            int csize = static_cast<int>( rws.size());
            points.reserve( csize );
            for ( int q = 0; q < csize; q++ ) {
                points.push_back( rws[q].p2 );
            }
            removeCollinear( points, 0.001f );
            lCovingSegments.push_back( points );
        }

        return lCovingSegments;
    }

    float skirtingDepth( const RoomBSData *r ) {
//	Profile skirtingProfile{ "skirting_kensington" };
//	return skirtingProfile.width();
        // TODO: Re-enable profiles
        return .02f;
    }

    void setCoving( RoomBSData *r, bool _state ) {
        r->mHasCoving = _state;
    }

    void changeFloorType( RoomBSData *r, FloorMatTypeT _fmt ) {
        r->floorType = _fmt;
    }

    size_t numTotalSegments( const RoomBSData *r ) {
        size_t n = 0;
        for ( auto& rws : r->mWallSegments ) {
            n += rws.size();
        }
        return n;
    }

    bool roomNeedsCoving( const RoomBSData *r ) {
        return ( r->asType != ASType::Bathroom && r->asType != ASType::ToiletRoom && r->asType != ASType::ShowerRoom &&
                 r->asType != ASType::Ensuite );
    }

    float area( const RoomBSData *r ) {
        float ret = 0.0f;
        for ( auto& vts : r->mTriangles2d ) {
            auto a = std::get<0>( vts );
            auto b = std::get<1>( vts );
            auto c = std::get<2>( vts );
            ret += fabs(( a.x() - c.x()) * ( b.y() - a.y()) - ( a.x() - b.x()) * ( c.y() - a.y())) / 2;
        }
        return ret;
    }

    roomTypeIndex sortedSegmentToPairIndex( const RoomBSData *r, int si ) {
        roomTypeIndex rti = std::make_pair( -1, -1 );

        for ( auto t = 0u; t < r->mWallSegments.size(); t++ ) {
            for ( auto m = 0u; m < r->mWallSegments[t].size(); m++ ) {
                if ( r->mWallSegments[t][m].p1 == r->mWallSegmentsSorted[si].p1 &&
                     r->mWallSegments[t][m].p2 == r->mWallSegmentsSorted[si].p2 ) {
                    rti = std::make_pair( t, m );
                    return rti;
                }
            }
        }
        return rti;
    }

    void rescale( RoomBSData *r, float _scale ) {
        ArchStructuralService::rescale( r, _scale );

        Vector3f scale3f = { _scale, _scale, 1.0f };
        for ( auto& covs : r->mWallSegments ) {
            for ( auto& s : covs ) {
                ArchSegmentService::rescale( s, _scale );
            }
        }
        for ( auto& s : r->mWallSegmentsSorted ) {
            ArchSegmentService::rescale( s, _scale );
        }
        for ( auto& s : r->mPerimeterSegments ) {
            s *= _scale;
        }
        r->mPerimeter *= _scale;
        for ( auto& s : r->mMaxEnclsingBoundingBox ) {
            s *= _scale;
        }
        for ( auto& s : r->mLightFittingsLocators ) {
            s *= scale3f;
        }
        for ( auto& covs : r->mvCovingSegments ) {
            for ( auto& s : covs ) {
                s *= _scale;
            }
        }
        for ( auto& covs : r->mvSkirtingSegments ) {
            for ( auto& s : covs ) {
                s *= _scale;
            }
        }
        r->mLongestWallOppositePoint *= _scale;

        r->maxSizeEnclosedHP1 *= _scale;
        r->maxSizeEnclosedHP2 *= _scale;
        r->maxSizeEnclosedWP1 *= _scale;
        r->maxSizeEnclosedWP2 *= _scale;

        calcBBox( r );
    }

    bool intersectLine2d( const RoomBSData *r, Vector2f const& p0, Vector2f const& p1,
                          Vector2f& /*i*/ ) {
        return r->bbox.lineIntersection( p0, p1 );
    }

    void calcBBox( RoomBSData *r ) {
        r->bbox = Rect2f::INVALID;

        for ( auto& ws : r->mWallSegments ) {
            for ( auto& ep : ws ) {
                r->bbox.expand( ep.p1 );
                r->bbox.expand( ep.p2 );
            }
        }
        r->bbox3d.calc( r->bbox, r->height, Matrix4f::IDENTITY );
        r->center = r->bbox.calcCentre();
    }

    Vector2f maxEnclsingBoundingBoxCenter( const RoomBSData *r ) {
        Vector2f lcenter = Vector2f::ZERO;
        for ( auto& v : r->mMaxEnclsingBoundingBox ) {
            lcenter += v;
        }

        return lcenter / static_cast<float>( r->mMaxEnclsingBoundingBox.size());
    }

    std::string roomName( const RoomBSData *r ) {
        return roomTypeToName( r->asType );
    }

    bool isGeneric( const RoomBSData *r ) {
        return r->asType == ASType::GenericRoom;
    }

    std::string roomTypeToName( ASTypeT ast ) {
        switch ( ast ) {
            case ASType::GenericRoom:
                return "generic";

            case ASType::LivingRoom:
                return "living-room";

            case ASType::Studio:
                return "studio";

            case ASType::Kitchen:
                return "kitchen";

            case ASType::BedroomSingle:
            case ASType::BedroomDouble:
                return "Bedroom";

            case ASType::BedroomMaster:
                return "Master-bedroom";

            case ASType::Bathroom:
                return "bathroom";

            case ASType::ShowerRoom:
                return "shower-room";

            case ASType::Ensuite:
                return "ensuite";

            case ASType::ToiletRoom:
                return "toilet";

            case ASType::DiningRoom:
                return "dining-room";

            case ASType::Conservatory:
                return "conservatory";

            case ASType::GamesRoom:
                return "gameroom";

            case ASType::Laundry:
                return "laundry";

            case ASType::Hallway:
                return "hallway";

            case ASType::Garage:
                return "garage";

            default:
                break;
        }

        return std::string{};
    }

    Vector4f roomColor( const RoomBSData *r ) {
        switch ( r->asType ) {
            case ASType::GenericRoom:
                return Color4f::SAND.A( 0.25f );

            case ASType::LivingRoom:
            case ASType::Studio:
                return Color4f::ALMOND.A( 0.25f );

            case ASType::Kitchen:
                return Color4f::PASTEL_GREEN;

            case ASType::BedroomSingle:
                return Color4f::PASTEL_BROWN;

            case ASType::BedroomDouble:
                return Color4f::PASTEL_BROWN;

            case ASType::BedroomMaster:
                return Color4f::PASTEL_BROWN;

            case ASType::Bathroom:
                return Color4f::PASTEL_CYAN;

            case ASType::ShowerRoom:
                return Color4f::PASTEL_CYAN;

            case ASType::Ensuite:
                return Color4f::PASTEL_CYAN;

            case ASType::ToiletRoom:
                return Color4f::PASTEL_CYAN;

            case ASType::Conservatory:
                return Color4f::PASTEL_GRAYLIGHT;

            case ASType::GamesRoom:
                return Color4f::PASTEL_RED;

            case ASType::Laundry:
                return Color4f::PASTEL_GRAYLIGHT;

            case ASType::Hallway:
                return Color4f::PASTEL_GRAYLIGHT;

            case ASType::Garage:
                return Color4f::PASTEL_GRAY;

            default:
                break;
        }

        return Color4f::WHITE;
    }

}
