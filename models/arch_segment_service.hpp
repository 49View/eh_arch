
#pragma once

#include "house_bsdata.hpp"

class ArchSegmentService {
public:
	static void rescale( ArchSegment& _as, float _scale ) {
		_as.p1 *= _scale;
		_as.p2 *= _scale;
		_as.middle *= _scale;
	}

	//static bool operator==( const ArchSegment& rhs ) const {
	//	if ( isVerySimilar( p1, rhs.p1 ) && isVerySimilar( p2, rhs.p2 ) ) return true;
	//	return false;
	//}

	static ArchSegment createArchSegment( const int32_t _iFloor, const int32_t _iWall, const int32_t _iIndex, const int64_t _wallHash,
				 const Vector2f& _p1, const Vector2f& _p2, const Vector2f& _middle, const Vector2f& _normal, const uint64_t& _tag, const uint64_t& _sequencePart ) {
		ArchSegment ret;
		ret.iFloor = _iFloor;
		ret.iWall = _iWall;
		ret.iIndex = _iIndex;
		ret.wallHash = _wallHash;
		ret.p1 = _p1;
		ret.p2 = _p2;
		ret.middle = _middle;
		ret.normal = _normal;
		ret.tag = _tag;
		ret.crossNormal = normalize( ret.p2 - ret.p1 );
		ret.sequencePart = _sequencePart;

		return ret;
	}

	static float length( const ArchSegment& _as ) {
		return distance( _as.p1, _as.p2 );
	}

	static bool doSegmentsEndsMeet( const ArchSegment& s1, const ArchSegment& s2 ) {
	    return isVerySimilar( s1.p1, s2.p2 ) || isVerySimilar( s1.p2, s2.p1 );
	}

    static bool doSegmentsEndsMeet( const std::vector<ArchSegment>& segs ) {
	    if ( segs.size() < 2 ) return false;
	    return doSegmentsEndsMeet( segs.front(), segs.back());
    }

};
