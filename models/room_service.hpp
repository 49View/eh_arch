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

JSONDATA(RoomPreData, wallSegmentsInternal, bboxInternal, rtypes)

    RoomPreData( const std::vector<std::vector<ArchSegment>>& wallSegmentsInternal, const Rect2f& bboxInternal,
                 const std::vector<ASTypeT>& _rtypes ) : wallSegmentsInternal(wallSegmentsInternal),
                                                         bboxInternal(bboxInternal),
                                                         rtypes(_rtypes) {}

    std::vector<std::vector<ArchSegment>> wallSegmentsInternal;
    Rect2f bboxInternal;
    std::vector<ASTypeT> rtypes;
};

struct RoomPreDataResult {
    RoomPreDataResult() = default;
    RoomPreDataResult( bool isValidPreRoom ) : isValidPreRoom(isValidPreRoom) {}

    bool isValidPreRoom = true;
    std::vector<RoomPreData> rds{};
};

using RoomPreDataResultContainer = std::vector<RoomPreDataResult>;

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
        FT_CoffeeMachine,
        FT_DiningTable,
        FT_Plant,
        FT_Toilet,
        FT_Shower,
        FT_BathTub,
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

    static std::string name( FurnitureTypeHandler::Type fth ) {
        switch ( fth ) {
            case FT_Bed:
                return "Bed";
            case FT_Bedside:
                return "Bedside";
            case FT_Shelf:
                return "Shelf";
            case FT_Wardrobe:
                return "Wardrobe";
            case FT_Drawer:
                return "Drawer";
            case FT_Carpet:
                return "Carpet";
            case FT_Armchair:
                return "Armchair";
            case FT_Sofa:
                return "Sofa";
            case FT_SideBoard:
                return "SideBoard";
            case FT_TVHanged:
                return "TVHanged";
            case FT_TVWithStand:
                return "TVWithStand";
            case FT_Picture:
                return "Picture";
            case FT_CoffeeTable:
                return "CoffeeTable";
            case FT_DiningTable:
                return "DiningTable";
            case FT_Plant:
                return "Plant";
            case FT_Toilet:
                return "Toilet";
            case FT_Shower:
                return "Shower";
            case FT_BathroomSink:
                return "BathroomSink";
            case FT_BathroomTowerRadiator:
                return "BathroomTowerRadiator";
            case FT_Sink:
                return "Sink";
            case FT_OvenPanel:
                return "OvenPanel";
            case FT_Microwave:
                return "Microwave";
            case FT_Cooktop:
                return "Cooktop";
            case FT_Fridge:
                return "Fridge";
            case FT_ExtractorHood:
                return "ExtractorHood";
            case FT_DrawersHandle:
                return "DrawersHandle";
            case FT_Invalid:
                return "Invalid";
            case Last:
                return "Last";
            case FT_BathTub:
                return "BathTub";
            case FT_CoffeeMachine:
                return "CoffeeMachine";
        }
        return "[MISSING FURNITURE TYPE]";
    }

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

struct FurnitureRuleIgnoreDoorClipping {};
struct FurnitureRuleForceCanOverlap {};
struct FurnitureRuleDoNotClipAgainstRoom {};

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
using FurnitureMultiMap = std::unordered_multimap<FT, FittedFurniture>;
using FurnitureMap = std::unordered_map<FT, FittedFurniture>;

namespace FurnitureRuleFlags {
    constexpr uint64_t None = 0;
    constexpr uint64_t IgnoreDoorClipping = 1u << 0u;
    constexpr uint64_t ForceCanOverlap = 1u << 1u;
    constexpr uint64_t DoNotClipAgainstRoom = 1u << 2u;
}

