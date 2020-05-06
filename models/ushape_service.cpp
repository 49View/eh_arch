#include "ushape_service.hpp"

void UShapeService::elaborateData( UShape& us ) {
	us.width = JMATH::distance( us.points[1], us.points[2] );
	us.middle = JMATH::lerp( 0.5f, us.points[1], us.points[2] );
	us.inwardNormals[0] = normalize( us.middle - us.points[1] );
	us.inwardNormals[1] = normalize( us.middle - us.points[2] );
	Vector2f middleOuterPoints = JMATH::lerp( 0.5f, us.points[0], us.points[3] );
	us.crossNormals[0] = normalize( us.middle - middleOuterPoints );
	us.crossNormals[1] = us.crossNormals[0] * -1.0f;
}

int64_t UShapeService::sharesIndexWith( const UShape& us, const UShape& rhs ) {
	// check if they don't share the main edge indices, in this case we do not want to touch them
	if ( us.indices[1] == rhs.indices[1] || us.indices[1] == rhs.indices[2] ||
		 us.indices[2] == rhs.indices[1] || us.indices[2] == rhs.indices[2] ) {
		return -2;
	}
	if ( us.indices[1] == rhs.indices[0] && isScalarEqual( dot( us.edges[0], rhs.edges[0] ), 1.0f ) ) {
		return us.indices[1];
	}
	if ( us.indices[1] == rhs.indices[3] && isScalarEqual( dot( us.edges[0], rhs.edges[2] ), 1.0f ) ) {
		return us.indices[1];
	}
	if ( us.indices[2] == rhs.indices[0] && isScalarEqual( dot( us.edges[2], rhs.edges[0] ), 1.0f ) ) {
		return us.indices[2];
	}
	if ( us.indices[2] == rhs.indices[3] && isScalarEqual( dot( us.edges[2], rhs.edges[2] ), 1.0f ) ) {
		return us.indices[2];
	}
	return -1;
}

void UShapeService::detachFromWall( UShape& us ) {
	us.mIsDetached = true;
}

bool UShapeService::isTheSame( const UShape& us, const UShape& rhs ) {
	bool bSame = true;
	for ( int t = 0; t < 4; t++ ) {
		if ( us.points[t] != rhs.points[t] ) {
			bSame = false;
			break;
		}
	}
	return bSame;
}

void UShapeService::rescale( UShape& us, float _scale ) {
	for ( int64_t t = 0; t < 4; t++ ) us.points[t] *= _scale;
	us.middle *= _scale;
	us.width *= _scale;
}

bool UShapeService::doesShareMaineEdge( const UShape& us, const UShape& rhs ) {
	if ( rhs.points[1] == us.points[1] || rhs.points[1] == us.points[2] ) {
		if ( rhs.points[2] == us.points[1] || rhs.points[2] == us.points[2] ) {
			return true;
		}
	}
	return false;
}

bool UShapeService::isMaineEdge( const V2f& p1, const V2f& p2, const UShape& rhs ) {
    if ( rhs.points[1] ==p1 || rhs.points[1] == p2 ) {
        if ( rhs.points[2] ==p1 || rhs.points[2] == p2 ) {
            return true;
        }
    }
    return false;
}

bool UShapeService::isMaineEdgeEspsilon( const V2f& p1, const V2f& p2, const UShape& rhs, float epsilon ) {
    if ( isVerySimilar(rhs.points[1], p1, epsilon ) || isVerySimilar( rhs.points[1], p2, epsilon ) ) {
        if ( isVerySimilar(rhs.points[2], p1, epsilon) || isVerySimilar(rhs.points[2], p2, epsilon) ) {
            return true;
        }
    }
    return false;
}

bool UShapeService::isUShapeConvex( const UShape& us ) {
    auto me1 = lerp(0.5f, us.points[0], us.points[1] );
    auto me1r = me1 + rotate90(us.edges[0])*100000.0f;

    auto t1o = us.points[2] - us.edges[2]*10000.0f;
    auto t1e = us.points[3] + us.edges[2]*10000.0f;

    V2f ir{};
    return intersection( me1, me1r, t1o, t1e, ir );
}