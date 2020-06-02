//
//  house_service.cpp
//  sixthview
//
//  Created by Dado on 16/7/2017.
//
//

#include "house_service.hpp"

#include "floor_service.hpp"
#include "arch_structural_service.hpp"
#include "room_service.hpp"

#include "core/file_manager.h"
#include "poly/collision_mesh.hpp"


std::shared_ptr<FloorBSData> HouseService::addFloorFromData( HouseBSData* _house, const JMATH::Rect2f& _rect ) {

    std::shared_ptr<FloorBSData> f = std::make_shared<FloorBSData>();
    f->asType = ASType::Floor;
    f->height = _house->defaultCeilingHeigh;
    f->anchorPoint = JMATH::Rect2fFeature::bottomRight;
    f->number = static_cast<int>( _house->mFloors.size() );
    f->bbox = _rect;
    f->ceilingContours.push_back( f->bbox.points3d( f->height ) );
    f->z = f->number * ( _house->defaultGroundHeight + _house->defaultCeilingHeigh );
    f->concreteHeight = _house->defaultGroundHeight;
    f->doorHeight = _house->doorHeight;
    f->windowHeight = _house->defaultWindowHeight;
    f->windowBaseOffset = _house->defaultWindowBaseOffset;
    f->hasCoving = true;

    _house->mFloors.push_back( f );

    return f;
}

std::shared_ptr<CollisionMesh> HouseService::createCollisionMesh( const HouseBSData *house ) {
    auto ret = std::make_shared<CollisionMesh>();

    ret->bbox = house->bbox;
    std::set<int64_t> doorAddedSet{};
    for ( const auto& f : house->mFloors ) {
        CollisionGroup cg{};
        cg.bbox = f->bbox;
        for ( const auto& r : f->rooms ) {
            CollisionRoom cr{};
            cr.bbox = r->bbox;
            for ( const auto& ls : r->mWallSegmentsSorted ) {
                if ( !checkBitWiseFlag(ls.tag, WF_IsDoorPart) ) {
                    cr.collisionCollisionElement.emplace_back( ls.p1, ls.p2, ls.normal );
                }
            }

            for ( const auto& doorHash : r->doors ) {
                for ( const auto& door : f->doors ) {
                    if ( door->hash == doorHash && doorAddedSet.find(doorHash) == doorAddedSet.end() ) {
                        cr.collisionCollisionElement.emplace_back( door->us1.points[1], door->us1.points[2], door->us1.inwardNormals[0] );
                        cr.collisionCollisionElement.emplace_back( door->us2.points[1], door->us2.points[2], door->us2.inwardNormals[0] );
                        if ( door->isMainDoor ) {
                            cr.collisionCollisionElement.emplace_back( door->us2.middle, door->us1.middle, door->us1.crossNormals[0] );
                        }
                        cr.bbox.merge(door->bbox);
                        doorAddedSet.insert(doorHash);
                    }
                }
            }
            cg.collisionRooms.emplace_back(cr);
        }
        ret->collisionGroups.emplace_back(cg);
    }
    return ret;
}

template <typename T>
bool rayIntersectInternal( std::shared_ptr<HouseBSData> _house, const std::vector<std::shared_ptr<T>>& archs, const Vector3f& origin, const Vector3f& dir, float& minNear, std::shared_ptr<ArchStructural> found ) {
    bool bHasBeenFound = false;

    for ( const auto& e : archs ) {
        float nearV = 0.0f;
        if ( ArchStructuralService::intersectLine( e.get(), origin, dir, nearV ) ) {
            if ( nearV < minNear ) {
                minNear = nearV;
                found = e;
                bHasBeenFound = true;
            }
        }
    }

    return bHasBeenFound;
}

V2f HouseService::centerOfBiggestRoom( const HouseBSData *house ) {
    using floatV2fPair = std::pair<float, V2f>;
    std::vector<floatV2fPair> roomSizes;

    for ( const auto& f : house->mFloors ) {
        for ( const auto& room : f->rooms ) {
            roomSizes.emplace_back( room->mPerimeter, RS::maxEnclsingBoundingBoxCenter(room.get()) );
        }
    }

    std::sort( roomSizes.begin(), roomSizes.end(), []( const floatV2fPair &a, const floatV2fPair &b ) -> bool { return a.first > b.first; } );

    return roomSizes[0].second;
}

bool HouseService::findFloorOrRoomAt( std::shared_ptr<HouseBSData> _house, const Vector2f& pos, int& floorIndex ) {
	for ( size_t t = 0; t < _house->mFloors.size(); t++ ) {
		if ( _house->mFloors[t]->bbox.contains( pos ) ) {
			floorIndex = t;
			return true;
		}
	}
	return false;
}