using FurnitureRuleFlagsT = uint64_t ;

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
        if constexpr ( std::is_same_v<D, FurnitureRuleIgnoreDoorClipping> ) {
            flags |= FurnitureRuleFlags::IgnoreDoorClipping;
            return;
        }
        if constexpr ( std::is_same_v<D, FurnitureRuleForceCanOverlap> ) {
            flags |= FurnitureRuleFlags::ForceCanOverlap;
            return;
        }
        if constexpr ( std::is_same_v<D, FurnitureRuleDoNotClipAgainstRoom> ) {
            flags |= FurnitureRuleFlags::DoNotClipAgainstRoom;
            return;
        }

        LOGR("Houston we have a problem, furniture set rule is bonkers");
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

    [[nodiscard]] FurnitureRuleFlagsT getFlags() const {
        return flags;
    }

    static FurniturePlacementRule randomGeneration();

private:
    FurnitureRuleIndex ruleFunctionIndex = 0;
    FurnitureRefs furnitureRefs{};
    WallSegmentIdentifier wallSegmentId{ WSLOH::Longest() };
    FurnitureSlacks slack{};
    FurnitureRuleFlagsT flags = FurnitureRuleFlags::None;
    WallSegmentCorner preferredCorner = WSC_P1;
};

struct FRPSource {
    std::shared_ptr<FittedFurniture> source;
};

struct FRPWidthNormal {
    V2f widthNormal;
};

struct FRPDepthNormal {
    V2f depthNormal;
};

struct FRPSlack {
    V2f slack;
};

struct FRPSlackScalar {
    float slackScalar = 0.0f;
};

struct FRPWSLO {
    WSLO wslo = WSLO::WSLO_Longest;
};

struct FRPWallSegmentCorner {
    WallSegmentCorner preferredCorner = WSC_P1;
};

struct FRPFurnitureRuleFlags {
    FurnitureRuleFlagsT flags = FurnitureRuleFlags::None;
};

static constexpr FurnitureRuleFlagsT forceManualFurnitureFlags = FurnitureRuleFlags::IgnoreDoorClipping | FurnitureRuleFlags::ForceCanOverlap |
                                             FurnitureRuleFlags::DoNotClipAgainstRoom;

struct FurnitureRuleParams {
    template<typename ...Args>
    explicit FurnitureRuleParams( Args&& ...args ) {
        (initParams( std::forward<Args>( args )), ...); // Fold expression (c++17)
    }

    template <typename T>
    void initParams( const T& param ) {
        if constexpr ( std::is_same_v<std::decay_t<T>, FloorBSData*> ) {
            f = param;
        }
        if constexpr ( std::is_same_v<std::decay_t<T>, RoomBSData*> ) {
            r = param;
        }
        if constexpr ( std::is_same_v<T, const ArchSegment*> ) {
            ls = param;
        }
        if constexpr ( std::is_same_v<T, std::shared_ptr<FittedFurniture>> ) {
            ff = param;
        }
        if constexpr ( std::is_same_v<T, FRPSource> ) {
            source = param.source;
        }
        if constexpr ( std::is_same_v<T, Quaternion> ) {
            rot = param;
        }
        if constexpr ( std::is_same_v<T, FRPWidthNormal> ) {
            widthNormal = param.widthNormal;
        }
        if constexpr ( std::is_same_v<T, FRPDepthNormal> ) {
            depthNormal = param.depthNormal;
        }
        if constexpr ( std::is_same_v<T, FRPSlack> ) {
            slack = param.slack;
        }
        if constexpr ( std::is_same_v<T, FRPSlackScalar> ) {
            slackScalar = param.slackScalar;
        }
        if constexpr ( std::is_same_v<T, V2f> ) {
            pos = param;
        }
        if constexpr ( std::is_same_v<T, float> ) {
            heightOffset = param;
        }
        if constexpr ( std::is_same_v<T, FRPWallSegmentCorner> ) {
            preferredCorner = param.preferredCorner;
        }
        if constexpr ( std::is_same_v<T, PivotPointPosition> ) {
            where = param;
        }
        if constexpr ( std::is_same_v<T, uint32_t> ) {
            exactIndex = param;
        }
        if constexpr ( std::is_same_v<T, FRPWSLO> ) { //WSLO
            wslo = param.wslo;
        }
        if constexpr ( std::is_same_v<T, FRPFurnitureRuleFlags> ) {
            flags = param.flags;
        }
        if constexpr ( std::is_same_v<T, std::vector<std::shared_ptr<FittedFurniture>>> ) {
            furnitureList = param;
        }
    }

