#pragma once

#include "house_bsdata.hpp"
#include "arch_structural_service.hpp"
#include "ushape_service.hpp"

class TwoUShapesBasedService {
public:
	static void rescale( TwoUShapesBased* t, float _scale ) {
		ArchStructuralService::rescale( t, _scale );

		UShapeService::rescale( t->us1, _scale );
		UShapeService::rescale( t->us2, _scale );
		t->thickness *= _scale;
		calcBBox( t );
	}

	static void evalData( TwoUShapesBased* t ) {
		Vector2f p1 = t->us1.middle;
		Vector2f p2 = t->us2.middle;
		float wwidth = min( t->us1.width, t->us2.width );

		// Recalculate all data that might have changed
		t->dirWidth = normalize( p2 - p1 );
		t->dirDepth = rotate( t->dirWidth, M_PI_2 );
		t->width = JMATH::distance( p1, p2 );
		t->depth = wwidth;
		t->center = JMATH::lerp( 0.5f, p1, p2 );
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
		Vector2f negD = -t->dirDepth * ( t->depth*0.5f );
		Vector2f posD = t->dirDepth * ( t->depth*0.5f );
		Vector2f negW = -t->dirWidth * ( t->width*0.5f );
		Vector2f posW = t->dirWidth * ( t->width*0.5f );
		t->bbox.expand( t->center + negD + posW );
		t->bbox.expand( t->center + negD + negW );
		t->bbox.expand( t->center + posD + posW );
		t->bbox.expand( t->center + posD + negW );

		t->bbox3d.calc( t->bbox, t->ceilingHeight, Matrix4f::IDENTITY );
	}
};
