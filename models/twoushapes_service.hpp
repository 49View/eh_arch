#pragma once

#include "house_bsdata.hpp"
#include "arch_structural_service.hpp"
#include "ushape_service.hpp"

class TwoUShapesBasedService {
public:
	static void rescale( TwoUShapesBased* t, float _scale ) {

		UShapeService::rescale( t->us1, _scale );
		UShapeService::rescale( t->us2, _scale );
		t->thickness *= _scale;

        ArchStructuralService::rescale( t, _scale );
		calcBBox( t );
	}

	static void evalData( TwoUShapesBased* t, float _height ) {
		Vector2f p1 = t->us1.middle;
		Vector2f p2 = t->us2.middle;
		float wwidth = min( t->us1.width, t->us2.width );

		// Recalculate all data that might have changed
		t->dirWidth = normalize( p2 - p1 );
		t->dirDepth = rotate( t->dirWidth, M_PI_2 );
		t->w() = JMATH::distance( p1, p2 );
		t->d() = wwidth;
		t->h() = _height;

		calcBBox( t );
	}

	static bool intersectLine2d( const TwoUShapesBased* t, Vector2f const& p0, Vector2f const& p1, Vector2f& /*i*/ ) {
		return t->bbox.lineIntersection( p0, p1 );
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

    static void calcBBox( TwoUShapesBased* t ) {
		t->bbox = JMATH::Rect2f::INVALID;
		Vector2f negD = -t->dirDepth * ( t->HalfDepth() );
		Vector2f posD = t->dirDepth * ( t->HalfDepth() );
		Vector2f negW = -t->dirWidth * ( t->HalfWidth() );
		Vector2f posW = t->dirWidth * ( t->HalfWidth() );
		t->bbox.expand( t->Center() + negD + posW );
		t->bbox.expand( t->Center() + negD + negW );
		t->bbox.expand( t->Center() + posD + posW );
		t->bbox.expand( t->Center() + posD + negW );

		t->bbox3d.calc( t->bbox, t->ceilingHeight, Matrix4f::IDENTITY );
	}
};
