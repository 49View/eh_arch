//
//  floor_service.hpp
//  sixthview
//
//  Created by Dado on 16/7/2017.
//
//

#pragma once

#include "house_bsdata.hpp"

#include <poly/polyclipping/clipper.hpp>

//namespace ClipperLib {
//    struct IntPoint;
//    typedef std::vector< IntPoint > Path;
//    typedef std::vector< Path > Paths;
//}

struct RoomPreData;

struct DebugUShapeIntersection {
    UShape *s1 = nullptr;
    UShape *s2 = nullptr;
    V2f p = V2f::ZERO;
    int i = 0;

    DebugUShapeIntersection( UShape *s1, UShape *s2, const V2f& p, int i ) : s1( s1 ), s2( s2 ), p( p ), i( i ) {}
};

class FloorServiceIntermediateData {
public:
    static std::vector<DebugUShapeIntersection>& USI() {
        return usi;
    }

    static std::vector<ArchSegment>& WSG() {
        return wsg;
    }

    static std::vector<ArchSegment>& WSGE() {
        return wsge;
    }

    static std::vector<std::pair<V2f, V2f>>& RCUnconnectedSegments() {
        return rcus;
    }

private:
    static std::vector<DebugUShapeIntersection> usi;
    static std::vector<ArchSegment> wsg;
    static std::vector<ArchSegment> wsge;
    static std::vector<std::pair<V2f, V2f>> rcus;
};

struct FSRays {
    V2f p1;
    V2fVector dir;
};

using FSRayVector = std::vector<FSRays>;

struct ArchSegmentBucket {
    Vector2f lineOrigin = V2f::ZERO;
    Vector2f lineEnd = V2f::ZERO;
    int sourceIndex = -1;
    int destIndex = -1;

    bool operator==( const ArchSegmentBucket& lhr ) {
        return sourceIndex == lhr.sourceIndex && destIndex == lhr.destIndex;
    }

    bool hasSameCoords( const ArchSegmentBucket& lhr ) {
        return isVerySimilar( lineOrigin, lhr.lineOrigin ) && isVerySimilar( lineEnd, lhr.lineEnd );
    }

    friend std::ostream& operator<<( std::ostream& os, const ArchSegmentBucket& bucket ) {
        os << "lineOrigin: " << bucket.lineOrigin << " lineEnd: " << bucket.lineEnd << " sourceIndex: "
           << bucket.sourceIndex << " destIndex: " << bucket.destIndex;
        return os;
    }
};

class FloorService {
public:
    static void externalRaysIntoWalls( FloorBSData *f, std::vector<ArchSegment>& ws, std::vector<ArchSegment>& wse );
    static bool roomRecognition( FloorBSData *f );
    static void guessFittings( FloorBSData *f );
    static std::string naturalLanguageFloorNumber( int numFloor );
    static bool findRoom( FloorBSData *f, int _floorNumber, ASTypeT _roomASTypeToFind,
                          std::shared_ptr<RoomBSData>& ret );

    // Create
    static void
    addWallsFromData( FloorBSData *f, const V2fVectorOfVector& floorWalls,
                      WallLastPointWrapT wpw = WallLastPointWrap::No );
    static void addRoomsFromData( FloorBSData *f );
    static void addDoorFromData( FloorBSData *f, float _doorHeight, const UShape& w1, const UShape& w2,
                                 ArchSubTypeT st = ArchSubType::NotApplicable );
    static void addWindowFromData( FloorBSData *f, float _windowHeight, float _defaultWindowBaseOffset,
                                   const UShape& w1, const UShape& w2 );
    static void addCeilingContour( FloorBSData *f, const std::vector<Vector3f>& cc );

    // Update
    static std::vector<UShape *> allUShapes( FloorBSData *f );
    static void ushapesReconciliation( FloorBSData *f );
    static void
    changeTypeOfSelectedElementTo( FloorBSData *f, ArchStructural *source, ArchType t,
                                   ArchSubTypeT st );
    static void rescale( FloorBSData *f, float _scale );
    static void setCoving( FloorBSData *f, bool _state );
    static void updateFromNewDoorOrWindow( FloorBSData *f );
    static std::vector<std::pair<UShape *, UShape *> > alignSuitableUShapesFromWalls( FloorBSData *f );
    static bool checkTwoUShapesDoNotIntersectAnything( FloorBSData *f, UShape *s1, UShape *s2 );
    static void
    changeUShapeType( FloorBSData *f, const UShape& sourceUShape1, const UShape& sourceUShape2,
                      ArchType _type );
    static void swapWindowOrDoor( FloorBSData *f, HouseBSData *h, int64_t hashOfTwoShape );
    // Delete
    static void removeArch( FloorBSData *f, int64_t hashToRemove );
    static void clearFurniture( FloorBSData *f );

    // Query
    static bool hasAnyWall( const FloorBSData *f ) { return !f->walls.empty(); }

    static std::vector<Vector2f> allFloorePoints( const FloorBSData *f );
    static bool intersectLine2d( const FloorBSData *f, Vector2f const& p0, Vector2f const& p1, Vector2f& i );
    static ArchIntersection
    intersectLine2dMin( const FloorBSData *f, Vector2f const& p0, Vector2f const& p1, Vector2f& i,
                        uint32_t filterFlags = 0xffffffff );
    static bool isInsideRoomRDS( const V2f& i, const std::vector<RoomPreData>& rds );
    static bool whichRoomAmI( const FloorBSData *f, const Vector2f& _pos, std::shared_ptr<RoomBSData>& outRoom );
    static int64_t findWallIndex( const FloorBSData *f, int64_t hash );
    static bool findWallAt( const FloorBSData *f, const Vector2f& matPos, std::vector<ArchStructural *>& ret );
    static bool findRoomAt( const FloorBSData *f, const Vector2f& matPos, std::vector<ArchStructural *>& ret );
    static ArchStructural *findElementWithHash( const FloorBSData *f, int64_t hash );
    static std::vector<ArchStructural *> findElementWithLinkedHash( const FloorBSData *f, int64_t hash );
    static float area( const FloorBSData *f );

    static bool
    isInsideCeilingContour( const FloorBSData *f, const Vector2f& v1, float& topZ1, int& hitLevel1 );
    static void centrePointOfBiggestRoom( const FloorBSData *f, float& _currMaxArea,
                                          Vector2f& _currCenter );
    static ClipperLib::Paths calcPlainPath( const FloorBSData *f );

    // Update
    static void calcBBox( FloorBSData *f );
    static float updatePerimeter( FloorBSData *f, const std::vector<ArchSegment>& singleRoomSegmentsExternal );
    static void rollbackToCalculatedWalls( FloorBSData *f );

    // Remove
    static void removeLinkedArch( FloorBSData *f, int64_t hashToRemove );
    static void removeWalls( FloorBSData *f );
    static void removeWalls( FloorBSData *f, float wwidth );
    static void removeUnPairedUShapes( FloorBSData *f );
};
