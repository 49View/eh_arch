//
//  wall_service.cpp
//  sixthview
//
//  Created by Dado on 05/10/2015.
//
//

#include "wall_service.hpp"

#include "arch_structural_service.hpp"
#include "ushape_service.hpp"
#include "arch_segment_service.hpp"

#include "core/math/triangulator.hpp"

std::shared_ptr<WallBSData> WallService::createWall2( const std::vector<Vector2f>& epts,
                                                      float _height,
                                                      WallLastPointWrapT wlpw,
                                                      float _z,
                                                      uint32_t wf,
                                                      int64_t _linkedHash,
                                                      SequencePart sequencePart ) {

    auto w = std::make_shared<WallBSData>();
    w->asType = ASType::Wall;
    w->type = ArchType::WallT;
    w->wrapLastPoint = wlpw;
    w->linkedHash = _linkedHash;
    w->sequencePart = sequencePart;
    w->z = _z;
    w->wallFlags = wf;
    w->height = _height;

    WallService::update( w.get(), epts );

    return w;
}

void WallService::update( WallBSData *w ) {
    removeCollinear( w->epoints, accuracy1Sqmm );
    makeTriangles2d( w );

    // Normals
    int csize = static_cast<int>( w->epoints.size());
    w->enormals.resize( csize );
    w->slinesGHType.resize( csize );
    for ( auto t = 0; t < csize; t++ ) {
        // This small if statement is needed in order to guarantee the normal facing the same way in case of a 2 vertex wall
        // Without it it will assign 2 normals (the second being the inverted of the first) and that's wrong because for
        // a simple 2 vertex line there's only 1 normal
        if ( !( csize == 2 && t == 1 )) {
            w->enormals[t] = normalize( rotate90( w->epoints[t] - w->epoints[cai( t + 1, csize )] ));
        } else {
            w->enormals[t] = w->enormals.back();
        }
        ASSERT( isValid( w->enormals[t].x()));
        w->slinesGHType[t] = static_cast<uint64_t>( GHType::WallPlaster );
    }

    calcBBox( w );
    w->width = w->bbox.width();
    WallService::updateUShapes( w );
}

void WallService::update( WallBSData *w, const std::vector<Vector2f>& epts ) {
    w->epoints = epts;
    update( w );
}


std::shared_ptr<WallBSData> WallService::createWall( const std::vector<Vector2f>& epts,
                                                     float _height,
                                                     WallLastPointWrapT wlpw,
                                                     float _z,
                                                     uint32_t wf,
                                                     const HouseHints::WallHints& uShapesHints ) {

    auto w = std::make_shared<WallBSData>();
    w->asType = ASType::Wall;
    w->type = ArchType::WallT;
    w->wrapLastPoint = wlpw;
    w->epoints = epts;

    removeCollinear( w->epoints, accuracy1Sqmm );
    makeTriangles2d( w.get());

    // Normals
    int csize = static_cast<int>( w->epoints.size());
    for ( auto t = 0; t < csize; t++ ) {
        w->enormals.push_back(
                normalize( rotate90( w->epoints[t] - w->epoints[getCircularArrayIndex( t + 1, csize )] )));
        ASSERT( isValid( w->enormals[t].x()));
        w->slinesGHType.push_back( static_cast<uint64_t>( GHType::WallPlaster ));
    }

    w->z = _z;
    w->wallFlags = wf;
    calcBBox( w.get());
    w->width = w->bbox.width();
    w->height = _height;
    updateUShapes( w.get());

    return w;
}

bool isInsideCeilingContour( const std::vector<std::vector<Vector3f>>& cd, const Vector2f& v1, float& topZ1,
                             int& hitLevel1 ) {

    bool bFoundAtLeastOneContour = false;
    for ( size_t level = 0; level < cd.size(); level++ ) {
        auto& vv = cd[level];
        Rect2f r = Rect2f::INVALID;
        for ( auto& v : vv ) r.expand( v.xy());
        if ( r.contains( v1 )) {
            topZ1 = vv[0].z();
            hitLevel1 = level + 1;
            bFoundAtLeastOneContour = true;
        }
    }

    return bFoundAtLeastOneContour;
}

