#pragma once

#include "house_bsdata.hpp"

class ArchStructuralService {
public:
	static bool typeIsiPoint( const ArchStructural* a ) {
		return a->type == ArchType::WallPointT || a->type == ArchType::DoorAnchorT || a->type == ArchType::WindowAnchorT;
	}

	static bool typeIsDOW( const ArchStructural* a ) {
		return a->type == ArchType::DoorT || a->type == ArchType::WindowT;
	}

	static bool typeIsWall( const ArchStructural* a ) {
		return a->type == ArchType::WallT;
	}

	int pnpoly( const std::vector<Vector2f>& points, const Vector2f& test ) {
		size_t i = 0;
		size_t j = 0;
		size_t c = 0;
		for ( i = 0, j = points.size() - 1; i < points.size(); j = i++ ) {
			if ( ( ( points[i].y() > test.y() ) != ( points[j].y() > test.y() ) ) &&
				( test.x() < ( points[j].x() - points[i].x() ) * ( test.y() - points[i].y() ) / ( points[j].y() - points[i].y() ) + points[i].x() ) )
				c = !c;
		}
		return static_cast<int>( c );
	}

	static bool isPointInside( const ArchStructural* a, const Vector2f& _pos ) {
		if ( !a->bbox.contains( _pos ) ) return false;

		if ( a->mTriangles2d.size() == 0 ) {
			return true;
		} else {
			//		pnpoly( mPerimeterSegments, _pos ) == 1;

			for ( auto& t : a->mTriangles2d ) {
				if ( isInsideTriangle( _pos, std::get<0>( t ), std::get<1>( t ), std::get<2>( t ) ) ) {
					return true;
				}
			}
		}

		return false;
	}

	static Vector3f posForSDV( const ArchStructural* a, bool doomMode ) {
		if ( doomMode ) {
			Vector3f bc = a->bbox3d.centre();
			bc.swizzle( 2, 1 );
			return Vector3f::ZERO;
		}

		return a->bbox.centreRight() + Vector2f( a->bbox.width()*0.05f, 0.0f );
	}

	static void rescale( ArchStructural* a, float _scale ) {
		//height *= _scale;
		a->width *= _scale;
		a->depth *= _scale;
		a->center *= _scale;

		for ( auto& vts : a->mTriangles2d ) {
			std::get<0>( vts ) *= _scale;
			std::get<1>( vts ) *= _scale;
			std::get<2>( vts ) *= _scale;
		}
	}

	static bool intersectLine( const ArchStructural* a, const Vector3f & linePos, const Vector3f & lineDir, float & tNear ) {
		float farV = std::numeric_limits<float>::max();
		return a->bbox3d.intersectLine( linePos, lineDir, tNear, farV );
	}

};

template <typename T>
void erase_if( std::vector<T>& data, int64_t hashToRemove ) {
	data.erase( remove_if( data.begin(), data.end(), [hashToRemove]( T& us ) -> bool { return ( us->hash == hashToRemove ); } ), data.end() );
}

