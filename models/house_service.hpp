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

#include <eh_arch/models/htypes.hpp>
#include <eh_arch/models/house_bsdata.hpp>
#include <eh_arch/models/arch_structural_service.hpp>
#include <core/htypes_shared.hpp>
#include <core/math/htypes.hpp>
#include <poly/htypes.hpp>

struct IsNear {
};
struct IsInside {
};

namespace HouseService {

    // Update
    void pushTourPath( HouseBSData *_house, const CameraSpatialsKeyFrame& csk );
    void pushKeyFrameTourPath( HouseBSData *_house, const CameraSpatialsKeyFrame& csk );
    void popTourPath( HouseBSData *_house, int i );
    void swapWindowOrDoor( HouseBSData *house, int64_t hashOfTwoShape );
    void mergePoints( HouseBSData *f, const V2fVectorOfVector& points );
    void guessFittings( HouseBSData *house, FurnitureMapStorage& furns );
    void reevaluateDoorsAndWindowsAfterRoomChange( HouseBSData* h );
    void moveArch( HouseBSData *_house, ArchStructural* elem, const V2f& offset2d );
    void changeWallsMaterial( HouseBSData *house, const MaterialAndColorProperty& mcp );
    void changeFloorsMaterial( HouseBSData *house, const MaterialAndColorProperty& mcp );
    void changeCeilingsMaterial( HouseBSData *house, const MaterialAndColorProperty& mcp );
    void changeSkirtingsMaterial( HouseBSData *, const MaterialAndColorProperty& mcp );
    void changeCovingsMaterial( HouseBSData *, const MaterialAndColorProperty& mcp );
    void changeSkirtingsProfile( HouseBSData *, const MaterialAndColorProperty& mcp );
    void changeCovingsProfile( HouseBSData *, const MaterialAndColorProperty& mcp );

    // Delete
    void removeArch( HouseBSData *_house, int64_t hashToRemove );
    void clearHouse( HouseBSData *house );
    void clearHouseRooms( HouseBSData *house );
    void clearFurniture( std::shared_ptr<HouseBSData> _house );
    void clearHouseExcludingFloorsAndWalls( HouseBSData *house );

    // Query
    V2fVectorOfVector rescaleWallInverse( const HouseBSData *house, float scaleFactor );
    std::shared_ptr<CollisionMesh> createCollisionMesh( const HouseBSData *_house );
    [[nodiscard]] bool hasTour(const HouseBSData *_house);
    float area( const HouseBSData *_house );
    V3f centerOfBiggestRoom( const HouseBSData *house, float _preferredHeight );
    void bestStartingPositionAndAngle( const HouseBSData *house, V3f& pos, Quaternion& rot );
    void bestDollyPositionAndAngle( const HouseBSData *house, V3f& pos, Quaternion& rot );
    int getNumberOfePoints( std::shared_ptr<HouseBSData> _house );
    int getNumberOfWallSegments( std::shared_ptr<HouseBSData> _house );
    Vector2f getFirstFloorAnchor( std::shared_ptr<HouseBSData> _house );
    bool isLastFloor( std::shared_ptr<HouseBSData> _house, int floorNumber );
    std::pair<uint64_t, uint64_t> getFloorWallPairFor( std::shared_ptr<HouseBSData> _house, int64_t _hash );
    std::shared_ptr<ArchStructural>
    rayIntersect( const HouseBSData* _house, const Vector3f& origin, const Vector3f& dir );
    FeatureIntersection rayFeatureIntersect( const HouseBSData* house, const RayPair3& rayPair, FeatureIntersectionFlagsT fif );
    bool findFloorOrRoomAt( std::shared_ptr<HouseBSData> _house, const Vector2f& pos, int& floorIndex );
    FloorBSData* findFloorOf( HouseBSData* _house, int64_t _hash );
    bool areThereStairsAtFloorNumber( std::shared_ptr<HouseBSData> _house, int floorNumber );
    int floorIndexAtHeight( std::shared_ptr<HouseBSData> _house, float heightToCheck );
    Vector2f maxSingleFloorSize( std::shared_ptr<HouseBSData> _house );
    std::vector<std::tuple<std::string, int64_t>> getRooms( std::shared_ptr<HouseBSData> _house );
    std::shared_ptr<RoomBSData> getRoomByName( std::shared_ptr<HouseBSData> _house, const std::string& roomName );
    std::shared_ptr<RoomBSData> getRoomOnFloor( std::shared_ptr<HouseBSData> _house, int floorIndex, int roomIndex );
    std::optional<RoomBSData*> whichRoomAmI( HouseBSData* _house, const Vector2f& _pos );
    Vector2f centrePointOfBiggestRoom( std::shared_ptr<HouseBSData> _house );
    std::optional<uint64_t> findRoomArchSegmentWithWallHash( HouseBSData *_house, HashEH hashToFind, int64_t index );

    // Update
    // Templates
    template<typename T>
    T *find( HouseBSData *_house, HashEH hash ) {
        for ( auto& f : _house->mFloors ) {
            if constexpr ( std::is_same_v<T, WallBSData> ) {
                for ( auto& w : f->walls ) {
                    if ( w->hash == hash ) {
                        return w.get();
                    }
                }
            }
            if constexpr ( std::is_same_v<T, DoorBSData> ) {
                for ( auto& w : f->doors ) {
                    if ( w->hash == hash ) {
                        return w.get();
                    }
                }
            }
            if constexpr ( std::is_same_v<T, WindowBSData> ) {
                for ( auto& w : f->windows ) {
                    if ( w->hash == hash ) {
                        return w.get();
                    }
                }
            }
            if constexpr ( std::is_same_v<T, RoomBSData> ) {
                for ( auto& w : f->rooms ) {
                    if ( w->hash == hash ) {
                        return w.get();
                    }
                }
            }
            if constexpr ( std::is_same_v<T, StairsBSData> ) {
                for ( auto& w : f->stairs ) {
                    if ( w->hash == hash ) {
                        return w.get();
                    }
                }
            }
            if constexpr ( std::is_same_v<T, BalconyBSData> ) {
                for ( auto& w : f->balconies ) {
                    if ( w->hash == hash ) {
                        return w.get();
                    }
                }
            }
        }
        return nullptr;;
    }

    template<typename T, typename NI>
    std::shared_ptr<T> point2d( const HouseBSData *_house, const Vector2f& point, float radius = 0.01f ) {
        std::shared_ptr<T> found;

        auto checkNearOrInside = [&]( auto *w ) {
            bool isNearInside1 = false;
            if constexpr ( std::is_same_v<NI, IsNear> ) {
                isNearInside1 = ArchStructuralService::isPointNear2d(w, point, radius);
            }
            if constexpr ( std::is_same_v<NI, IsInside> ) {
                isNearInside1 = ArchStructuralService::isPointInside2d(w, point);
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
                if constexpr ( std::is_same_v<T, FittedFurniture> ) {
                    for ( const auto& w : f->rooms ) {
                        if ( checkNearOrInside(w.get()) ) {
                            for ( const auto& ff : w->mFittedFurniture ) {
                                if ( checkNearOrInside(ff.get()) ) {
                                    return ff;
                                }
                            }
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
                if constexpr ( std::is_same_v<T, BalconyBSData> ) {
                    for ( const auto& w : f->balconies ) {
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