void WallService::updateUShapes( WallBSData *w ) {

    w->mUShapes.clear();
    if ( w->epoints.size() < 4 ) return;

    // Calculate median width
    std::vector<float> medianWidthV;
    std::vector<UShape> tempUShapes;

    for ( auto t = 0u; t < w->epoints.size(); t++ ) {
        UShape us;
        us.type = ArchType::WallT;
        for ( auto m = 0; m < 4; m++ ) {
            us.indices[m] = static_cast<int32_t>( getCircularArrayIndex( static_cast<int32_t>( t + m ),
                                                                         static_cast<int32_t>( w->epoints.size())));
            us.points[m] = w->epoints[us.indices[m]];
        }

        us.edges[0] = normalize( us.points[1] - us.points[0] );
        us.edges[1] = normalize( us.points[2] - us.points[1] );
        us.edges[2] = normalize( us.points[3] - us.points[2] );

        float d1 = dot( us.edges[0], us.edges[1] );
        float d2 = dot( us.edges[1], us.edges[2] );
        float d3 = dot( us.edges[0], us.edges[2] );

        UShapeService::elaborateData( us );

        tempUShapes.push_back( us );

        float usw = distance( us.points[1], us.points[2] );
        if ( distance( d1, d2 ) < 0.1f && d3 < 0.0f ) {
            medianWidthV.push_back( static_cast<float>(usw));
        }
    }

    float medianWallWidth = medianWidthV.empty() ? 0.0f : medianSort( medianWidthV );

    for ( auto t = 0u; t < w->epoints.size(); t++ ) {
        UShape us = tempUShapes[t];

        float d1 = dot( us.edges[0], us.edges[1] );
        float d2 = dot( us.edges[1], us.edges[2] );
        float d3 = dot( us.edges[0], us.edges[2] );

        if ( UShapeService::isUShapeConvex( us ) && distance( d1, d2 ) < 0.1f && d3 < 0.0f &&
             us.width <= medianWallWidth * 3.0f ) {
            w->mUShapes.push_back( us );
        }
    }
}

void WallService::removeUnPairedUShapes( WallBSData *w ) {
    erase_if( w->mUShapes, []( const auto& us ) -> bool { return !us.mIsPaired; } );
}

struct WallSegmentCrossingData {
    Vector2f pi;
    int indexZ1;
    int indexZ2;
    float z1;
    float z2;
};

bool addVertexFromWall( const std::vector<Vector3f>& vv, const Vector3f& v1, Vector3f& v3 ) {
    float minDist = std::numeric_limits<float>::max();
    Vector2f pi = Vector2f::ZERO;
    float dfl = 0.0f;
    bool bFoundHit = false;
    for ( size_t q = 0; q < vv.size(); q++ ) {
        dfl = distanceFromLine( v1.xy(), vv[q].xy(), vv[getCircularArrayIndexUnsigned( q + 1, vv.size())].xy(), pi );
        if ( dfl < minDist ) {
            minDist = dfl;
            v3 = { pi, vv[0].z() };
            bFoundHit = true;
        }
    }
    ASSERT( bFoundHit );
    return bFoundHit;
}

bool checkTraspassingSegments( const std::vector<std::vector<Vector3f>>& cd, const Vector3f& v1, Vector3f& v2,
                               WallSegmentCrossingData& wscd ) {
    Vector2f pi;
    for ( auto level = 0u; level < cd.size(); level++ ) {
        auto vv = cd[level];
        for ( size_t x = 0; x < vv.size(); x++ ) {
            bool isIntersecting = intersection( v1.xz(), v2.xz(), vv[x].xz(), vv[getCircularArrayIndexUnsigned( x +
                                                                                                                1,
                                                                                                                vv.size())].xz(),
                                                pi );
            if ( isIntersecting ) {
                wscd.pi = pi;
                isInsideCeilingContour( cd, v1.xz(), wscd.z1, wscd.indexZ1 );
                isInsideCeilingContour( cd, v2.xz(), wscd.z2, wscd.indexZ2 );
                return true;
            }
        }
    }
    return false;
}

