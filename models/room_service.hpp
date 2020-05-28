//
//  room_service.hpp
//  sixthview
//
//  Created by Dado on 16/7/2017.
//
//

#pragma once

#include <memory>
#include <utility>
#include <vector>

#include "house_bsdata.hpp"

class FurnitureTypeHandler {
public:
    enum Type {
        FT_Bed = 0,
        FT_Bedside,
        FT_Shelf,
        FT_Wardrobe,
        FT_Drawer,
        FT_Carpet,
        FT_Armchair,
        FT_Sofa,
        FT_SideBoard,
        FT_TVHanged,
        FT_TVWithStand,
        FT_Picture,
        FT_CoffeeTable,
        FT_DiningTable,
        FT_Plant,
        FT_Toilet,
        FT_Shower,
        FT_BathroomSink,
        FT_BathroomTowerRadiator,
        FT_Sink,
        FT_OvenPanel,
        FT_Microwave,
        FT_Cooktop,
        FT_Fridge,
        FT_ExtractorHood,
        FT_DrawersHandle,

        FT_Invalid,
        Last
    };

    static Type random() {
        return Type(unitRandI(Type::Last));
    }

    static Type at( size_t index ) {
        if ( index >= FT_Invalid ) return FT_Invalid;
        return Type(index);
    }
    static size_t count() {
        return Type::Last - 1;
    }
};

using FT = FurnitureTypeHandler::Type;
using FTH = FurnitureTypeHandler;

class FurnitureMapStorage;

enum class FFD;

enum class IncludeWindowsOrDoors {
    None,
    WindowsOnly,
    DoorsOnly,
    Both
};

enum class WalkSegmentDirection {
    Left,
    Right
};

enum WallSegmentCorner {
    WSC_P1 = 0,
    WSC_P2
};

class WallSegmentLenghtOrderHandler {
public:
    enum Type {
        WSLO_Longest = 0,
        WSLO_LongestOpposite,
        WSLO_SecondLongest,
        WSLO_Shortest,
        WSLO_SecondShortest,
        WSLO_ExactIndex,
        WSLO_FirstAvailable,
        WSLO_Invalid,
        Last
    };

    static constexpr Type Longest() { return Type::WSLO_Longest; }
    static constexpr Type LongestOpposite() { return Type::WSLO_LongestOpposite; }
    static constexpr Type SecondLongest() { return Type::WSLO_SecondLongest; }
    static constexpr Type Shortest() { return Type::WSLO_Shortest; }
    static constexpr Type SecondShortest() { return Type::WSLO_SecondShortest; }
    static constexpr Type ExactIndex() { return Type::WSLO_ExactIndex; }
    static constexpr Type FirstAvailable() { return Type::WSLO_FirstAvailable; }

    static Type random() {
        return Type(unitRandI(Type::Last - 2));
    }

    static Type at( size_t index ) {
        if ( index >= Type::WSLO_Invalid ) return Type::WSLO_Invalid;
        return Type(index);
    }
    static size_t count() {
        return Type::Last - 2;
    }
};

using WSLO = WallSegmentLenghtOrderHandler::Type;
using WSLOH = WallSegmentLenghtOrderHandler;

struct WallSegmentIdentifier {
    explicit WallSegmentIdentifier( WSLO type ) : type(type) {}
    WallSegmentIdentifier( WSLO type, int index ) : type(type), index(index) {}

    WSLO type = WSLOH::Longest();
    int index = 0;
};

template<typename T>
struct DistanceNormalPair {
    float distance;
    T normal;
    DistanceNormalPair( float distance, const T& normal ) : distance(distance), normal(normal) {}
};

using Distance2dNormalPair = DistanceNormalPair<V2f>;
using Distance3dNormalPair = DistanceNormalPair<V3f>;

class FurniturePlacementRule;

using FurnitureRefs = std::vector<std::vector<FT>>;
using FurnitureSlacks = std::vector<V3f>;
using FurnitureRuleIndex = int;
using FurnitureRuleFunction = std::function<bool( FloorBSData *, RoomBSData *, FurnitureMapStorage&,
                                                  const FurniturePlacementRule& )>;
using FurnitureRuleFunctionContainer = std::vector<FurnitureRuleFunction>;

namespace RoomService {
}
class FurniturePlacementRule {
public:
    inline static constexpr size_t BASE_FURNITURE_INDEX = 0;
    inline static constexpr size_t DECORATION_FURNITURE_INDEX = 1;

    template<typename ...Args>
    explicit FurniturePlacementRule( Args&& ...args ) {
        furnitureRefs.resize(2);
        (setParam(std::forward<Args>(args)), ...);
    }

