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
class FurnitureMapStorage;

struct DebugUShapeIntersection {
    UShape *s1 = nullptr;
    UShape *s2 = nullptr;
    V2f p = V2fc::ZERO;
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
    Vector2f lineOrigin = V2fc::ZERO;
    Vector2f lineEnd = V2fc::ZERO;
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

namespace FloorService {
    void externalRaysIntoWalls( FloorBSData *f, std::vector<ArchSegment>& ws, std::vector<ArchSegment>& wse );
    bool roomRecognition( FloorBSData *f );
    void guessFittings( FloorBSData *f, FurnitureMapStorage& furns );
    std::string naturalLanguageFloorNumber( int numFloor );
    bool findRoom( FloorBSData *f, int _floorNumber, ASTypeT _roomASTypeToFind,
                          std::shared_ptr<RoomBSData>& ret );
    const RoomBSData* findRoomWithHash( FloorBSData *f, int64_t hash );

    // Create
    void
    addWallsFromData( FloorBSData *f, const V2fVectorOfVector& floorWalls,
                      WallLastPointWrapT wpw = WallLastPointWrap::No );
    void addRoomsFromData( FloorBSData *f );
    void addDoorFromData( FloorBSData *f, float _doorHeight, const UShape& w1, const UShape& w2,
                                 ArchSubTypeT st = ArchSubType::NotApplicable );
    void addWindowFromData( FloorBSData *f, float _windowHeight, float _defaultWindowBaseOffset,
                                   const UShape& w1, const UShape& w2 );
    void addCeilingContour( FloorBSData *f, const std::vector<Vector3f>& cc );

    // Update
    void assignRoomTypeFromBeingClever( FloorBSData *f, HouseBSData* house );
    void calcWhichRoomDoorsAndWindowsBelong( FloorBSData *f, HouseBSData* house );
    std::vector<UShape *> allUShapes( FloorBSData *f );
    void
    changeTypeOfSelectedElementTo( FloorBSData *f, ArchStructural *source, ArchType t,
                                   ArchSubTypeT st );
    void rescale( FloorBSData *f, float _scale );
    void setCoving( FloorBSData *f, bool _state );
    void updateFromNewDoorOrWindow( FloorBSData *f );
    std::vector<std::pair<UShape *, UShape *> > alignSuitableUShapesFromWalls( FloorBSData *f );
    bool checkTwoUShapesDoNotIntersectAnything( FloorBSData *f, UShape *s1, UShape *s2 );
    void
    changeUShapeType( FloorBSData *f, const UShape& sourceUShape1, const UShape& sourceUShape2,
                      ArchType _type );
    void swapWindowOrDoor( FloorBSData *f, HouseBSData *h, int64_t hashOfTwoShape );
    // Delete
    void removeArch( FloorBSData *f, int64_t hashToRemove );
    void clearFurniture( FloorBSData *f );

    // Query
    bool hasAnyWall( const FloorBSData *f );

    std::vector<Vector2f> allFloorePoints( const FloorBSData *f );
    bool intersectLine2d( const FloorBSData *f, Vector2f const& p0, Vector2f const& p1, Vector2f& i );
    ArchIntersection
    intersectLine2dMin( const FloorBSData *f, Vector2f const& p0, Vector2f const& p1, Vector2f& i,
                        uint32_t filterFlags = 0xffffffff );
    bool isInsideRoomRDS( const V2f& i, const std::vector<RoomPreData>& rds );
    bool isIndexInUShape( size_t t, WallBSData *w );
    bool whichRoomAmI( const FloorBSData *f, const Vector2f& _pos, std::shared_ptr<RoomBSData>& outRoom );
    std::vector<RoomBSData*> roomsIntersectingBBox( FloorBSData *f, const Rect2f& bbox, bool earlyOut );
    int64_t findWallIndex( const FloorBSData *f, int64_t hash );
    bool findWallAt( const FloorBSData *f, const Vector2f& matPos, std::vector<ArchStructural *>& ret );
    bool findRoomAt( const FloorBSData *f, const Vector2f& matPos, std::vector<ArchStructural *>& ret );
    ArchStructural *findElementWithHash( const FloorBSData *f, int64_t hash );
    std::vector<ArchStructural *> findElementWithLinkedHash( const FloorBSData *f, int64_t hash );
    float area( const FloorBSData *f );

    bool
    isInsideCeilingContour( const FloorBSData *f, const Vector2f& v1, float& topZ1, int& hitLevel1 );
    void centrePointOfBiggestRoom( const FloorBSData *f, float& _currMaxArea,
                                          Vector2f& _currCenter );
    ClipperLib::Paths calcPlainPath( const FloorBSData *f );
    bool isFloorUShapeValid( const FloorUShapesPair& fus );
    std::optional<uint64_t> findRoomArchSegmentWithWallHash( FloorBSData *f, HashEH hashToFind, int64_t index );

    // Update
    void calcBBox( FloorBSData *f );
    float updatePerimeter( FloorBSData *f, const std::vector<ArchSegment>& singleRoomSegmentsExternal );
    void rollbackToCalculatedWalls( FloorBSData *f);
    bool mergePoints( FloorBSData *w, const V2fVectorOfVector& points, const Rect2f& pointsBBox );

    // Remove
    void removeLinkedArch( FloorBSData *f, int64_t hashToRemove );
    void removeWalls( FloorBSData *f );
    void removeWalls( FloorBSData *f, float wwidth );
    void removeUnPairedUShapes( FloorBSData *f );
};