//std::vector<Vector3f> WallService::vertsForWallAt( WallBSData* w, int t, const std::vector<std::vector<Vector3f>>& cd ) {
//	std::vector<Vector3f> _verts;
//
//	float topZ1 = w->height;
//	float topZ2 = w->height;
//	Vector3f v1 = { w->epoints[t], w->z };
//	Vector3f v2 = { w->epoints[getCircularArrayIndex( t + 1, static_cast<int>( w->epoints.size() ) )], w->z };
//
//	Vector3f v3 = Vector3f::ZERO;
//	Vector3f v4 = Vector3f::ZERO;
//	auto hitLevel1 = 0;
//	auto hitLevel2 = 0;
//	isInsideCeilingContour( cd, v1.xy(), topZ1, hitLevel1 );
//	isInsideCeilingContour( cd, v2.xy(), topZ2, hitLevel2 );
//	float topZmin1 = min( topZ1, w->height );
//	float topZmin2 = min( topZ2, w->height );
//	float minCommonZ = min( topZmin1, topZmin2 );
//	v3 = v1 + Vector3f::Z_AXIS * minCommonZ;
//	v4 = v2 + Vector3f::Z_AXIS * minCommonZ;
//	_verts.push_back( v1 );
//	_verts.push_back( v2 );
//	_verts.push_back( v4 );
//	_verts.push_back( v3 );
//
//	bool bTraspassing = false;
//	WallSegmentCrossingData wscd;
//	if ( checkTraspassingSegments( cd, v1, v2, wscd ) ) {
//		bTraspassing = true;
//	}
//
//	auto hitLevel = max( hitLevel1, hitLevel2 );
//
//	if ( minCommonZ < w->height ) {
//		if ( bTraspassing ) {
//			v1 = v3;
//			v2 = v4;
//			if ( wscd.z1 > wscd.z2 ) {
//				addVertexFromWall( cd[wscd.indexZ2], v2, v4 );
//				v3 = { v1.xy(), wscd.z1 };
//			} else {
//				addVertexFromWall( cd[wscd.indexZ1], v1, v3 );
//				v4 = { v2.xy(), wscd.z2 };
//			}
//			_verts.push_back( v1 );
//			_verts.push_back( v2 );
//			_verts.push_back( v4 );
//			_verts.push_back( v3 );
//		} else {
//			for ( ; hitLevel < cd.size(); hitLevel++ ) {
//				auto& vv = cd[hitLevel];
//				v1 = v3;
//				v2 = v4;
//				if ( v1.z() < w->height ) {
//					addVertexFromWall( vv, v1, v3 );
//				} else {
//					v3 = { v1.xy(), vv[0].z() };
//				}
//				if ( v2.z() < w->height ) {
//					addVertexFromWall( vv, v2, v4 );
//				} else {
//					v4 = { v2.xy(), vv[0].z() };
//				}
//				_verts.push_back( v1 );
//				_verts.push_back( v2 );
//				_verts.push_back( v4 );
//				_verts.push_back( v3 );
//			}
//		}
//	}
//
//	return _verts;
//}

QuadVector3fList WallService::vertsForWallAt( const WallBSData *w, int t,
                                              const std::vector<std::vector<Vector3f>>& cd ) {
    QuadVector3fList ret;

    float topZ1 = w->height;
    float topZ2 = w->height;
    Vector3f v1 = XZY::C( w->epoints[t], w->z );
    Vector3f v2 = XZY::C( w->epoints[getCircularArrayIndex( t + 1, static_cast<int>( w->epoints.size()))], w->z );

    Vector3f v3 = Vector3f::ZERO;
    Vector3f v4 = Vector3f::ZERO;
    auto hitLevel1 = 0;
    auto hitLevel2 = 0;
    isInsideCeilingContour( cd, v1.xz(), topZ1, hitLevel1 );
    isInsideCeilingContour( cd, v2.xz(), topZ2, hitLevel2 );
    float topZmin1 = min( topZ1, w->height );
    float topZmin2 = min( topZ2, w->height );
    float minCommonZ = min( topZmin1, topZmin2 );
    v3 = v1 + Vector3f::UP_AXIS * minCommonZ;
    v4 = v2 + Vector3f::UP_AXIS * minCommonZ;
    ret.push_back( {{ v1, v2, v4, v3 }} );


    bool bTraspassing = false;
    WallSegmentCrossingData wscd{};
    if ( checkTraspassingSegments( cd, v1, v2, wscd )) {
        bTraspassing = true;
    }

    size_t hitLevel = max( hitLevel1, hitLevel2 );

    if ( minCommonZ < w->height ) {
        if ( bTraspassing ) {
            v1 = v3;
            v2 = v4;
            if ( wscd.z1 > wscd.z2 ) {
                addVertexFromWall( cd[wscd.indexZ2], v2, v4 );
                v3 = { v1.xy(), wscd.z1 };
            } else {
                addVertexFromWall( cd[wscd.indexZ1], v1, v3 );
                v4 = { v2.xy(), wscd.z2 };
            }
            ret.push_back( {{ v1, v2, v4, v3 }} );
        } else {
            for ( ; hitLevel < cd.size(); hitLevel++ ) {
                auto& vv = cd[hitLevel];
                v1 = v3;
                v2 = v4;
                if ( v1.z() < w->height ) {
                    addVertexFromWall( vv, v1, v3 );
                } else {
                    v3 = { v1.xy(), vv[0].z() };
                }
                if ( v2.z() < w->height ) {
                    addVertexFromWall( vv, v2, v4 );
                } else {
                    v4 = { v2.xy(), vv[0].z() };
                }
                ret.push_back( {{ v1, v2, v4, v3 }} );
            }
        }
    }

    return ret;
}