    template<typename D>
    void setParam( D&& _data ) {
        if constexpr ( std::is_same_v<D, FurnitureRefs> ) {
            furnitureRefs = _data;
            return;
        }
        if constexpr ( std::is_same_v<D, WallSegmentIdentifier> ) {
            wallSegmentId = _data;
            return;
        }
        if constexpr ( std::is_same_v<D, WSLO> ) {
            wallSegmentId = WallSegmentIdentifier{ _data };
            return;
        }
        if constexpr ( std::is_same_v<D, FurnitureSlacks> ) {
            slack = _data;
            return;
        }
        if constexpr ( std::is_same_v<D, V3f> ) {
            slack.emplace_back(_data);
            return;
        }
        if constexpr ( std::is_same_v<D, WallSegmentCorner> ) {
            preferredCorner = _data;
            return;
        }
        if constexpr ( std::is_same_v<D, FurnitureRuleIndex> ) {
            ruleFunctionIndex = _data;
            return;
        }
        LOGR("Houston we have a problem");
    }

    [[nodiscard]] bool hasSomethingToDo() const {
        return hasFurnitures();
    }

    [[nodiscard]] bool hasFurnituresArray() const {
        return !( furnitureRefs.empty() );
    }
    [[nodiscard]] bool hasDecorationsArray() const {
        return furnitureRefs.size() > DECORATION_FURNITURE_INDEX;
    }

    [[nodiscard]] bool hasFurnitures() const {
        return ( hasFurnituresArray() && !furnitureRefs[BASE_FURNITURE_INDEX].empty() );
    }
    [[nodiscard]] bool hasDecorations() const {
        return ( hasDecorationsArray() && !furnitureRefs[DECORATION_FURNITURE_INDEX].empty() );
    }

    [[nodiscard]] size_t baseFurnitureCount() const {
        ASSERT(hasFurnituresArray());
        return furnitureRefs[BASE_FURNITURE_INDEX].size();
    }

    [[nodiscard]] size_t decorationFurnitureCount() const {
        ASSERT(hasFurnitures());
        return furnitureRefs[DECORATION_FURNITURE_INDEX].size();
    }

    [[nodiscard]] bool hasBase( size_t _index ) const {
        return ( hasFurnituresArray() && baseFurnitureCount() > _index );
    }

    [[nodiscard]] bool hasDecoration( size_t _index ) const {
        return ( hasDecorationsArray() && decorationFurnitureCount() > _index );
    }

    [[nodiscard]] FT getBase( size_t _index ) const {
        ASSERT(hasFurnituresArray() && baseFurnitureCount() > _index);
        return furnitureRefs[BASE_FURNITURE_INDEX][_index];
    }

    [[nodiscard]] FT getDecoration( size_t _index ) const {
        ASSERT(hasDecorationsArray() && decorationFurnitureCount() > _index);
        return furnitureRefs[DECORATION_FURNITURE_INDEX][_index];
    }

    [[nodiscard]] const auto& getBases() const {
        ASSERT(hasFurnituresArray());
        return furnitureRefs[BASE_FURNITURE_INDEX];
    }

    [[nodiscard]] const auto& getDecorations() const {
        ASSERT(hasDecorationsArray());
        return furnitureRefs[DECORATION_FURNITURE_INDEX];
    }

    void addBase( const FT& _ft ) {
        furnitureRefs[BASE_FURNITURE_INDEX].emplace_back(_ft);
    }

    void addDecoration( const FT& _ft ) {
        furnitureRefs[DECORATION_FURNITURE_INDEX].emplace_back(_ft);;
    }

    [[nodiscard]] const WallSegmentIdentifier& getWallSegmentId() const {
        return wallSegmentId;
    }

    void setWallSegmentId( const WallSegmentIdentifier& _wallSegmentId ) {
        FurniturePlacementRule::wallSegmentId = _wallSegmentId;
    }

    [[nodiscard]] const V3f& getSlack( size_t _index = 0 ) const {
        if ( slack.size() <= _index ) return V3f::ZERO;
        return slack[_index];
    }

    void setSlack( const V3f& _slack, size_t _index = 0 ) {
        ASSERT(slack.size() > _index);
        slack[_index] = _slack;
    }

    [[nodiscard]] WallSegmentCorner getPreferredCorner() const {
        return preferredCorner;
    }

    void setPreferredCorner( WallSegmentCorner _preferredCorner ) {
        FurniturePlacementRule::preferredCorner = _preferredCorner;
    }

    [[nodiscard]] FurnitureRuleIndex getRuleFunctionIndex() const {
        return ruleFunctionIndex;
    }

    void setRuleFunctionIndex( FurnitureRuleIndex _ruleFunctionIndex ) {
        ruleFunctionIndex = _ruleFunctionIndex;
    }