    FloorBSData *f = nullptr;
    RoomBSData *r = nullptr;
    std::shared_ptr<FittedFurniture> ff;
    std::shared_ptr<FittedFurniture> source;
    V2f pos = V2fc::ZERO;
    Quaternion rot{};
    V2f widthNormal = V2fc::X_AXIS;
    V2f depthNormal = V2fc::Y_AXIS;
    const ArchSegment *ls = nullptr;
    V2f slack = V2fc::ZERO;
    float slackScalar = 0.0f;
    WallSegmentCorner preferredCorner = WSC_P1;
    float heightOffset = 0.0f;
    PivotPointPosition where = PivotPointPosition::Center;
    uint32_t exactIndex = 0;
    WSLO wslo{WSLO::WSLO_Longest};
    FurnitureRuleFlagsT flags = FurnitureRuleFlags::None;
    std::vector<std::shared_ptr<FittedFurniture>> furnitureList{};
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
    std::shared_ptr<RoomBSData> createRoom( const RoomPreData& _preData, float _floorHeight, float _z, const HouseBSData* house );

    void updateFromArchSegments( RoomBSData *r, const std::vector<std::vector<ArchSegment>>& ws );
    void calcOptimalLightingFittingPositions( RoomBSData *r );
    void addSocketsAndSwitches( RoomBSData *r );
    void calcSkirtingSegments( RoomBSData *r );
    void setCoving( RoomBSData *r, bool _state );
    void changeFloorType( RoomBSData *r, FloorMatTypeT _fmt );
    void assignDefaultRoomFeaturesForType( RoomBSData *r, ASTypeT ast, const HouseBSData* house );
    std::vector<std::vector<Vector2f>> calcCovingSegments( const std::vector<std::vector<ArchSegment>>& ws );

    void calcBBox( RoomBSData *r );
    void rescale( RoomBSData *r, float _scale );

    void addSocketIfSafe( RoomBSData *r, const std::vector<Vector2f>& cov, size_t indexSkirting,
                          float safeSocketBoxWidth );
    void calcLongestWall( RoomBSData *r );
    void WallSegments( RoomBSData *r, const std::vector<std::vector<ArchSegment>>& val );
    void makeTriangles2d( RoomBSData *r );
    void calclMaxBoundingBox( RoomBSData *r );
    void changeWallsMaterial( RoomBSData *r, const MaterialAndColorProperty& mcp );
    void changeFloorsMaterial( RoomBSData *r, const MaterialAndColorProperty& mcp );
    void changeCeilingsMaterial( RoomBSData *r, const MaterialAndColorProperty& mcp );
    void changeSkirtingsMaterial( RoomBSData *r, const MaterialAndColorProperty& mcp );
    void changeCovingsMaterial( RoomBSData *r, const MaterialAndColorProperty& mcp );
    void changeSkirtingsProfile( RoomBSData *r, const MaterialAndColorProperty& mcp );
    void changeCovingsProfile( RoomBSData *r, const MaterialAndColorProperty& mcp );

    void furnish( FloorBSData *f, RoomBSData *r, FurnitureMapStorage& furns );
    const ArchSegment *getWallSegmentFor( RoomBSData *r, WSLO wslo, uint32_t _exactIndex = 0 );
    ArchSegment *segmentAtIndex( RoomBSData *r, uint32_t _index );

