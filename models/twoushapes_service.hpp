#pragma once

#include "house_bsdata.hpp"
#include "arch_structural_service.hpp"
#include "ushape_service.hpp"

class TwoUShapesBasedService {
public:

	static bool intersectLine2d( const TwoUShapesBased* t, Vector2f const& p0, Vector2f const& p1, Vector2f& /*i*/ ) {
		return t->BBox().lineIntersection( p0, p1 );
	}

	static void createGapsForSkirtingAndCoving( const TwoUShapesBased* t, std::vector<Vector2f>& fverts, FollowerGap& vGapsSkirting, FollowerGap& vGapsCoving ) {
		fverts.push_back( t->us1.points[2] );
		fverts.push_back( t->us1.points[1] );
		fverts.push_back( t->us2.points[2] );
		fverts.push_back( t->us2.points[1] );

		float almostZero = 0.00001f;
		vGapsCoving.createGap( 0, 1, almostZero, almostZero );
		vGapsCoving.createGap( 2, 3, almostZero, almostZero );

		vGapsSkirting.createGap( 0, 1, almostZero, almostZero );
		vGapsSkirting.createGap( 2, 3, almostZero, almostZero );
	}

	static std::vector<Vector2f> createWallVertices( const TwoUShapesBased* t ) {
		std::vector<Vector2f> fverts;
		fverts.push_back( t->us1.points[2] );
		fverts.push_back( t->us1.points[1] );
		fverts.push_back( t->us2.points[2] );
		fverts.push_back( t->us2.points[1] );

		return fverts;
	}

	static std::vector<Vector2f> createFrontWallVertices( const TwoUShapesBased* t ) {
		std::vector<Vector2f> fverts;
		fverts.push_back( t->us1.points[2] );
		fverts.push_back( t->us2.points[1] );

		return fverts;
	}

	static std::vector<Vector2f> createBackWallVertices( const TwoUShapesBased* t ) {
		std::vector<Vector2f> fverts;
		fverts.push_back( t->us2.points[2] );
		fverts.push_back( t->us1.points[1] );

		return fverts;
	}

    static std::vector<Vector2f> createFrontWallVertices2( const TwoUShapesBased* t ) {
        std::vector<Vector2f> fverts;
        fverts.push_back( t->us2.points[1] );
        fverts.push_back( t->us1.points[2] );

        return fverts;
    }

    static std::vector<Vector2f> createBackWallVertices2( const TwoUShapesBased* t ) {
        std::vector<Vector2f> fverts;
        fverts.push_back( t->us1.points[1] );
        fverts.push_back( t->us2.points[2] );

        return fverts;
    }

    static std::vector<Vector2f> createFrontWallVertices3( const TwoUShapesBased* t ) {
        std::vector<Vector2f> fverts;
        fverts.push_back( t->us1.points[1] );
        fverts.push_back( t->us2.points[2] );

        return fverts;
    }

    static std::vector<Vector2f> createBackWallVertices3( const TwoUShapesBased* t ) {
        std::vector<Vector2f> fverts;
        fverts.push_back( t->us2.points[1] );
        fverts.push_back( t->us1.points[2] );

        return fverts;
    }
};