std::pair<uint64_t, uint64_t> HouseService::getFloorWallPairFor( std::shared_ptr<HouseBSData> _house, const int64_t _hash ) {
	for ( auto& f : _house->mFloors ) {
		for ( uint64_t t = 0; t < f->walls.size(); t++ ) {
			if ( f->walls[t]->hash == _hash ) {
				return std::make_pair( f->number, t );
			}
		}
	}
	ASSERT( false );
	return std::make_pair( 0, 0 );
}

std::shared_ptr<FloorBSData> HouseService::findFloorOf( std::shared_ptr<HouseBSData> _house, const int64_t _hash ) {
	for ( auto& f : _house->mFloors ) {
		for ( uint64_t t = 0; t < f->walls.size(); t++ ) {
			if ( f->walls[t]->hash == _hash ) {
				return f;
			}
		}
		for ( auto& w : f->doors ) {
			if ( w->hash == _hash ) {
				return f;
			}
		}
		for ( auto& w : f->windows ) {
			if ( w->hash == _hash ) {
				return f;
			}
		}
		for ( uint64_t t = 0; t < f->stairs.size(); t++ ) {
			if ( f->stairs[t]->hash == _hash ) {
				return f;
			}
		}
	}

	return nullptr;
}

Vector2f HouseService::getFirstFloorAnchor( std::shared_ptr<HouseBSData> _house ) {
	ASSERT( _house->mFloors.size() > 0 );

	return _house->mFloors[0]->bbox.topLeft();
}

int HouseService::floorIndexAtHeight( std::shared_ptr<HouseBSData> _house, float heightToCheck ) {
	int whatOnZFloor = 0;

	for ( size_t q = 0; q < _house->mFloors.size(); q++ ) {
		if ( isbetween( heightToCheck, _house->mFloors[q]->bbox3d.minPoint().z(), _house->mFloors[q]->bbox3d.maxPoint().z() ) ) {
			whatOnZFloor = q;
			break;
		}
	}
	return whatOnZFloor;
}

std::shared_ptr<RoomBSData> HouseService::getRoomByName( std::shared_ptr<HouseBSData> _house, const std::string& roomName ) {
	for ( const auto& f : _house->mFloors ) {
		for ( const auto& r : f->rooms ) {
			if ( RoomService::roomName( r.get() ) == roomName )
				return r;
		}
	}

	return nullptr;
}

std::vector<std::tuple<std::string, int64_t>> HouseService::getRooms( std::shared_ptr<HouseBSData> _house ) {
	std::vector<std::tuple<std::string, int64_t>> rooms;

	for ( const auto & fr : _house->mFloors )
		for ( const auto & r : fr->rooms )
			rooms.push_back( std::make_tuple( RoomService::roomName( r.get() ), r->hash ) );

	return rooms;
}

int HouseService::getNumberOfePoints( std::shared_ptr<HouseBSData> _house ) {
	auto ret = 0;
	for ( const auto& f : _house->mFloors ) {
		for ( const auto& w : f->walls ) {
			ret += static_cast<int>( w->epoints.size() );
		}
	}
	return ret;
}

int HouseService::getNumberOfWallSegments( std::shared_ptr<HouseBSData> _house ) {
	auto ret = 0;
	for ( const auto& f : _house->mFloors ) {
		ret += static_cast<int>( f->walls.size() );
	}
	return ret;
}

std::shared_ptr<ArchStructural> HouseService::rayIntersect( std::shared_ptr<HouseBSData> _house, const Vector3f& origin, const Vector3f& dir ) {
	float minNear = std::numeric_limits<float>::max();
	std::shared_ptr<ArchStructural> found;

	for ( uint64_t t = 0; t < _house->mFloors.size(); t++ ) {
		auto& floor = _house->mFloors[t];
		float nearV = 0.0f;
		if ( ArchStructuralService::intersectLine( floor.get(), origin, dir, nearV ) ) {
			rayIntersectInternal( _house, floor->walls, origin, dir, minNear, found );
			rayIntersectInternal( _house, floor->doors, origin, dir, minNear, found );
			rayIntersectInternal( _house, floor->windows, origin, dir, minNear, found );
			rayIntersectInternal( _house, floor->stairs, origin, dir, minNear, found );
		}
	}
	return found;
}