    void calculateFurnitureBBox( FittedFurniture* _ff );
    void clearFurniture( RoomBSData *r );
    void addFurniture( RoomBSData *r, std::shared_ptr<FittedFurniture> ff );
    [[nodiscard]] bool addFurniture( FurnitureRuleParams& params );

    // ********************
    // Main furniture rules
    // ********************
    bool placeManually( FurnitureRuleParams params );
    [[nodiscard]] bool placeWallAligned( FurnitureRuleParams params );
    [[nodiscard]] bool placeWallCorner( FurnitureRuleParams params );
    [[nodiscard]] bool placeAround( FurnitureRuleParams params );
    [[nodiscard]] bool placeWallAlong( FurnitureRuleParams params );
    [[nodiscard]] bool placeInBetween( FurnitureRuleParams params );
    // ********************
    // Main furniture rules
    // ********************

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
    [[nodiscard]] bool cplaceSetBestFitHanged( FloorBSData *f, RoomBSData *r, FurnitureMapStorage& furns,
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
            cplaceSetBestFitHanged,
            cplacedFirstAvailableCorner,
            cplaceMiddleOfRoom,
    };
    enum FurnitureRuleIndexNames {
        MainWith2Sides,
        CornerWithDec,
        SetAlignedAtCorner,
        SetAlignedMiddle,
        FRBestFit,
        FRBestFitHanged,
        FRFirstAvailableCorner,
        MiddleOfRoom,
    };
    bool runRuleScript( FloorBSData *f, RoomBSData *r, FurnitureMapStorage& furns, const FurnitureRuleScript& fs );
    FittedFurniture* findFurniture( RoomBSData *r, HashEH furnitureHash );

    void setRoomType( RoomBSData *r, ASTypeT rt, const HouseBSData* house );
    void addRoomType( RoomBSData *r, ASTypeT rt, const HouseBSData* house );
    void removeRoomType( RoomBSData *r, ASTypeT rt );

    // Query
    template<typename T>
    bool hasRoomType( const T *r, ASTypeT roomType ) {
        for ( const auto rt : r->roomTypes ) {
            if ( rt == roomType ) return true;
        }
        return false;
    }

    bool checkBBoxInsideRoom( const RoomBSData* r, const Rect2f& bbox );
    Vector2f maxEnclsingBoundingBoxCenter( const RoomBSData *r );
    size_t numTotalSegments( const RoomBSData *r );
    std::string roomName( const RoomBSData *r );
    std::string roomNames( const RoomBSData *r );
    Vector4f roomColor( const RoomBSData *r );
    std::string roomTypeToName( ASTypeT ast );
    std::string roomTypeToName1to1( ASTypeT ast );
    std::string roomSizeToString( const RoomBSData* r );
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
    bool findOppositeWallFromPointAllowingGap( const RoomBSData *r, const Vector2f& p1, const Vector2f& normal,
                                    std::pair<size_t, size_t>& ret, Vector2f& iPoint,
                                    IncludeWindowsOrDoors bwd = IncludeWindowsOrDoors::None, float allowedGap = 0.0f );
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
    const ArchSegment *segmentAtIndex( const RoomBSData *r, uint32_t _index );
    const ArchSegment *segmentAt( const RoomBSData *r, roomTypeIndex rdi );
    const ArchSegment *walkSegment( const RoomBSData *r, const ArchSegment *ls, WalkSegmentDirection wsd );
    float lengthOfArchSegments( const std::vector<const ArchSegment*>& input );
    std::vector<const ArchSegment*>
    walkAlongWallsUntilCornerChanges( const RoomBSData *r, const ArchSegment *ls, WalkSegmentDirection wsd,
                                      IncludeWindowsOrDoors bwd );
    std::optional<uint64_t> findArchSegmentWithWallHash( RoomBSData *f, HashEH hashToFind, int64_t index );
    // bsdata getters
    float skirtingDepth( const RoomBSData *r );

}

namespace RS = RoomService;