    static FurniturePlacementRule randomGeneration();

private:
    FurnitureRuleIndex ruleFunctionIndex = 0;
    FurnitureRefs furnitureRefs{};
    WallSegmentIdentifier wallSegmentId{ WSLOH::Longest() };
    FurnitureSlacks slack{};
    WallSegmentCorner preferredCorner = WSC_P1;
};

class FurnitureRuleScript {
public:
    template<typename ...Args>
    void addRule( Args&& ...args ) {
        rules.emplace_back(std::forward<Args>(args)...);
    }
    bool execute( FloorBSData *f, RoomBSData *r, FurnitureMapStorage& furns,
                  const FurnitureRuleFunctionContainer& funcRules ) const;
    void clear();
private:
    std::vector<FurniturePlacementRule> rules;
};

namespace RoomService {
    std::shared_ptr<RoomBSData> createRoom( const RoomPreData& _preData, float _floorHeight, float _z );

    void updateFromArchSegments( RoomBSData *r, const std::vector<std::vector<ArchSegment>>& ws );
    void calcOptimalLightingFittingPositions( RoomBSData *r );
    void addSocketsAndSwitches( RoomBSData *r );
    void calcSkirtingSegments( RoomBSData *r );
    void setCoving( RoomBSData *r, bool _state );
    void changeFloorType( RoomBSData *r, FloorMatTypeT _fmt );
    std::vector<std::vector<Vector2f>> calcCovingSegments( const std::vector<std::vector<ArchSegment>>& ws );

    void calcBBox( RoomBSData *r );
    void rescale( RoomBSData *r, float _scale );

    void addSocketIfSafe( RoomBSData *r, const std::vector<Vector2f>& cov, size_t indexSkirting,
                          float safeSocketBoxWidth );
    void calcLongestWall( RoomBSData *r );
    void WallSegments( RoomBSData *r, const std::vector<std::vector<ArchSegment>>& val );
    void makeTriangles2d( RoomBSData *r );
    void calclMaxBoundingBox( RoomBSData *r );

    void furnish( FloorBSData *f, RoomBSData *r, FurnitureMapStorage& furns );
    const ArchSegment *getWallSegmentFor( RoomBSData *r, WSLO wslo, uint32_t _exactIndex = 0 );

    void clearFurniture( RoomBSData *r );
    [[nodiscard]] bool addFurniture( FloorBSData *f, RoomBSData *r, FittedFurniture& ff );
    bool placeManually( FloorBSData *f, RoomBSData *r, FittedFurniture& _ff, const V2f& _pos, const Quaternion& _rot,
                        const V2f& _widthNormal, const V2f& _depthNormal );
    [[nodiscard]] bool placeWallAligned( FloorBSData *f, RoomBSData *r, FittedFurniture& _ff,
                                         WSLO wslo, float extraSlack = 0.0f, uint32_t _exactIndex = 0 );
    [[nodiscard]] bool placeWallCorner( FloorBSData *f, RoomBSData *r, FittedFurniture& _ff,
                                        const ArchSegment *ls,
                                        const V2f& slack = V2fc::ZERO,
                                        WallSegmentCorner wsc = WSC_P1,
                                        float _height = 0.0f );
    [[nodiscard]] bool placeAround( FloorBSData *f, RoomBSData *r, FittedFurniture& dest, const FittedFurniture& source,
                                    PivotPointPosition where,
                                    const V2f& slack = V2fc::ZERO, float _height = 0.0f );
    [[nodiscard]] bool
    placeWallAlong( FloorBSData *f, RoomBSData *r, FittedFurniture& dest, const FittedFurniture& source,
                    const ArchSegment *ls, WallSegmentCorner preferredCorner, const V2f& slack = V2fc::ZERO,
                    float _height = 0.0f );
    [[nodiscard]] bool
    placeInBetween( FloorBSData *f, RoomBSData *r, const std::vector<FittedFurniture>& fset, PivotPointPosition where,
                    const V2f& slack = V2fc::ZERO, float _height = 0.0f );

    // Composite furnitures
    [[nodiscard]] bool cplaceMainWith2Sides( FloorBSData *f, RoomBSData *r, FurnitureMapStorage& furns,
                                             const FurniturePlacementRule& fpd );
    [[nodiscard]] bool
    cplaceCornerWithDec( FloorBSData *f, RoomBSData *r, FurnitureMapStorage& furns, const FurniturePlacementRule& fpd );
    [[nodiscard]] bool cplaceSetAlignedAtCorner( FloorBSData *f, RoomBSData *r, FurnitureMapStorage& furns,
                                                 const FurniturePlacementRule& fpd );
    [[nodiscard]] bool cplaceSetAlignedMiddle( FloorBSData *f, RoomBSData *r, FurnitureMapStorage& furns,
                                               const FurniturePlacementRule& fpd );
    [[nodiscard]] bool cplaceSetBestFit( FloorBSData *f, RoomBSData *r, FurnitureMapStorage& furns,
                                         const FurniturePlacementRule& fpd );
    [[nodiscard]] bool cplacedFirstAvailableCorner( FloorBSData *f, RoomBSData *r, FurnitureMapStorage& furns,
                                                    const FurniturePlacementRule& fpd );
    [[nodiscard]] bool
    cplaceMiddleOfRoom( FloorBSData *f, RoomBSData *r, FurnitureMapStorage& furns, const FurniturePlacementRule& fpd );

