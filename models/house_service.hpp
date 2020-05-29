//
//  house_service.hpp
//  sixthview
//
//  Created by Dado on 16/7/2017.
//
//

#pragma once

#include <string>
#include <memory>

#include "house_bsdata.hpp"

class FurnitureMapStorage;

class CollisionMesh;

namespace HouseService {
    // Create
    std::shared_ptr<FloorBSData> addFloorFromData( HouseBSData *_house, const JMATH::Rect2f& _rect );
    std::shared_ptr<CollisionMesh> createCollisionMesh( const HouseBSData *_house );

    // Update

    // Delete
    void removeArch( std::shared_ptr<HouseBSData> _house, int64_t hashToRemove );
    void clearFurniture( std::shared_ptr<HouseBSData> _house );

    // Query
    float area( HouseBSData *_house );
    V2f centerOfBiggestRoom( const HouseBSData *house );
    int getNumberOfePoints( std::shared_ptr<HouseBSData> _house );
    int getNumberOfWallSegments( std::shared_ptr<HouseBSData> _house );
    Vector2f getFirstFloorAnchor( std::shared_ptr<HouseBSData> _house );
    bool isLastFloor( std::shared_ptr<HouseBSData> _house, int floorNumber );
    std::pair<uint64_t, uint64_t> getFloorWallPairFor( std::shared_ptr<HouseBSData> _house, const int64_t _hash );
    std::shared_ptr<ArchStructural>
    rayIntersect( std::shared_ptr<HouseBSData> _house, const Vector3f& origin, const Vector3f& dir );
    std::shared_ptr<WallBSData> isPointInsideWalls( std::shared_ptr<HouseBSData> _house, const Vector3f& point );
    bool findFloorOrRoomAt( std::shared_ptr<HouseBSData> _house, const Vector2f& pos, int& floorIndex );
    std::shared_ptr<FloorBSData> findFloorOf( std::shared_ptr<HouseBSData> _house, const int64_t _hash );
    bool areThereStairsAtFloorNumber( std::shared_ptr<HouseBSData> _house, int floorNumber );
    int floorIndexAtHeight( std::shared_ptr<HouseBSData> _house, float heightToCheck );
    Vector2f maxSingleFloorSize( std::shared_ptr<HouseBSData> _house );
    std::vector<std::tuple<std::string, int64_t>> getRooms( std::shared_ptr<HouseBSData> _house );
    std::shared_ptr<RoomBSData> getRoomByName( std::shared_ptr<HouseBSData> _house, const std::string& roomName );
    std::shared_ptr<RoomBSData> getRoomOnFloor( std::shared_ptr<HouseBSData> _house, int floorIndex, int roomIndex );
    bool
    whichRoomAmI( std::shared_ptr<HouseBSData> _house, const Vector2f& _pos, std::shared_ptr<RoomBSData>& outRoom );
    Vector2f centrePointOfBiggestRoom( std::shared_ptr<HouseBSData> _house );
    void guessFittings( HouseBSData *house, FurnitureMapStorage& furns );
};