std::shared_ptr<WallBSData> HouseService::isPointInsideWall( std::shared_ptr<HouseBSData> _house, const Vector3f& point ) {
    std::shared_ptr<WallBSData> found;

    for ( const auto& f : _house->mFloors ) {
        if ( ArchStructuralService::isPointInside(f.get(), point)) {
            for ( const auto& w : f->walls ) {
                if ( ArchStructuralService::isPointInside(w.get(), point) ) {
                    return w;
                }
            }
        }
    }
    return found;
}

std::shared_ptr<WallBSData> HouseService::isPointNearWall( const HouseBSData* _house, const Vector3f& point, float radius ) {
    std::shared_ptr<WallBSData> found;

    for ( const auto& f : _house->mFloors ) {
        if ( ArchStructuralService::isPointNear(f.get(), point, radius)) {
            for ( const auto& w : f->walls ) {
                if ( ArchStructuralService::isPointNear(w.get(), point, radius) ) {
                    return w;
                }
            }
        }
    }
    return found;
}

bool HouseService::isLastFloor( std::shared_ptr<HouseBSData> _house, int floorNumber ) {
	return ( static_cast<size_t>(floorNumber) == _house->mFloors.size() - 1 );
}

bool HouseService::areThereStairsAtFloorNumber( std::shared_ptr<HouseBSData> _house, int floorNumber ) {
	if ( floorNumber < 0 ) return false;
	return _house->mFloors[floorNumber]->stairs.size() > 0;
}

Vector2f HouseService::maxSingleFloorSize( std::shared_ptr<HouseBSData> _house ) {
	Vector2f maxSingleFloorSize = V2fc::ZERO;
	for ( auto& floor : _house->mFloors ) {
		maxSingleFloorSize = max( floor->bbox.size(), maxSingleFloorSize );
	}
	return maxSingleFloorSize;
}

std::shared_ptr<RoomBSData> HouseService::getRoomOnFloor( std::shared_ptr<HouseBSData> _house, int floorIndex, int roomIndex ) {
	return _house->mFloors[floorIndex]->rooms[roomIndex];
}

float HouseService::area( HouseBSData* _house ) {
	float ret = 0.0f;
	
	for ( const auto&f : _house->mFloors ) {
		ret += FloorService::area( f.get() );
	}
	
	return ret;
}

bool HouseService::whichRoomAmI( std::shared_ptr<HouseBSData> _house, const Vector2f& _pos, std::shared_ptr<RoomBSData>& outRoom ) {
	for ( const auto& f : _house->mFloors ) {
		if ( FloorService::whichRoomAmI( f.get(), _pos, outRoom ) ) {
			return true;
		}
	}
	return false;
}

void HouseService::removeArch( std::shared_ptr<HouseBSData> _house, int64_t hashToRemove ) {
	for ( auto& f : _house->mFloors ) {
		FloorService::removeArch( f.get(), hashToRemove );
	}
}

Vector2f HouseService::centrePointOfBiggestRoom( std::shared_ptr<HouseBSData> _house ) {
    float currMaxArea = 0.0f;
    Vector2f currCenter = V2fc::ZERO;

    for ( const auto& f : _house->mFloors ) {
        FloorService::centrePointOfBiggestRoom( f.get(), currMaxArea, currCenter );
    }

    return currCenter - _house->bbox.centre();
}

void HouseService::clearFurniture( std::shared_ptr<HouseBSData> _house ) {
    for ( const auto& f : _house->mFloors ) {
        FloorService::clearFurniture( f.get() );
    }
}
void HouseService::guessFittings( HouseBSData *house, FurnitureMapStorage &furns ) {
    for ( auto &f : house->mFloors ) {
        FloorService::guessFittings( f.get(), furns );
    }
}

WallBSData* HouseService::findWall( HouseBSData *house, HashEH hash ) {
    for ( auto &f : house->mFloors ) {
        for ( auto &w : f->walls ) {
            if ( w->hash == hash ) {
                return w.get();
            }
        }
    }
    return nullptr;
}

void HouseService::clearHouseExcludingFloorsAndWalls( HouseBSData *house ) {
    for ( auto &f : house->mFloors ) {
        f->rooms.clear();
        f->windows.clear();
        f->doors.clear();
        f->stairs.clear();
    }
}

void HouseService::rescale( HouseBSData *house, float scale ) {
    for ( auto &floor : house->mFloors ) {
        FloorService::rescale( floor.get(), scale );
    }

    house->bbox = Rect2f::INVALID;
    for ( const auto &floor : house->mFloors ) {
        house->bbox.merge( floor->bbox );
        house->bbox3d.merge( floor->bbox3d );
    }

    house->center = house->bbox.centre();
    house->width = house->bbox3d.calcWidth();
    house->height = house->bbox3d.calcHeight();
    house->depth = house->bbox3d.calcDepth();
}