    // Furniture script loading
    static FurnitureRuleFunctionContainer functionRules = {
            cplaceMainWith2Sides,
            cplaceCornerWithDec,
            cplaceSetAlignedAtCorner,
            cplaceSetAlignedMiddle,
            cplaceSetBestFit,
            cplacedFirstAvailableCorner,
            cplaceMiddleOfRoom,
    };
    enum FurnitureRuleIndexNames {
        MainWith2Sides,
        CornerWithDec,
        SetAlignedAtCorner,
        SetAlignedMiddle,
        FRBestFit,
        FRFirstAvailableCorner,
        MiddleOfRoom,
    };
    bool runRuleScript( FloorBSData *f, RoomBSData *r, FurnitureMapStorage& furns, const FurnitureRuleScript& fs );

    void addRoomType( RoomBSData *r, ASTypeT rt );

    // Query
    template<typename T>
    bool hasRoomType( const T *r, ASTypeT roomType ) {
        for ( const auto rt : r->roomTypes ) {
            if ( rt == roomType ) return true;
        }
        return false;
    }

    Vector2f maxEnclsingBoundingBoxCenter( const RoomBSData *r );
    size_t numTotalSegments( const RoomBSData *r );
    std::string roomName( const RoomBSData *r );
    std::string roomNames( const RoomBSData *r );
    Vector4f roomColor( const RoomBSData *r );
    std::string roomTypeToName( ASTypeT ast );
    bool isGeneric( const RoomBSData *r );
    bool checkMaxSizeEnclosure( const RoomBSData *r, Vector2f& ep1, Vector2f& ep2, const Vector2f& ncheck );
    bool checkIncludeDoorsWindowsFlag( const ArchSegment *ls, IncludeWindowsOrDoors bwd );
    bool roomNeedsCoving( const RoomBSData *r );
    bool intersectLine2d( const RoomBSData *r, Vector2f const& p0, Vector2f const& p1, Vector2f& i );
    float area( const RoomBSData *r );
    bool isPointInsideRoom( const RoomBSData *r, const V2f& point );
    bool findOppositeWallFromPoint( const RoomBSData *r, const Vector2f& p1, const Vector2f& normal,
                                    std::pair<size_t, size_t>& ret, Vector2f& iPoint,
                                    IncludeWindowsOrDoors bwd = IncludeWindowsOrDoors::None );
    float furnitureAngleFromWall( const ArchSegment *ls );
    float furnitureAngleFromNormal( const Vector2f& normal );
    Vector2f furnitureNormalFromAngle( float angle );
    roomTypeIndex sortedSegmentToPairIndex( const RoomBSData *r, int si );
    roomTypeIndex sortedSegmentToPairIndex( const RoomBSData *r, const ArchSegment *ls );
    ArchSegment *longestSegmentOpposite( const RoomBSData *r );
    const ArchSegment *longestSegmentCornerP1( const RoomBSData *r );
    const ArchSegment *longestSegmentCornerP2( const RoomBSData *r );
    const ArchSegment *longestSegment( const RoomBSData *r );
    const ArchSegment *secondLongestSegment( const RoomBSData *r );
    const ArchSegment *secondShortestSegment( const RoomBSData *r );
    const ArchSegment *thirdLongestSegment( const RoomBSData *r );
    const ArchSegment *shortestSegment( const RoomBSData *r );
    const ArchSegment *secondShortestSegment( const RoomBSData *r );
    const ArchSegment *segmentAtIndex( const RoomBSData *r, uint32_t _index );
    const ArchSegment *segmentAt( const RoomBSData *r, roomTypeIndex rdi );
    const ArchSegment *walkSegment( const RoomBSData *r, const ArchSegment *ls, WalkSegmentDirection wsd );
    float lengthOfArchSegments( const std::vector<const ArchSegment*>& input );
    std::vector<const ArchSegment*>
    walkAlongWallsUntilCornerChanges( const RoomBSData *r, const ArchSegment *ls, WalkSegmentDirection wsd,
                                      IncludeWindowsOrDoors bwd );
    // bsdata getters
    float skirtingDepth( const RoomBSData *r );

}

namespace RS = RoomService;