
#pragma once

#include <eh_arch/models/htypes.hpp>

class ArchSegmentService {
public:
	static void rescale( ArchSegment& _as, float _scale );

	//static bool operator==( const ArchSegment& rhs ) const {
	//	if ( isVerySimilar( p1, rhs.p1 ) && isVerySimilar( p2, rhs.p2 ) ) return true;
	//	return false;
	//}

	static ArchSegment createArchSegment( const std::vector<std::shared_ptr<WallBSData>> floorWalls, const int32_t _iFloor, const int32_t _iWall, const int32_t _iIndex, const int64_t _wallHash,
				 const Vector2f& _p1, const Vector2f& _p2, const Vector2f& _middle, const Vector2f& _normal, const uint64_t& _tag, const uint64_t& _sequencePart, float _z, float _height );
	static float length( const ArchSegment& _as );
	static bool doSegmentsEndsMeet( const ArchSegment& s1, const ArchSegment& s2 );
    static bool doSegmentsEndsMeet( const std::vector<ArchSegment>& segs );
    static bool isSegmentPartOfWindowOrDoor(const ArchSegment& ls);

};
