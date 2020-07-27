//
//  house_service.cpp
//  sixthview
//
//  Created by Dado on 16/7/2017.
//
//

#include "house_service.hpp"

#include <core/util.h>
#include <core/math/vector_util.hpp>
#include <core/file_manager.h>
#include <poly/collision_mesh.hpp>

#include "floor_service.hpp"
#include "arch_structural_service.hpp"
#include "room_service.hpp"
#include "wall_service.hpp"


std::shared_ptr<FloorBSData> HouseService::addFloorFromData( HouseBSData* _house, const JMATH::Rect2f& _rect ) {

    std::shared_ptr<FloorBSData> f = std::make_shared<FloorBSData>();
    f->type = ArchType::FloorT;
    f->height = _house->defaultCeilingHeight;
    f->anchorPoint = JMATH::Rect2fFeature::bottomRight;
    f->number = static_cast<int>( _house->mFloors.size() );
    f->bbox = _rect;
    f->ceilingContours.push_back( f->bbox.points3d( f->height ) );
    f->z = f->number * ( _house->defaultGroundHeight + _house->defaultCeilingHeight );
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
bool rayIntersectInternal( const HouseBSData* _house, const std::vector<std::shared_ptr<T>>& archs, const Vector3f& origin, const Vector3f& dir, float& minNear, std::shared_ptr<ArchStructural> found ) {
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

    if ( house->mFloors.empty() ) {
        return V2fc::ZERO;
    }

    using floatV2fPair = std::pair<float, V2f>;
    std::vector<floatV2fPair> roomSizes;

    for ( const auto& f : house->mFloors ) {
        for ( const auto& room : f->rooms ) {
            roomSizes.emplace_back( room->mPerimeter, RS::maxEnclsingBoundingBoxCenter(room.get()) );
        }
    }

    if ( roomSizes.empty() ) {
        return house->center;
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

FloorBSData* HouseService::findFloorOf( HouseBSData* _house, const int64_t _hash ) {
	for ( auto& f : _house->mFloors ) {
		for ( uint64_t t = 0; t < f->walls.size(); t++ ) {
			if ( f->walls[t]->hash == _hash ) {
				return f.get();
			}
		}
		for ( auto& w : f->doors ) {
			if ( w->hash == _hash ) {
				return f.get();
			}
		}
		for ( auto& w : f->windows ) {
			if ( w->hash == _hash ) {
				return f.get();
			}
		}
        for ( auto& w : f->rooms ) {
            if ( w->hash == _hash ) {
                return f.get();
            }
        }
		for ( uint64_t t = 0; t < f->stairs.size(); t++ ) {
			if ( f->stairs[t]->hash == _hash ) {
				return f.get();
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

std::shared_ptr<ArchStructural> HouseService::rayIntersect( const HouseBSData* _house, const Vector3f& origin, const Vector3f& dir ) {
	std::shared_ptr<ArchStructural> found = nullptr;
    float nearV = std::numeric_limits<float>::max();

	for ( uint64_t t = 0; t < _house->mFloors.size(); t++ ) {
		auto& floor = _house->mFloors[t];
        if ( ArchStructuralService::intersectLine( floor.get(), origin, dir, nearV ) ) {
			rayIntersectInternal( _house, floor->walls, origin, dir, nearV, found );
			rayIntersectInternal( _house, floor->doors, origin, dir, nearV, found );
			rayIntersectInternal( _house, floor->windows, origin, dir, nearV, found );
			rayIntersectInternal( _house, floor->stairs, origin, dir, nearV, found );
		}
	}
	return found;
}

FeatureIntersection HouseService::rayFeatureIntersect( const HouseBSData* house, const RayPair3& rayPair, FeatureIntersectionFlagsT fif ) {
    FeatureIntersection fd{};

    for ( const auto& f : house->mFloors ) {
        FloorService::rayFeatureIntersect( f.get(), rayPair, fd, fif );
    }
    return fd;
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

std::optional<RoomBSData*> HouseService::whichRoomAmI( HouseBSData* _house, const Vector2f& _pos ) {
	for ( const auto& f : _house->mFloors ) {
		if ( auto ret = FloorService::whichRoomAmI( f.get(), _pos ); ret ) {
			return ret;
		}
	}
	return std::nullopt;
}

void HouseService::removeArch( HouseBSData* house, int64_t hashToRemove ) {
	for ( auto& f : house->mFloors ) {
		FloorService::removeArch( f.get(), hashToRemove );
	}
}

void HouseService::moveArch( HouseBSData* house, ArchStructural* elem, const V2f& offset2d ) {
    for ( auto& f : house->mFloors ) {
        FloorService::moveArch( f.get(), elem, offset2d );
    }
}

void HouseService::mergePoints( HouseBSData *house, const V2fVectorOfVector& points ) {
    Rect2f pointsBBox{points};

    if ( house->mFloors.empty() ) {
        HouseService::addFloorFromData( house, pointsBBox );
    }

    for ( auto& f : house->mFloors ) {
        FloorService::mergePoints( f.get(), points, pointsBBox );
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

void HouseService::clearHouseExcludingFloorsAndWalls( HouseBSData *house ) {
    for ( auto &f : house->mFloors ) {
        FloorService::rollbackToCalculatedWalls(f.get());
    }
}

void HouseService::clearHouse( HouseBSData *house ) {
    if ( !house ) return;
    house->mFloors.clear();
}

void HouseService::clearHouseRooms( HouseBSData *house ) {
    for ( auto &f : house->mFloors ) {
        f->rooms.clear();
    }
}

V2fVectorOfVector HouseService::rescaleWallInverse( const HouseBSData* house, float scaleFactor ) {
    V2fVectorOfVector wallsPoints;
    float scale = 1.0f / scaleFactor;
    for ( const auto& f : house->mFloors ) {
        for ( const auto& w : f->walls ) {
            if ( !WallService::isWindowOrDoorPart(w.get()) ) {
                V2fVector ePointScaled{};
                for ( const auto& ep : w->epoints ) {
                    ePointScaled.emplace_back(ep * scale);
                }
                wallsPoints.emplace_back(ePointScaled);
            }
        }
    }
    return wallsPoints;
}

void HouseService::rescale( HouseBSData *house, float scale ) {
    for ( auto &floor : house->mFloors ) {
        FloorService::rescale( floor.get(), scale );
    }
    recalculateBBox( house );
    house->walkableArea = HouseService::area(house);

    // Post rescaling we might need to get feature with accurate values in cm, so this is a chance to do it
    for ( auto& f : house->mFloors ) {
        for ( auto& r : f->rooms ) {
            RoomService::calcSkirtingSegments(r.get());
        }
    }
}

void HouseService::recalculateBBox( HouseBSData *house ) {
    house->bbox = Rect2f::INVALID;
    for ( auto &floor : house->mFloors ) {
        FloorService::calcBBox(floor.get());
        house->bbox.merge( floor->bbox );
        house->bbox3d.merge( floor->bbox3d );
    }

    house->center = house->bbox.centre();
    house->width = house->bbox3d.calcWidth();
    house->height = house->bbox3d.calcHeight();
    house->depth = house->bbox3d.calcDepth();
}

float HouseService::area( const HouseBSData *house ) {
    float ret = 0.0f;

    for ( const auto& w : house->mFloors ) ret += FloorService::area(w.get());

    return ret;
}

void HouseService::swapWindowOrDoor( HouseBSData *house, int64_t hashOfTwoShape ) {
    for ( auto &floor : house->mFloors ) {
        FloorService::swapWindowOrDoor( floor.get(), house, hashOfTwoShape );
    }
}

std::optional<uint64_t> HouseService::findRoomArchSegmentWithWallHash( HouseBSData *house, HashEH hashToFind, int64_t index ) {
    for ( auto &floor : house->mFloors ) {
        auto ret = FloorService::findRoomArchSegmentWithWallHash( floor.get(), hashToFind, index );
        if ( ret ) return ret;
    }
    return std::nullopt;
}

void HouseService::bestStartingPositionAndAngle( const HouseBSData *house, V3f& pos, Quaternion& rot ) {
    if ( house->bestInternalViewingPosition == V3f::ZERO ) {
        pos = XZY::C(HouseService::centerOfBiggestRoom(house), 1.48f);
        rot = quatCompose( V3f{ 0.08f, -0.70f, 0.0f } );
    } else {
        pos = house->bestInternalViewingPosition;
        rot = house->bestInternalViewingAngle;
    }
}

void HouseService::bestDollyPositionAndAngle( const HouseBSData *house, V3f& pos, Quaternion& rot ) {
    if ( house->bestDollyViewingPosition == V3f::ZERO ) {
        rot = quatCompose( V3f{ 1.0f, -0.75f, 0.0f } );
        pos = V3f{ house->bbox.bottomRight().x(), house->depth * 3.0f,
                       house->bbox.bottomRight().y() } ;
    } else {
        pos = house->bestDollyViewingPosition;
        rot = house->bestDollyViewingAngle;
    }

}

void HouseService::reevaluateDoorsAndWindowsAfterRoomChange( HouseBSData* house ) {
    for ( auto &floor : house->mFloors ) {
        FloorService::reevaluateDoorsAndWindowsAfterRoomChange( floor.get() );
    }
}

///
/// \param _house
/// \param csk
/// we add an initial keyframe just to avoid having to do an push + add in two separate calls

void HouseService::pushTourPath( HouseBSData *house, const CameraSpatialsKeyFrame& csk ) {
    CameraPath newPath{};
    newPath.path.emplace_back(csk);
    house->tourPaths.emplace_back(newPath);
}

void HouseService::pushKeyFrameTourPath( HouseBSData *house, const CameraSpatialsKeyFrame& csk ) {
    auto incrementalCSK = csk;
    incrementalCSK.timestamp += house->tourPaths[house->tourPaths.size()-1].path.back().timestamp;
    house->tourPaths[house->tourPaths.size()-1].path.emplace_back(incrementalCSK);
}

void HouseService::popTourPath( HouseBSData *_house, int i ) {
    if ( i == -1 ) {
        _house->tourPaths.pop_back();
    } else {
        if ( i>=0 && i < static_cast<int>(_house->tourPaths.size()) ) {
            _house->tourPaths.erase(_house->tourPaths.begin()+i);
        }
    }
}

bool HouseService::hasTour( const HouseBSData *_house ) {
    return !_house->tourPaths.empty();
}

void HouseService::changeFloorsMaterial( HouseBSData *house, const MaterialAndColorProperty& mcp ) {
    for ( auto& f : house->mFloors ) {
        for ( auto& r : f->rooms ) {
            RoomService::changeFloorsMaterial( r.get(), mcp );
        }
    }
}

void HouseService::changeWallsMaterial( HouseBSData *house, const MaterialAndColorProperty& mcp ) {
    for ( auto& f : house->mFloors ) {
        for ( auto& r : f->rooms ) {
            RoomService::changeWallsMaterial( r.get(), mcp );
        }
    }
}

void HouseService::changeCeilingsMaterial( HouseBSData *house, const MaterialAndColorProperty& mcp ) {
    for ( auto& f : house->mFloors ) {
        for ( auto& r : f->rooms ) {
            RoomService::changeCeilingsMaterial( r.get(), mcp );
        }
    }
}

void HouseService::changeSkirtingsMaterial( HouseBSData *house, const MaterialAndColorProperty& mcp ) {
    for ( auto& f : house->mFloors ) {
        for ( auto& r : f->rooms ) {
            RoomService::changeSkirtingsMaterial( r.get(), mcp );
        }
    }
}

void HouseService::changeCovingsMaterial( HouseBSData *house, const MaterialAndColorProperty& mcp ) {
    for ( auto& f : house->mFloors ) {
        for ( auto& r : f->rooms ) {
            RoomService::changeCovingsMaterial( r.get(), mcp );
        }
    }
}

void HouseService::changeSkirtingsProfile( HouseBSData *house, const MaterialAndColorProperty& mcp ) {

}

void HouseService::changeCovingsProfile( HouseBSData *house, const MaterialAndColorProperty& mcp ) {

}
