//
//  room_service.cpp
//  sixthview
//
//  Created by Dado on 16/7/2017.
//
//

#include "room_service.hpp"

#include <core/util.h>
#include "core/math/triangulator.hpp"
#include "core/service_factory.h"
#include "arch_segment_service.hpp"
#include "arch_structural_service.hpp"
#include "wall_service.hpp"

namespace RoomService {

    std::shared_ptr<RoomBSData> createRoom( const RoomPreData& _preData, const float _floorHeight, const float _z, const HouseBSData* house ) {
        auto w = ServiceFactory::create<RoomBSData>();
        w->type = ArchType::RoomT;
        for ( const auto& rt: _preData.rtypes ) {
            RoomService::addRoomType(w.get(), rt, house);
        }
        w->mHasCoving = !RS::hasRoomType(w.get(), ASType::Kitchen);
        w->height = _floorHeight;
        w->width = _preData.bboxInternal.calcWidth();
        w->depth = _preData.bboxInternal.calcHeight();
        w->z = _z;

        return w;
    }

    void
    addSocketIfSafe( RoomBSData *r, const std::vector<Vector2f>& cov, size_t indexSkirting,
                     float safeSocketBoxWidth ) {
        if ( distance(cov[indexSkirting], cov[indexSkirting + 1]) > safeSocketBoxWidth ) {
            Vector2f snormal = normalize(cov[indexSkirting] - cov[indexSkirting + 1]);
            Vector2f sanePos = cov[indexSkirting] + ( -snormal * safeSocketBoxWidth * 0.5f );
            Vector2f saneSocketPos = cov[indexSkirting] + ( -snormal * safeSocketBoxWidth );
            float fa = furnitureAngleFromNormal(rotate90(snormal));
            r->mSocketLocators.emplace_back(saneSocketPos, fa);
            if ( indexSkirting == 0 ) {
                r->mSwichesLocators.emplace_back(sanePos, fa);
            }
        }
    }

    void calcOptimalLightingFittingPositions( RoomBSData *r ) {
        JMATH::Rect2f br(r->mMaxEnclsingBoundingBox);
        Vector2f bs = br.size();
        bool bCenterRoomOnly = false;

        r->mLightFittingsLocators.clear();
        float lightYOffset = r->defaultCeilingThickness;// + r->spotLightYOffset;

        int numX = static_cast<int>( bs.x() / r->minLightFittingDistance );
        int numY = static_cast<int>( bs.y() / r->minLightFittingDistance );

        if ( numX == 0 || numY == 0 || bCenterRoomOnly ) {
            r->mLightFittingsLocators.emplace_back(br.centre(), r->height - lightYOffset);
            return;
        }

        for ( auto t = 1; t < numY + 1; t++ ) {
            float ly = static_cast<float>( t ) / static_cast<float>( numY + 1 );
            float lvy = lerp(ly, br.bottom(), br.top());
            Vector2f lv1 = { br.left(), lvy };
            Vector2f lv2 = { br.right(), lvy };
            for ( auto m = 1; m < numX + 1; m++ ) {
                float lx = static_cast<float>( m ) / static_cast<float>( numX + 1 );
                Vector2f i = lerp(lx, lv1, lv2);
                r->mLightFittingsLocators.emplace_back(i, r->height - lightYOffset);
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
                addSocketIfSafe(r, cov, 0, safeSingleSocketBoxWidth);
            }
            if ( csize > 2 ) {
                addSocketIfSafe(r, cov, csize - 2, safeSingleSocketBoxWidth);
            }
        }
    }