bool WallService::checkUShapeIndexStartIsDoorOrWindow( const WallBSData *w, size_t index ) {
    for ( auto& us : w->mUShapes ) {
        if ( us.indices[1] == static_cast<int32_t>(index) &&
             ( us.type == ArchType::DoorT || us.type == ArchType::WindowT ))
            return true;
    }
    return false;
}

void WallService::makeTriangles2d( WallBSData *w ) {
    w->mTriangles2d.clear();
    // Check they are not self-intersecting
    size_t csize = w->epoints.size();
    Vector2f pi = Vector2f::ZERO;
    bool bNonPolyLine = false;
    for ( size_t t = 0; t < csize; t++ ) {
        Vector2f p1 = w->epoints[getCircularArrayIndexUnsigned( t, csize )];
        Vector2f p2 = w->epoints[getCircularArrayIndexUnsigned( t + 1, csize )];
        for ( size_t m = t + 2; m < csize + t - 2; m++ ) {
            if ( t != m ) {
                Vector2f p3 = w->epoints[getCircularArrayIndexUnsigned( m, csize )];
                Vector2f p4 = w->epoints[getCircularArrayIndexUnsigned( m + 1, csize )];
                if ( intersection( p1, p2, p3, p4, pi )) {
                    bNonPolyLine = true;
                    break;
                }
            }
        }
        if ( bNonPolyLine ) break;
    }
    if ( !bNonPolyLine ) {
        Triangulator tri( w->epoints, 0.000001f );
        w->mTriangles2d = tri.get2dTrianglesTuple();
    }
}

void WallService::rescale( WallBSData *w, float _scale ) {
    ArchStructuralService::rescale( w, _scale );

    for ( auto& s : w->epoints ) {
        s *= _scale;
    }
    for ( auto& s : w->mUShapes ) {
        UShapeService::rescale( s, _scale );
    }

    calcBBox( w );
}

float WallService::segmentLenght( const WallBSData *w, size_t index ) {
    ASSERT( index < w->epoints.size());
    return distance( w->epoints[index], w->epoints[getCircularArrayIndexUnsigned( index + 1, w->epoints.size())] );
}

void WallService::getPlasterMiddlePoints( const WallBSData *w, std::vector<Vector3f>& mpoints ) {
    int csize = static_cast<int>( w->epoints.size());
    for ( auto t = 0; t < csize; t++ ) {
        mpoints.push_back( middlePointAt( w, t ));
    }
}

