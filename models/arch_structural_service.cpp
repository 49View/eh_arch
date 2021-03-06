////
//// Created by dado on 30/05/2020.
////

#include "arch_structural_service.hpp"
#include <core/math/vector_util.hpp>
#include <eh_arch/models/house_bsdata.hpp>

float metersToCentimeters(float valueInMeter) {
    return valueInMeter*0.01f;
}

float centimetersToMeters(float valueInCM) {
    return valueInCM*100.0f;
}

bool ArchStructuralService::typeIsiPoint( const ArchStructural *a ) {
    return a->type == ArchType::WallPointT || a->type == ArchType::DoorAnchorT ||
           a->type == ArchType::WindowAnchorT;
}

bool ArchStructuralService::typeIsDOW( const ArchStructural *a ) {
    return a->type == ArchType::DoorT || a->type == ArchType::WindowT;
}

bool ArchStructuralService::typeIsWall( const ArchStructural *a ) {
    return a->type == ArchType::WallT;
}

bool ArchStructuralService::typeIsFittedFurniture( const ArchStructural *a ) {
    return a->type == ArchType::FittedFurnitureT;
}

bool ArchStructuralService::typeIsOutdoorArea( const ArchStructural *a ) {
    return a->type == ArchType::OutdoorAreaT;
}

bool ArchStructuralService::isPointInside( const ArchStructural *a, const Vector2f& _pos ) {
    if ( !a->BBox().contains(_pos) ) return false;

    if ( a->Triangles2d().empty() ) {
        return true;
    } else {
        for ( auto& t : a->Triangles2d() ) {
            if ( isInsideTriangle(_pos, std::get<0>(t), std::get<1>(t), std::get<2>(t)) ) {
                return true;
            }
        }
    }

    return false;
}

bool ArchStructuralService::isPointInside2d( const ArchStructural *a, const Vector2f& _pos ) {
    return isPointInside(a, _pos);
}

bool ArchStructuralService::isPointInside( const ArchStructural *a, const Vector3f& _pos ) {
    return isPointInside(a, _pos.xy()) && a->BBox3d().containsZ(_pos.z());
}

bool ArchStructuralService::isPointNear2d( const ArchStructural *a, const Vector2f& _pos, float radius ) {
    // We squared the circle here because it's faster to compute bbox intersection of 2 boxes rather than a sphere and a box
    Rect2f squaredCircle = Rect2f{};
    squaredCircle.setCenterAndSize(_pos, V2f{ radius });

    if ( !a->BBox().contains(_pos) && !a->BBox().intersect(squaredCircle) ) return false;

    if ( a->Triangles2d().empty() ) {
        return true;
    } else {
        for ( auto& t : a->Triangles2d() ) {
            if ( isInsideTriangle(_pos, std::get<0>(t), std::get<1>(t), std::get<2>(t)) ) {
                return true;
            }
            if ( isNearTriangle(_pos, radius, std::get<0>(t), std::get<1>(t), std::get<2>(t)) ) {
                return true;
            }
        }
    }

    return false;
}

bool ArchStructuralService::intersectLine( const ArchStructural *a, const Vector3f& linePos, const Vector3f& lineDir,
                                           float& tNear ) {
    float farV = std::numeric_limits<float>::max();
    return a->BBox3d().intersectLine(linePos, lineDir, tNear, farV);
}

bool ArchStructuralService::intersectRay( const ArchStructural *a, const RayPair3& rayPair ) {
    float farV = std::numeric_limits<float>::max();
    float vNear = 0.0f;
    return a->BBox3d().intersectLine( rayPair.origin, rayPair.dir, vNear, farV);
}

bool ArchStructuralService::intersectRayMin( const ArchStructural *a, const RayPair3& rayPair, float& tNear ) {
    float farV = std::numeric_limits<float>::max();
    float vNear = 0.0f;
    if ( a->BBox3d().intersectLine( rayPair.origin, rayPair.dir, vNear, farV) ) {
        if ( vNear < tNear ) {
            tNear = vNear;
            return true;
        }
    }
    return false;
}

bool
ArchStructuralService::intersectLine2d( const ArchStructural *t, const Vector2f& p0, const Vector2f& p1, Vector2f& ) {
    return t->BBox().lineIntersection( p0, p1 );
}

FeatureIntersection::FeatureIntersection() {
    nearV = std::numeric_limits<float>::max();
}

bool FeatureIntersection::hasHit() const {
    return nearV != std::numeric_limits<float>::max();
}