    void calcSkirtingSegments( RoomBSData *r ) {
        // Skirting
        r->mvSkirtingSegments.clear();
        for ( auto& rws : r->mWallSegments ) {
            int csize = static_cast<int>( rws.size());
            Vector2f p1 = V2fc::HUGE_VALUE_POS;
            std::vector<Vector2f> points;
            int startIndex = 0;
            for ( int q = 0; q < csize; q++ ) {
                if ( checkBitWiseFlag(rws[cai(q - 1, csize)].tag, WallFlags::WF_IsDoorPart) &&
                     !checkBitWiseFlag(rws[q].tag, WallFlags::WF_IsDoorPart) ) {
                    startIndex = q;
                    break;
                }
            }
            for ( int q = startIndex; q < startIndex + csize; q++ ) {
                int qc = cai(q, csize);
                if ( checkBitWiseFlag(rws[qc].tag, WallFlags::WF_IsDoorPart ) ) {
                    continue;
                }
                int qcm1 = cai(q - 1, csize);
                int qcp1 = cai(q + 1, csize);
                if ( checkBitWiseFlag(rws[qcm1].tag, WallFlags::WF_IsDoorPart) ) {
                    Vector2f p2mp1 = rws[qc].p2 - rws[qc].p1;
                    if ( length(p2mp1) > r->mArchiTravesWidth ) {
                        p1 = rws[qc].p1 + ( normalize(p2mp1) * r->mArchiTravesWidth );
                        points.push_back(p1);
                    }
                }
                p1 = rws[qc].p2;
                points.push_back(p1);
                if ( checkBitWiseFlag(rws[qcp1].tag, WallFlags::WF_IsDoorPart) ) {
                    Vector2f p2mp1 = points[points.size() - 2] - p1;
                    if ( length(p2mp1) > r->mArchiTravesWidth ) {
                        p1 += normalize(p2mp1) * r->mArchiTravesWidth;
                        points[points.size() - 1] = p1;
                    } else {
                        points.pop_back();
                    }
                    //removeCollinear( points, accuracy1mm );
                    if ( points.size() >= 2 ) r->mvSkirtingSegments.push_back(points);
                    points.clear();
                }
            }
            if ( r->mvSkirtingSegments.size() == 0 && points.size() > 1 ) {
                removeCollinear(points, accuracy1Sqmm);
                if ( points.size() >= 2 ) r->mvSkirtingSegments.push_back(points);
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
            auto qp1 = r->mPerimeterSegments[getCircularArrayIndexUnsigned(q + 1, csize)];
            for ( size_t m = 0; m < csize; m++ ) {
                if ( m == q ) continue;
                auto mp = r->mPerimeterSegments[m];
                auto mp1 = r->mPerimeterSegments[getCircularArrayIndexUnsigned(m + 1, csize)];
                auto mpmiddle = lerp(0.5f, mp, mp1);
                auto mpn = normalize(mp1 - mp);
                auto mnr = rotate90(mpn) * 1000.0f;
                if ( intersection(qp, qp1, mpmiddle + mnr, mpmiddle - mnr, i) ) {
                    float d = distance(mpmiddle, i);
                    if ( ncheck.x() != V2fc::HUGE_VALUE_POS.x() ) {
                        float dn = fabs(dot(ncheck, normalize(mpmiddle - i)));
                        if ( dn > 0.95f ) continue;
                    }
                    float l = distance(qp, qp1) + distance(mp, mp1);
                    if ( ( d - SMALL_EPSILON > maxDist ) ||
                         ( ( d >= maxDist - SMALL_EPSILON ) && ( l > maxLengths ) ) ) {
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

    bool checkIncludeDoorsWindowsFlag( const ArchSegment *ls, IncludeWindowsOrDoors bwd ) {
        if ( bwd != IncludeWindowsOrDoors::Both ) {
            if ( checkBitWiseFlag(ls->tag, WallFlags::WF_IsDoorPart) ) {
                if ( ( bwd == IncludeWindowsOrDoors::None ) || ( bwd != IncludeWindowsOrDoors::DoorsOnly ) )
                    return false;
            }
            if ( checkBitWiseFlag(ls->tag, WallFlags::WF_IsWindowPart) ) {
                if ( bwd == IncludeWindowsOrDoors::None || bwd != IncludeWindowsOrDoors::WindowsOnly ) return false;
            }
        }
        return true;
    }

    void WallSegments( RoomBSData *r, const std::vector<std::vector<ArchSegment>>& val ) {
        r->mWallSegments = val;

        // Perimeter
        WallService::perimeterFromSegments(val, r->mPerimeterSegments, r->mPerimeter);

        // Find max room span points
        Vector2f ep1, ep2;
        Vector2f ncheck = V2fc::HUGE_VALUE_POS;
        bool bEncloseFound = checkMaxSizeEnclosure(r, ep1, ep2, ncheck);
        if ( bEncloseFound ) {
            r->maxSizeEnclosedHP1 = ep1;
            r->maxSizeEnclosedHP2 = ep2;
            ncheck = normalize(ep1 - ep2);
            bEncloseFound = checkMaxSizeEnclosure(r, ep1, ep2, ncheck);
            if ( bEncloseFound ) {
                r->maxSizeEnclosedWP1 = ep1;
                r->maxSizeEnclosedWP2 = ep2;
            }
        }

        // Coving
        r->mvCovingSegments = calcCovingSegments(r->mWallSegments);
        r->mCovingPerimeter = calculatePerimeterOf(r->mvCovingSegments);

        // Max bounding box
        // *** This has to be done AFTER coving calculations
        // as it uses the coving segment to determine a "sane" max bbox ***
        r->mBBoxCoving = getContainingBBox(r->mvCovingSegments);
        calclMaxBoundingBox(r);
        makeTriangles2d(r);
        r->area = RS::area(r);
    }

    void makeTriangles2d( RoomBSData *r ) {
        r->mTriangles2d.clear();
        Triangulator tri(r->mPerimeterSegments);
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
                size_t q1 = getCircularArrayIndexUnsigned(q + 1, csize);
                float dist = distance(covs[q], covs[q1]);
                if ( dist > maxDist ) {
                    v1 = covs[q];
                    v2 = covs[q1];
                    maxDist = dist;
                }
            }
        }

        // right we have the longest coving segment now, in v1, v2, now trace a ray from middle of v1,v2 to see where it hits
        Vector2f vn = normalize(rotate90(v2 - v1));
        Vector2f vm = lerp(0.5f, v1, v2);
        Vector2f vi1 = vm + vn * 10000.0f;
        Vector2f vi2 = vm - vn * 10000.0f;
        Vector2f vi;
        for ( auto& covs : r->mvCovingSegments ) {
            size_t csize = covs.size();
            for ( size_t q = 0; q < csize; q++ ) {
                size_t q1 = getCircularArrayIndexUnsigned(q + 1, csize);
                if ( covs[q] == v1 && covs[q1] == v2 ) continue;
                if ( intersection(vi1, vi2, covs[q], covs[q1], vi) ) {
                    float dist = distance(vm, vi);
                    if ( dist > VERY_SMALL_EPSILON && dist < minDist ) {
                        minDist = dist;
                        v3 = vi;
                    }
                }
            }
        }
        // Tentative bbox
        Vector2f vnnr = normalize(v2 - v1);
        vi1 = v3 + vnnr * 10000.0f;
        vi2 = v3 - vnnr * 10000.0f;

        intersection(v1 - vn * 10000.0f, v1 + vn * 10000.0f, vi1, vi2, v3);
        intersection(v2 - vn * 10000.0f, v2 + vn * 10000.0f, vi1, vi2, v4);

        r->mMaxEnclsingBoundingBox.clear();
        r->mMaxEnclsingBoundingBox.push_back(v1);
        r->mMaxEnclsingBoundingBox.push_back(v2);
        r->mMaxEnclsingBoundingBox.push_back(v3);
        r->mMaxEnclsingBoundingBox.push_back(v4);
    }

    bool findOppositeWallFromPointAllowingGap( const RoomBSData *r, const Vector2f& p1, const Vector2f& normal,
                                               std::pair<size_t, size_t>& ret, Vector2f& iPoint,
                                               IncludeWindowsOrDoors bwd, float allowedGap ) {
        V2f i{};
        float minDist = std::numeric_limits<float>::max();
        for ( auto t = 0u; t < r->mWallSegments.size(); t++ ) {
            for ( auto m = 0u; m < r->mWallSegments[t].size(); m++ ) {
                Vector2f normalOpposite = r->mWallSegments[t][m].normal;
                if ( isScalarEqual(dot(normalOpposite, normal), -1.0f) ) {
                    if ( intersection(r->mWallSegments[t][m].p1, r->mWallSegments[t][m].p2, p1,
                                      p1 + ( normal * 1000.0f ), i) ) {
                        float dist = distance(p1, i);
                        if ( dist < minDist ) {
                            if ( bwd != IncludeWindowsOrDoors::Both ) {
                                if ( checkBitWiseFlag(r->mWallSegments[t][m].tag, WallFlags::WF_IsDoorPart) ) {
                                    if ( ( bwd == IncludeWindowsOrDoors::None ) ||
                                         ( bwd != IncludeWindowsOrDoors::DoorsOnly ) ) {
                                        if ( dist < allowedGap ) continue;
                                    }
                                }
                                if ( checkBitWiseFlag(r->mWallSegments[t][m].tag, WallFlags::WF_IsWindowPart) ) {
                                    if ( bwd == IncludeWindowsOrDoors::None || bwd != IncludeWindowsOrDoors::WindowsOnly ) {
                                        if ( dist < allowedGap ) continue;
                                    }
                                }
                            }

                            ret = std::make_pair(t, m);
                            iPoint = i;
                            minDist = dist;
                        }
                    }
                }
            }
        }
        return minDist != std::numeric_limits<float>::max();
    }

    bool findOppositeWallFromPoint( const RoomBSData *r, const Vector2f& p1, const Vector2f& normal,
                                    std::pair<size_t, size_t>& ret, Vector2f& iPoint, IncludeWindowsOrDoors bwd ) {
        V2f i{};
        float minDist = std::numeric_limits<float>::max();
        for ( auto t = 0u; t < r->mWallSegments.size(); t++ ) {
            for ( auto m = 0u; m < r->mWallSegments[t].size(); m++ ) {
                if ( bwd != IncludeWindowsOrDoors::Both ) {
                    if ( checkBitWiseFlag(r->mWallSegments[t][m].tag, WallFlags::WF_IsDoorPart) ) {
                        if ( ( bwd == IncludeWindowsOrDoors::None ) ||
                             ( bwd != IncludeWindowsOrDoors::DoorsOnly ) )
                            continue;
                    }
                    if ( checkBitWiseFlag(r->mWallSegments[t][m].tag, WallFlags::WF_IsWindowPart) ) {
                        if ( bwd == IncludeWindowsOrDoors::None || bwd != IncludeWindowsOrDoors::WindowsOnly ) continue;
                    }
                }
                Vector2f normalOpposite = r->mWallSegments[t][m].normal;
                if ( isScalarEqual(dot(normalOpposite, normal), -1.0f) ) {
                    if ( intersection(r->mWallSegments[t][m].p1, r->mWallSegments[t][m].p2, p1,
                                      p1 + ( normal * 1000.0f ), i) ) {
                        float dist = distance(p1, i);
                        if ( dist < minDist ) {
                            ret = std::make_pair(t, m);
                            iPoint = i;
                            minDist = dist;
                        }
                    }
                }
            }
        }
        return minDist != std::numeric_limits<float>::max();
    }

    void updateFromArchSegments( RoomBSData *r,
                                 const std::vector<std::vector<ArchSegment>>& ws ) {
        WallSegments(r, ws);
        calcLongestWall(r);
        calcBBox(r);
    }

    std::vector<std::vector<Vector2f>>
    calcCovingSegments( const std::vector<std::vector<ArchSegment>>& ws ) {
        std::vector<std::vector<Vector2f>> lCovingSegments;

        for ( auto& rws : ws ) {
            std::vector<Vector2f> points;
            int csize = static_cast<int>( rws.size());
            points.reserve(csize);
            for ( int q = 0; q < csize; q++ ) {
                points.push_back(rws[q].p2);
            }
            removeCollinear(points, 0.0001f);
            lCovingSegments.push_back(points);
        }

        return lCovingSegments;
    }

    float skirtingDepth( const RoomBSData *r ) {
//	Profile skirtingProfile{ "skirting_kensington" };
//	return skirtingProfile.width();
        // TODO: Re-enable profiles
        return roomNeedsCoving(r) ? 0.02f : 0.0f;
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
        return !( RS::hasRoomType(r, ASType::Bathroom) || RS::hasRoomType(r, ASType::ToiletRoom) ||
                  RS::hasRoomType(r, ASType::ShowerRoom) ||
                  RS::hasRoomType(r, ASType::Ensuite) );
    }

    float area( const RoomBSData *r ) {
        float ret = 0.0f;
        for ( auto& vts : r->mTriangles2d ) {
            auto a = std::get<0>(vts);
            auto b = std::get<1>(vts);
            auto c = std::get<2>(vts);
            ret += fabs(( a.x() - c.x() ) * ( b.y() - a.y() ) - ( a.x() - b.x() ) * ( c.y() - a.y() )) / 2;
        }
        return ret;
    }

    bool isPointInsideRoom( const RoomBSData *r, const V2f& point ) {

        if ( r->bbox.contains(point) ) {
            for ( auto& vts : r->mTriangles2d ) {
                auto a = std::get<0>(vts);
                auto b = std::get<1>(vts);
                auto c = std::get<2>(vts);
                if ( isInsideTriangle(point, a, b, c) ) {
                    return true;
                }
            }

        }

        return false;
    }

    roomTypeIndex sortedSegmentToPairIndex( const RoomBSData *r, int si ) {
        roomTypeIndex rti = std::make_pair(-1, -1);

        for ( auto t = 0u; t < r->mWallSegments.size(); t++ ) {
            for ( auto m = 0u; m < r->mWallSegments[t].size(); m++ ) {
                if ( r->mWallSegments[t][m].p1 == r->mWallSegmentsSorted[si].p1 &&
                     r->mWallSegments[t][m].p2 == r->mWallSegmentsSorted[si].p2 ) {
                    rti = std::make_pair(t, m);
                    return rti;
                }
            }
        }
        return rti;
    }

    roomTypeIndex sortedSegmentToPairIndex( const RoomBSData *r, const ArchSegment *ls ) {
        roomTypeIndex rti = std::make_pair(-1, -1);

        for ( auto t = 0u; t < r->mWallSegments.size(); t++ ) {
            for ( auto m = 0u; m < r->mWallSegments[t].size(); m++ ) {
                if ( r->mWallSegments[t][m].p1 == ls->p1 &&
                     r->mWallSegments[t][m].p2 == ls->p2 ) {
                    rti = std::make_pair(t, m);
                    return rti;
                }
            }
        }
        return rti;
    }

    void rescale( RoomBSData *r, float _scale ) {
        ArchStructuralService::rescale(r, _scale);

        Vector3f scale3f = { _scale, _scale, 1.0f };
        for ( auto& covs : r->mWallSegments ) {
            for ( auto& s : covs ) {
                ArchSegmentService::rescale(s, _scale);
            }
        }
        for ( auto& s : r->mWallSegmentsSorted ) {
            ArchSegmentService::rescale(s, _scale);
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
        r->mBBoxCoving *= _scale;

        r->mLongestWallOppositePoint *= _scale;

        r->maxSizeEnclosedHP1 *= _scale;
        r->maxSizeEnclosedHP2 *= _scale;
        r->maxSizeEnclosedWP1 *= _scale;
        r->maxSizeEnclosedWP2 *= _scale;

        calcBBox(r);

        r->area = RoomService::area(r);
    }

    bool intersectLine2d( const RoomBSData *r, Vector2f const& p0, Vector2f const& p1,
                          Vector2f& /*i*/ ) {
        return r->bbox.lineIntersection(p0, p1);
    }

    void calcBBox( RoomBSData *r ) {
        r->bbox = Rect2f::INVALID;

        for ( auto& ws : r->mWallSegments ) {
            for ( auto& ep : ws ) {
                r->bbox.expand(ep.p1);
                r->bbox.expand(ep.p2);
            }
        }
        r->bbox3d.calc(r->bbox, r->height, Matrix4f::IDENTITY);
        r->center = r->bbox.calcCentre();
    }

    Vector2f maxEnclsingBoundingBoxCenter( const RoomBSData *r ) {
        Vector2f lcenter = V2fc::ZERO;
        for ( auto& v : r->mMaxEnclsingBoundingBox ) {
            lcenter += v;
        }

        return lcenter / static_cast<float>( r->mMaxEnclsingBoundingBox.size());
    }

    std::optional<uint64_t> findArchSegmentWithWallHash( RoomBSData *r, HashEH hashToFind, int64_t index ) {
        for ( uint64_t t = 0; t < r->mWallSegmentsSorted.size(); t++ ) {
            auto seg = r->mWallSegmentsSorted[t];
            if ( seg.wallHash == hashToFind && seg.iIndex == index ) {
                return t;
            }
        }
        return std::nullopt;
    }

    std::string roomName( const RoomBSData *r ) {
        return roomTypeToName(r->roomTypes[0]);
    }

    std::string roomNames( const RoomBSData *r ) {
        std::string ret = roomTypeToName(r->roomTypes[0]);
        for ( auto i = 1UL; i < r->roomTypes.size(); i++ ) {
            ret += " / " + roomTypeToName(r->roomTypes[i]);
        }
        return ret;
    }

    void setRoomType( RoomBSData *r, ASTypeT rt, const HouseBSData* house ) {
        r->roomTypes.clear();
        r->roomTypes.emplace_back(rt);
        RoomService::assignDefaultRoomFeaturesForType(r, rt, house);
    }

    void addRoomType( RoomBSData *r, ASTypeT rt, const HouseBSData* house ) {
        if ( auto it = std::find(r->roomTypes.begin(), r->roomTypes.end(), rt); it == r->roomTypes.end() ) {
            if ( r->roomTypes.empty() ) {
                RoomService::setRoomType(r, rt, house);
            } else {
                r->roomTypes.emplace_back(rt);
            }
        }
    }

    void removeRoomType( RoomBSData *r, ASTypeT rt ) {
        erase_if(r->roomTypes, [rt]( const auto& lrt ) -> bool { return lrt == rt; });
    }

    bool isGeneric( const RoomBSData *r ) {
        return r->roomTypes.size() == 1 && r->roomTypes[0] == ASType::GenericRoom;
    }

    void assignDefaultRoomFeaturesForType( RoomBSData *r, ASTypeT ast, const HouseBSData* house ) {
        switch ( ast ) {
            case ASType::GenericRoom:
            case ASType::LivingRoom:
            case ASType::Studio:
            case ASType::DiningRoom:
            case ASType::Conservatory:
            case ASType::GamesRoom:
            case ASType::Laundry:
            case ASType::Hallway:
            case ASType::Garage:
            case ASType::Cupboard:
            case ASType::Storage:
            case ASType::BoilerRoom:
            return;
            case ASType::Kitchen:
                r->floorMaterial = house->defaultKitchenFloorMaterial;
                return;

            case ASType::BedroomSingle:
            case ASType::BedroomDouble:
            case ASType::BedroomMaster:
                r->floorMaterial = house->defaultBedroomFloorMaterial;
                return;

            case ASType::Bathroom:
            case ASType::ShowerRoom:
            case ASType::Ensuite:
            case ASType::ToiletRoom:
                r->floorMaterial = house->defaultBathroomFloorMaterial;
                r->wallsMaterial = house->defaultBathroomWallMaterial;
                return;

            default:
                break;
        }
    }

    std::string roomTypeToName( ASTypeT ast ) {
        switch ( ast ) {
            case ASType::GenericRoom:
                return "Room";

            case ASType::LivingRoom:
                return "Living Room";

            case ASType::Studio:
                return "Studio";

            case ASType::Kitchen:
                return "Kitchen";

            case ASType::BedroomSingle:
            case ASType::BedroomDouble:
                return "Bedroom";

            case ASType::BedroomMaster:
                return "Master Bedroom";

            case ASType::Bathroom:
                return "Bathroom";

            case ASType::ShowerRoom:
                return "Shower Room";

            case ASType::Ensuite:
                return "En Suite";

            case ASType::ToiletRoom:
                return "Toilet";

            case ASType::DiningRoom:
                return "Dining Room";

            case ASType::Conservatory:
                return "Conservatory";

            case ASType::GamesRoom:
                return "Games Room";

            case ASType::Laundry:
                return "Laundry";

            case ASType::Hallway:
                return "Hallway";

            case ASType::Garage:
                return "Garage";

            case ASType::Cupboard:
                return "Cupboard";

            case ASType::Storage:
                return "Storage";

            case ASType::BoilerRoom:
                return "BoilerRoom";

            default:
                break;
        }

        return std::string{};
    }

    std::string roomTypeToName1to1( ASTypeT ast ) {
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
                return "single bedroom";
            case ASType::BedroomDouble:
                return "double bedroom";

            case ASType::BedroomMaster:
                return "master-bedroom";

            case ASType::Bathroom:
                return "bathroom";

            case ASType::ShowerRoom:
                return "shower-room";

            case ASType::Ensuite:
                return "en-suite";

            case ASType::ToiletRoom:
                return "toilet";

            case ASType::DiningRoom:
                return "dining-room";

            case ASType::Conservatory:
                return "conservatory";

            case ASType::GamesRoom:
                return "game-room";

            case ASType::Laundry:
                return "laundry";

            case ASType::Hallway:
                return "hallway";

            case ASType::Garage:
                return "garage";

            case ASType::Cupboard:
                return "cupboard";

            case ASType::Storage:
                return "storage";

            case ASType::BoilerRoom:
                return "boilerRoom";

            default:
                break;
        }

        return std::string{};
    }

    Vector4f roomColor( const RoomBSData *r ) {
        switch ( r->roomTypes[0] ) {
            case ASType::GenericRoom:
                return Color4f::SAND;

            case ASType::LivingRoom:
            case ASType::Studio:
                return Color4f::ALMOND;

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

            case ASType::Cupboard:
                return Color4f::DARK_CYAN;

            case ASType::Storage:
                return Color4f::ORANGE_SCHEME1_1;

            case ASType::BoilerRoom:
                return C4f::DARK_BLUE;

            default:
                break;
        }

        return Color4f::WHITE;
    }

    std::string roomSizeToString( const RoomBSData *r ) {
        if ( r->bbox.width() > r->bbox.height() ) {
            return sizeToStringMeters(r->bbox.width(), r->bbox.height());
        }
        return sizeToStringMeters(r->bbox.height(), r->bbox.width());
    }

    FittedFurniture *findFurniture( RoomBSData *r, HashEH furnitureHash ) {
        for ( auto& ff : r->mFittedFurniture ) {
            if ( ff->hash == furnitureHash ) {
                return ff.get();
            }
        }
        return nullptr;
    }

    void changeWallsMaterial( RoomBSData *r, MaterialAndColorProperty& mcp ) {
        for ( auto& ws : r->mWallSegmentsSorted ) {
            ws.wallMaterial = mcp;
        }
    }

}