void WallService::getSegmentUShapePoint( const WallBSData *w, const int index,
                                         Vector2f& us1, Vector2f& us2, Vector2f& usm, Vector2f& usn, const float off ) {
    Vector2f n1 = -w->mUShapes[0].edges[1];
    Vector2f n1Off = n1 * off;
    Vector2f n2 = w->mUShapes[0].edges[1];
    Vector2f n2Off = n2 * off;
    switch ( index ) {
        case 0:
            us1 = w->mUShapes[0].points[1];
            us2 = w->mUShapes[0].middle + n1Off;
            usn = -w->mUShapes[0].edges[0];
            break;
        case 1:
            usn = n1;
            us1 = w->mUShapes[0].middle + n1Off;
            us2 = w->mUShapes[1].middle + n1Off;
            break;
        case 2:
            us1 = w->mUShapes[1].middle + n1Off;
            us2 = w->mUShapes[1].points[2];
            usn = -w->mUShapes[1].edges[0];
            break;

        case 3:
            us1 = w->mUShapes[0].points[2];
            us2 = w->mUShapes[0].middle + n2Off;
            usn = w->mUShapes[0].edges[2];
            break;
        case 4:
            usn = n2;
            us1 = w->mUShapes[0].middle + n2Off;
            us2 = w->mUShapes[1].middle + n2Off;
            break;
        case 5:
            us1 = w->mUShapes[1].middle + n2Off;
            us2 = w->mUShapes[1].points[1];
            usn = w->mUShapes[1].edges[2];
            break;

        default:
            ASSERT( 0 );
    }
    usm = lerp( 0.5f, us1, us2 );
}

bool WallService::checkIndexAreInMiddleAnyUSHape( const WallBSData *w, int i1, int i2 ) {
    for ( auto& us : w->mUShapes ) {
        if ( us.type == ArchType::DoorT || us.type == ArchType::WindowT ) {
            if (( isVerySimilar( w->epoints[i1], us.points[1] ) || isVerySimilar( w->epoints[i1], us.points[2] )) &&
                ( isVerySimilar( w->epoints[i2], us.points[1] ) || isVerySimilar( w->epoints[i2], us.points[2] ))) {
                return true;
            }
        }
    }
    return false;
}

Vector3f WallService::middlePointAt( const WallBSData *w, size_t index ) {
    ASSERT( index < w->epoints.size());
    return { lerp( 0.5f, w->epoints[index], w->epoints[getCircularArrayIndexUnsigned( index + 1, w->epoints.size())] ),
             w->height * 0.5f };
}

void WallService::addToArchSegment( const WallBSData *w, const int32_t floorNumber, const int32_t wallNumber,
                                    std::vector<ArchSegment>& ws ) {
    int csize = static_cast<int>( w->epoints.size());

    for ( auto t = 0; t < csize - ( w->wrapLastPoint == WallLastPointWrap::No ); t++ ) {
        int t1 = getCircularArrayIndex( t + 1, csize );
        if ( !checkIndexAreInMiddleAnyUSHape( w, t, t1 )) {
            ws.push_back( ArchSegmentService::createArchSegment( floorNumber, wallNumber, t, w->hash, w->epoints[t],
                                                                 w->epoints[t1], middlePointAt( w, t ).xy(),
                                                                 w->enormals[t], w->wallFlags, w->sequencePart ));
        }
    }
}

void WallService::addToArchSegmentInternal( const WallBSData *w, const int32_t floorNumber, const int32_t wallNumber,
                                            std::vector<ArchSegment>& ws ) {
    int csize = static_cast<int>( w->epoints.size());

    for ( auto t = 0; t < csize - ( w->wrapLastPoint == WallLastPointWrap::No ); t++ ) {
        int t1 = getCircularArrayIndex( t + 1, csize );
        if ( !checkBitWiseFlag( w->slinesGHType[t], GHType::WallPlasterExternal )) {
            ws.push_back( ArchSegmentService::createArchSegment( floorNumber, wallNumber, t, w->hash, w->epoints[t],
                                                                 w->epoints[t1], middlePointAt( w, t ).xy(),
                                                                 w->enormals[t], w->wallFlags, w->sequencePart ));
        }
    }
}

