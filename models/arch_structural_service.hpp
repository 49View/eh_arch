#pragma once

#include "house_bsdata.hpp"

float metersToCentimeters( float valueInMeter );
float centimetersToMeters( float valueInCM );

class FeatureIntersection {
public:
    [[nodiscard]] bool hasHit() const {
        return arch != nullptr;
    }

    ArchStructural* arch = nullptr;
    float nearV = 0.0f;
    V3f normal = V3f::ZERO;
};

struct ArchStructuralFeatureDescriptor {
    ArchStructuralFeatureDescriptor() = default;
    explicit ArchStructuralFeatureDescriptor( ArchBase *_elem ) : elem(_elem) {}

    ArchStructuralFeatureDescriptor( ArchStructuralFeature feature, int64_t index, ArchBase *_elem ) : feature(feature),
                                                                                                       index(index),
                                                                                                       elem(_elem) {}

    ArchStructuralFeatureDescriptor( ArchStructuralFeature feature, int64_t index, ArchBase *_elem,
                                     const V2fVector& pointOfInterests ) : feature(feature), index(index), elem(_elem),
                                                                           pointOfInterests(pointOfInterests) {}

    ArchStructuralFeatureDescriptor( ArchStructuralFeature feature, int64_t index, ArchBase *_elem,
                                     const V2fVector& pois, const V2f& nd ) : feature(feature),
                                                                              index(index),
                                                                              elem(_elem),
                                                                              pointOfInterests(pois),
                                                                              normalDirection(nd) {}

    bool operator==( const ArchStructuralFeatureDescriptor& rhs ) const {
        return std::tie(feature, index, elem) == std::tie(rhs.feature, rhs.index, rhs.elem);
    }
    bool operator!=( const ArchStructuralFeatureDescriptor& rhs ) const {
        return !( rhs == *this );
    }

    ArchStructuralFeature feature = ArchStructuralFeature::ASF_None;
    int64_t index = -1;
    ArchBase *elem = nullptr;
    float distanceToNearestFeature = 0.0f;
    V2fVector pointOfInterests{};
    V2f normalDirection{ V2fc::ZERO };
};

using ASFD = ArchStructuralFeatureDescriptor;

class ArchStructuralService {
public:
    static bool typeIsiPoint( const ArchStructural *a );
    static bool typeIsDOW( const ArchStructural *a );
    static bool typeIsWall( const ArchStructural *a );
    static bool typeIsFittedFurniture( const ArchStructural *a );
    static bool isPointInside( const ArchStructural *a, const Vector2f& _pos );
    static bool isPointInside( const ArchStructural *a, const Vector3f& _pos );
    static bool isPointNear( const ArchStructural *a, const Vector2f& _pos, float radius );
    static void rescale( ArchStructural *a, float _scale );
    static bool
    intersectLine( const ArchStructural *a, const Vector3f& linePos, const Vector3f& lineDir, float& tNear );
};

template<typename T>
void erase_if( std::vector<T>& data, int64_t hashToRemove ) {
    data.erase(remove_if(data.begin(), data.end(),
                         [hashToRemove]( T& us ) -> bool { return ( us->hash == hashToRemove ); }), data.end());
}

