
#pragma once

#include "house_bsdata.hpp"

class UShapeService {
public:

	static void elaborateData( UShape& us );
	static void detachFromWall( UShape& us );
	static bool isTheSame( const UShape& us, const UShape& rhs );
    static bool isUShapeConvex( const UShape& us );
	static bool doesShareMaineEdge( const UShape& us, const UShape& rhs );
	static int64_t sharesIndexWith( const UShape& us, const UShape& rhs );
    static bool isMaineEdge( const V2f& p1, const V2f& p2, const UShape& rhs );
    static bool isMaineEdgeEspsilon( const V2f& p1, const V2f& p2, const UShape& rhs, float epsilon = 0.001f );
	// Update
	static void rescale( UShape& us, float _scale );
};
