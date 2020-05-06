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

class HouseService {
public:
	// Create
	static std::shared_ptr<FloorBSData> addFloorFromData( HouseBSData* _house, const JMATH::Rect2f& _rect );

	// Update

	// Delete
	static void removeArch( std::shared_ptr<HouseBSData> _house, int64_t hashToRemove );
    static void clearFurniture( std::shared_ptr<HouseBSData> _house );

	// Query
	static float area( HouseBSData* _house );
	static int getNumberOfePoints( std::shared_ptr<HouseBSData> _house );
	static int getNumberOfWallSegments( std::shared_ptr<HouseBSData> _house );
	static Vector2f getFirstFloorAnchor( std::shared_ptr<HouseBSData> _house );
	static bool isLastFloor( std::shared_ptr<HouseBSData> _house, int floorNumber );
	static std::pair<uint64_t, uint64_t> getFloorWallPairFor( std::shared_ptr<HouseBSData> _house, const int64_t _hash );
	static std::shared_ptr<ArchStructural> rayIntersect( std::shared_ptr<HouseBSData> _house, const Vector3f& origin, const Vector3f& dir );
	static bool findFloorOrRoomAt( std::shared_ptr<HouseBSData> _house, const Vector2f& pos, int& floorIndex );
	static std::shared_ptr<FloorBSData> findFloorOf( std::shared_ptr<HouseBSData> _house, const int64_t _hash );
	static bool areThereStairsAtFloorNumber( std::shared_ptr<HouseBSData> _house, int floorNumber );
	static int floorIndexAtHeight( std::shared_ptr<HouseBSData> _house, float heightToCheck );
	static Vector2f maxSingleFloorSize( std::shared_ptr<HouseBSData> _house );
	static std::vector<std::tuple<std::string, int64_t>> getRooms( std::shared_ptr<HouseBSData> _house );
	static std::shared_ptr<RoomBSData> getRoomByName( std::shared_ptr<HouseBSData> _house, const std::string& roomName );
	static std::shared_ptr<RoomBSData> getRoomOnFloor( std::shared_ptr<HouseBSData> _house, int floorIndex, int roomIndex );
	static bool whichRoomAmI( std::shared_ptr<HouseBSData> _house, const Vector2f& _pos, std::shared_ptr<RoomBSData>& outRoom );
	static Vector2f centrePointOfBiggestRoom( std::shared_ptr<HouseBSData> _house );

private:
	// Update
	// Delete
	template <typename T>
	static bool rayIntersectInternal( std::shared_ptr<HouseBSData> _house, const std::vector<std::shared_ptr<T>>& archs, const Vector3f& origin, const Vector3f& dir, float& minNear, std::shared_ptr<ArchStructural> found );
};

const static std::string floorPlansPath = "floorplans/";
