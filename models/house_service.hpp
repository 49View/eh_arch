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
#include "arch_structural_service.hpp"

class FurnitureMapStorage;
class CollisionMesh;

struct IsNear {};
struct IsInside {};

namespace HouseService {
    // Create
    std::shared_ptr<FloorBSData> addFloorFromData( HouseBSData *_house, const JMATH::Rect2f& _rect );
    std::shared_ptr<CollisionMesh> createCollisionMesh( const HouseBSData *_house );

    // Update
    WallBSData *findWall( HouseBSData *house, HashEH hash );
    void rescale( HouseBSData *house, float scale );
    void recalculateBBox( HouseBSData *house );

    // Delete
    void removeArch( std::shared_ptr<HouseBSData> _house, int64_t hashToRemove );
    void clearHouse( HouseBSData *house );
    void clearFurniture( std::shared_ptr<HouseBSData> _house );
    void clearHouseExcludingFloorsAndWalls( HouseBSData *house );

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
    V2fVectorOfVector rescaleWallInverse( const HouseBSData *house, float scaleFactor );
    void guessFittings( HouseBSData *house, FurnitureMapStorage& furns );

    // Templates

    template<typename T, typename NI>
    std::shared_ptr<T> point( const HouseBSData *_house, const Vector3f& point, float radius = 0.01f ) {
        std::shared_ptr<T> found;

        auto checkNearOrInside = [&]( auto* w) {
            bool isNearInside1 = false;
            if constexpr ( std::is_same_v<NI,IsNear> ) {
                isNearInside1 = ArchStructuralService::isPointNear(w, point, radius);
            }
            if constexpr ( std::is_same_v<NI,IsInside> ) {
                isNearInside1 = ArchStructuralService::isPointInside(w, point);
            }
            return isNearInside1;
        };

        for ( const auto& f : _house->mFloors ) {
            if ( checkNearOrInside(f.get()) ) {
                if constexpr ( std::is_same_v<T, WallBSData> ) {
                    for ( const auto& w : f->walls ) {
                        if ( checkNearOrInside(w.get()) ) {
                            return w;
                        }
                    }
                }
                if constexpr ( std::is_same_v<T, DoorBSData> ) {
                    for ( const auto& w : f->doors ) {
                        if ( checkNearOrInside(w.get()) ) {
                            return w;
                        }
                    }
                }
                if constexpr ( std::is_same_v<T, WindowBSData> ) {
                    for ( const auto& w : f->windows ) {
                        if ( checkNearOrInside(w.get()) ) {
                            return w;
                        }
                    }
                }
                if constexpr ( std::is_same_v<T, RoomBSData> ) {
                    for ( const auto& w : f->rooms ) {
                        if ( checkNearOrInside(w.get()) ) {
                            return w;
                        }
                    }
                }
                if constexpr ( std::is_same_v<T, StairsBSData> ) {
                    for ( const auto& w : f->stairs ) {
                        if ( checkNearOrInside(w.get()) ) {
                            return w;
                        }
                    }
                }
            }
        }
        return found;
    }

};