void WallService::getArchSegments( const WallBSData *w, const int32_t floorNumber, const int32_t wallNumber,
                                   std::vector<ArchSegment>& ws ) {
    // If it's a window or door we shall de-couple half of the ushape's volume and slip into two walls-segments in order to allow half space each side of the door/window.
    addToArchSegmentInternal( w, floorNumber, wallNumber, ws );
    return;
//	bool madness = true;
//	if ( !madness && checkBitWiseFlag( w->wallFlags, WallFlags::WF_IsDoorPart ) && w->mUShapes.size() == 2 ) {
//		// First half
//		Vector2f us1 = Vector2f::ZERO;
//		Vector2f us2 = Vector2f::ZERO;
//		Vector2f usm = Vector2f::ZERO;
//		Vector2f usn = Vector2f::ZERO;
//
//		for ( auto t = 0; t < 6; t++ ) {
//			getSegmentUShapePoint( w, t, us1, us2, usm, usn );
//			ws.push_back( ArchSegmentService::createArchSegment( floorNumber, wallNumber, -1, w->hash, us1, us2, usm, usn, w->wallFlags ) );
//		}
//	} else if ( checkBitWiseFlag( w->wallFlags, WallFlags::WF_IsWindowPart ) ) {
//		if ( checkBitWiseFlag( w->wallFlags, WallFlags::WF_HasSkirting ) ) {
//			addToArchSegment( w, floorNumber, wallNumber, ws );
//		}
//	} else {
//		addToArchSegment( w, floorNumber, wallNumber, ws );
//	}
}

bool WallService::intersectLine2d( const WallBSData *w, Vector2f const& p0, Vector2f const& p1, Vector2f& i ) {
    bool ret = false;
//	if ( w->bbox.lineIntersection( p0, p1 ) ) {
    int csize = static_cast<int>( w->epoints.size());
    for ( auto t = 0; t < csize - ( w->wrapLastPoint == WallLastPointWrap::No ); t++ ) {
        ret = intersection( w->epoints[t], w->epoints[getCircularArrayIndex( t + 1, csize )], p0, p1, i );
        if ( ret ) return ret;
    }
//	}
    return ret;
}

void
WallService::intersectLine2dMin( WallBSData *w, Vector2f const& p0, Vector2f const& p1, Vector2f& i, float& minDist,
                                 ArchIntersection& ret, uint32_t filterFlags ) {
    V2f li{ V2f::ZERO };
//	if ( w->bbox.lineIntersection( p0, p1 ) ) {
    int csize = static_cast<int>( w->epoints.size());
    for ( auto t = 0; t < csize - ( w->wrapLastPoint == WallLastPointWrap::No ); t++ ) {
        bool lret = intersection( w->epoints[t], w->epoints[getCircularArrayIndex( t + 1, csize )], p0, p1, li );
        if ( lret ) {
            float dist = distance( li, p0 );
            if ( dist < minDist ) {
                i = li;
                minDist = dist;
                ret.hit = !checkBitWiseFlag( w->wallFlags, filterFlags );
                if ( ret.hit ) {
                    ret.arch = w;
                }
            }
        }
    }
}

void WallService::calcBBox( WallBSData *w ) {
    if ( w->epoints.size() == 0 ) return;

    w->bbox = Rect2f::INVALID;
    for ( auto& ep : w->epoints ) {
        w->bbox.expand( ep );
    }
    w->bbox3d.calc( w->bbox, w->height, Matrix4f::IDENTITY );
}

bool WallService::contains( const WallBSData *w, const Vector2f& pos ) {
    if ( w->bbox.contains( pos )) {
        if ( ArchStructuralService::isPointInside( w, pos )) return true;
    }
    return false;
}

bool WallService::findElementAt( const WallBSData *w, const Vector2f& pos, Vector2f& w1 ) {
    bool bHasFoundIt = false;

    if ( !w->bbox.contains( pos )) return bHasFoundIt;

    float minDist = std::numeric_limits<float>::max();
    for ( auto& it : w->epoints ) {
        float dist = distance( pos, it );
        if ( dist < minDist ) {
            w1 = it;
            minDist = dist;
            bHasFoundIt = true;
        }
    }
    return bHasFoundIt;
}

void WallService::perimeterFromSegments( const std::vector<std::vector<ArchSegment>>& segments,
                                         std::vector<Vector2f>& perimeterSegments, float& perimeterLength ) {
    perimeterSegments.clear();
    float lPerimeter = 0.0;
    for ( const auto& rws : segments ) {
        int csize = static_cast<int>( rws.size());
        if ( rws[0].p1 != rws[rws.size() - 1].p2 ) perimeterSegments.push_back( rws[0].p1 );
        for ( int q = 0; q < csize; q++ ) {
            auto p2 = rws[q].p2;
            perimeterSegments.push_back( p2 );
            lPerimeter += ArchSegmentService::length( rws[q] );
        }
    }
    perimeterLength = lPerimeter;
}