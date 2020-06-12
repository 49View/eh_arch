//
//  HouseBSData.hpp
//  sixthview
//
//  Created by Dado on 05/10/2025.
//
//

#pragma once

#include <vector>
#include <string>
#include <memory>
#include <array>
#include <ostream>
#include "core/math/vector4f.h"
#include "core/math/quaternion.h"
#include "core/hashing/incremental_hash.hpp"
#include "core/htypes_shared.hpp"
#include "core/serialization.hpp"
#include "core/names.hpp"

#include "htypes.hpp"

static const uint64_t SHouseJSONVersion = 2001;
static const float defaltToBeOverwritten = 7543859749023.0f;

#define MAKE_POLYMORPHIC virtual void nullfunc() {}

JSONDATA(HouseSourceData, floorPlanSize, floorPlanSourceName)
    Vector2f floorPlanSize = V2fc::ZERO;
    Rect2f floorPlanBBox = Rect2f::ZERO;
    std::string floorPlanSourceName{};
};

#define BASE_ELEMENT ArchBase hash, type

struct ArchBase {
    MAKE_POLYMORPHIC
    HashEH hash = HashInc();
    uint64_t type = ArchType::GenericT; // ArchType type;

    template <typename T>
    [[nodiscard]] std::string hashFeature( const std::string& _base, T _sf ) const {
        return std::to_string(hash) + _base + std::to_string(_sf);
    }
};

using SequencePart = int64_t;

#define STRUCTURAL_ELEMENT ArchStructural, asType, bbox, bbox3d, albedo, height, width, depth, center, linkedHash, sequencePart, mTriangles2d

struct ArchStructural : public ArchBase {
    ASTypeT asType = ASType::GenericRoom;
    JMATH::Rect2f bbox = JMATH::Rect2f::IDENTITY;
    JMATH::AABB bbox3d = JMATH::AABB::IDENTITY;
    Color4f albedo = Color4f::WHITE;
    float height = defaltToBeOverwritten;
    float width = defaltToBeOverwritten;
    float depth = defaltToBeOverwritten;
    Vector2f center = V2fc::ZERO;
    int64_t linkedHash = 0;
    SequencePart sequencePart = 0;
    std::vector<Triangle2d> mTriangles2d;

    inline float h() const { return height; }
    inline float w() const { return width; }
    inline float d() const { return depth; }

    inline float hh() const { return h() * 0.5f; }
    inline float wh() const { return w() * 0.5f; }
    inline float dh() const { return d() * 0.5f; }

};


JSONDATA_H(UShape, ArchBase, hash, type, indices, points, edges, middle, inwardNormals, crossNormals, width,
           mIsDetached)
    std::array<int32_t, 4> indices;
    std::array<Vector2f, 4> points;
    std::array<Vector2f, 3> edges;
    std::array<Vector2f, 2> inwardNormals;
    std::array<Vector2f, 2> crossNormals;
    Vector2f middle = V2fc::ZERO;
    float width = -1.0f;
    bool mIsDetached = false;
    bool mIsPaired = false;
};

#define TWOUSHAPEBASED_ELEMENT TwoUShapesBased, us1, us2, thickness, dirWidth, dirDepth, ceilingHeight, wallFlags

struct TwoUShapesBased : public ArchStructural {
    UShape us1;
    UShape us2;
    float thickness = defaltToBeOverwritten;
    Vector2f dirWidth = V2fc::ZERO; // Those are the 2 directions of the element, we know the other one is always up
    Vector2f dirDepth = V2fc::ZERO; // Those are the 2 directions of the element, we know the other one is always up
    float ceilingHeight = 2.75f;
    uint32_t wallFlags = WallFlags::WF_None;
};

JSONDATA(ArchSegment, iFloor, iWall, iIndex, wallHash, p1, p2, middle, normal, crossNormal, color, tag, sequencePart,
         zHeights)
    int32_t iFloor = 0;
    int32_t iWall = 0;
    int32_t iIndex = 0;
    int64_t wallHash = 0;

    Vector2f p1 = V2fc::ZERO;
    Vector2f p2 = V2fc::ZERO;
    Vector2f middle = V2fc::ZERO;

    Vector2f normal = V2fc::ZERO;
    Vector2f crossNormal = V2fc::ZERO;
    C4f      color = C4f::WHITE;
    uint64_t tag = 0;
    SequencePart sequencePart = 0;
    std::vector<V2f> zHeights;

    friend std::ostream& operator<<( std::ostream& os, const ArchSegment& segment ) {
        os << "iFloor: " << segment.iFloor << " iWall: " << segment.iWall << " iIndex: " << segment.iIndex
           << " wallHash: " << segment.wallHash << " p1: " << segment.p1 << " p2: " << segment.p2 << " middle: "
           << segment.middle << " normal: " << segment.normal << " crossNormal: " << segment.crossNormal << " tag: "
           << segment.tag << " sequencePart: " << segment.sequencePart << " zHeights: " << segment.zHeights.size();
        return os;
    }

    bool operator==( const ArchSegment& rhs ) const {
        return iFloor == rhs.iFloor &&
               iWall == rhs.iWall &&
               iIndex == rhs.iIndex &&
               wallHash == rhs.wallHash &&
               p1 == rhs.p1 &&
               p2 == rhs.p2 &&
               middle == rhs.middle &&
               normal == rhs.normal &&
               crossNormal == rhs.crossNormal &&
               tag == rhs.tag &&
               zHeights == rhs.zHeights &&
               sequencePart == rhs.sequencePart;
    }

    bool operator!=( const ArchSegment& rhs ) const {
        return !( rhs == *this );
    }

    void scale( float _factor ) {
        p1 *= _factor;
        p2 *= _factor;
        middle = lerp(0.5f, p1, p2);
    }

    [[nodiscard]] float length() const {
        return distance(p1, p2);
    }
};

enum FittedFurnitureFlags {
    FF_CanOverlap = 1 << 0,
    FF_CanBeHanged = 1 << 1,
    FF_CanBeDecorated = 1 << 2,
    FF_isDecoration = 1 << 3
};

JSONDATA(FittedFurniture, name, symbolRef, size, position3d, xyLocation, heightOffset, rotation, widthNormal,
         depthNormal, bbox3d, flags)
    std::string name;
    std::string symbolRef = S::SQUARE;
    Vector3f size = Vector3f::ONE;
    Vector3f position3d = V3f::ZERO;
    Vector2f xyLocation = V2fc::ZERO;
    float heightOffset = 0.0f;
    Quaternion rotation{ V3f::ZERO, 1.0f };
    V2f widthNormal = V2fc::ZERO;
    V2f depthNormal = V2fc::ZERO;
    JMATH::AABB bbox3d = JMATH::AABB::INVALID;
    int flags = 0;
    explicit FittedFurniture( const std::tuple<std::string, V3f>& args, std::string _symbolRef ) :
            name(std::get<0>(args)), symbolRef(std::move(_symbolRef)), size(std::get<1>(args)) {}
    FittedFurniture( std::string _name, const Vector3f& _size ) : name(std::move(_name)), size(_size) {}
    [[nodiscard]] bool checkIf( FittedFurnitureFlags _flag ) const;
};

JSONDATA_H(DoorBSData, TwoUShapesBased, hash, type, us1, us2, thickness, dirWidth, dirDepth, ceilingHeight, wallFlags,
           asType, bbox, bbox3d, albedo, height, width, depth, center, linkedHash, sequencePart, mTriangles2d,
           rooms, subType, isMainDoor, architraveProfile,
           dIndex, doorInnerBumpSize, doorGeomThickness, doorTrim, openingAngleMax, openingAngleMin,
           hingesPivot, doorHandlePivotLeft, doorHandlePivotRight, doorHandleAngle, frameHingesPivot,
           doorHandlePlateDoorSidePivot, doorHandlePlateFrameSidePivot, doorPivot, doorHandleRot, doorGeomPivot,
           doorSize)
    std::vector<int64_t> rooms;
    ArchSubTypeT subType = ArchSubType::DoorSingle;
    bool isMainDoor = false;
    std::string architraveProfile = "architrave,ovolo";

    // Internal door data
    int dIndex = 1;
    Vector2f doorInnerBumpSize = Vector2f(0.020f, 0.03f);
    float doorGeomThickness = 0.045f;
    float doorTrim = 0.01f;
    float openingAngleMax = M_PI;
    float openingAngleMin = 0.0f;
    float doorHandleAngle = 0.0f;
    Quaternion doorHandleRot{};

    Vector3f hingesPivot = Vector3f::ZERO;
    Vector3f doorHandlePivotLeft = Vector3f::ZERO;
    Vector3f doorHandlePivotRight = Vector3f::ZERO;
    Vector3f frameHingesPivot = Vector3f::ZERO;
    Vector3f doorHandlePlateDoorSidePivot = Vector3f::ZERO;
    Vector3f doorHandlePlateFrameSidePivot = Vector3f::ZERO;
    V3f doorPivot = V3f::ZERO;
    V3f doorGeomPivot = V3f::ZERO;
    V2f doorSize = V2fc::ZERO;
};

JSONDATA_H(WindowBSData, TwoUShapesBased, hash, type, us1, us2, thickness, dirWidth, dirDepth, ceilingHeight,
           wallFlags, asType, bbox, bbox3d, albedo, height, width, depth, center, linkedHash, sequencePart,
           mTriangles2d, rooms, numPanels, sillThickness, mainFrameWidth, minPanelWidth, baseOffset,
           rotOrientation, hasBlinds, hasCurtains, curtainGeom, curtainMaterial)
    std::vector<int64_t> rooms;
    int32_t numPanels = 0;
    float sillThickness = 0.02f;
    float mainFrameWidth = 0.06f;
    float minPanelWidth = 0.06f;
    float baseOffset = 0.2f;
    float rotOrientation = 0.0f;
    bool hasBlinds = false;
    bool hasCurtains = true;
    std::string curtainGeom = "curtain";
    std::string curtainMaterial = "diamante,curtain";
};

JSONDATA_H(WallBSData, ArchStructural, hash, type, asType, bbox, bbox3d, albedo, height, width, depth, center,
           linkedHash, sequencePart, mTriangles2d, epoints, enormals, slinesGHType, mUShapes, z, wallFlags,
           wrapLastPoint)

    // epoints are needed when the wall structure has got non-simplifiable shape lie boxing or curved walls, in those cases we just save the contours of the wall shape
    std::vector<Vector2f> epoints;
    // normals of pairs of epoints
    std::vector<Vector2f> enormals;
    // Every wall plaster pair render type (starting index hence why it's not std::pair, for optimization only)
    std::vector<uint64_t> slinesGHType;
    // Change of plan again, UShapes are now store per wall!
    std::vector<UShape> mUShapes;
    float z = 0.0f;
    uint32_t wallFlags = 0;
    WallLastPointWrapT wrapLastPoint = true;
};

JSONDATA_H(StairsBSData, ArchStructural, hash, type, asType, bbox, bbox3d, albedo, height, width, depth, center,
           linkedHash, sequencePart, mTriangles2d, name)
    std::string name = "";
};

JSONDATA(KitchenPathFlags, mainSegment, hasCooker, hasSink, hasFridge)
    bool mainSegment = false;
    bool hasCooker = false;
    bool hasSink = false;
    bool hasFridge = false;
};

JSONDATA(KitchenPath, p1, p2, normal, crossNormal, depth, cookerPos, cookerPosDelta, sinkPos, fridgePos, flags)
    V2f p1;
    V2f p2;
    V2f normal;
    V2f crossNormal;
    float depth = 0.0f;
    V2f cookerPos = V2fc::ZERO;
    float cookerPosDelta = 0.0f;
    V2f sinkPos = V2fc::ZERO;
    V2f fridgePos = V2fc::ZERO;
    KitchenPathFlags flags{};
    KitchenPath( const V2f& p1, const V2f& p2, const V2f& normal, const V2f& crossNormal, float depth ) : p1(p1),
                                                                                                          p2(p2),
                                                                                                          normal(normal),
                                                                                                          crossNormal(
                                                                                                                  crossNormal),
                                                                                                          depth(depth) {}
};

namespace KitchenDrawerType {
    const static uint64_t Custom = 1;
    const static uint64_t Filler = 2;
    const static uint64_t FlatBasic = 3;
    const static uint64_t FlatVertical25_75 = 4;
};

using KitchenDrawerTypeT = uint64_t;

JSONDATA(KitchenDrawerCustomShapeCol, size, profileName)
    V2f size;
    std::string profileName;
};

JSONDATA(KitchenDrawerCustomShapeRow, cols)
    std::vector<KitchenDrawerCustomShapeCol> cols;
};

JSONDATA(KitchenDrawerCustomShape, rows)
    std::vector<KitchenDrawerCustomShapeRow> rows;
};

JSONDATA(KitchenDrawerShape, type, customShapeGrid)
    KitchenDrawerTypeT type = 0;
    KitchenDrawerCustomShape customShapeGrid{};
    KitchenDrawerShape( KitchenDrawerTypeT type = KitchenDrawerType::FlatBasic ) : type(type) {}
};

JSONDATA(KitchenDrawer, p1, p2, z, unitHeight, depth, normal, shape, color)
    V2f p1 = V2fc::ZERO;
    V2f p2 = V2fc::ZERO;
    float z = 0.0f;
    float unitHeight = 0.7f;
    float depth = 0.3f;
    V2f normal = V2fc::ZERO;
    KitchenDrawerShape shape{ KitchenDrawerType::FlatBasic };
    Color4f color = C4f::WHITE;
    KitchenDrawer( const V2f& p1, const V2f& p2, float z, float unitHeight, float depth, const V2f& normal,
                   KitchenDrawerShape shape, const Color4f& color = C4f::WHITE ) : p1(p1), p2(p2), z(z),
                                                                                   unitHeight(unitHeight),
                                                                                   depth(depth), normal(normal),
                                                                                   shape(shape),
                                                                                   color(color) {}
};

JSONDATA(KitchenData, kitchenWorktopPath, kitchenSkirtingPath, kitchenUnitsPath, kitchenTopUnitsPath, kitchenDrawers,
         kitchenWorktopDepth, kitchenWorktopHeight, worktopThickness, skirtingHeight, kitchenSkirtingRecess,
         kitchenUnitsRecess, kitchenTopUnitsRecess, drawersPadding, drawersThickness, skirtingThickness,
         topUnitsCeilingGap, longDrawersSize, worktopMaterial, unitsMaterial, sinkModel, ovenPanelModel, microwaveModel,
         cooktopModel, fridgeModel, extractorHoodModel, drawersHandleModel,
         mainWorktopIndex)

    std::vector<KitchenPath> kitchenWorktopPath;
    std::vector<KitchenPath> kitchenSkirtingPath;
    std::vector<KitchenPath> kitchenUnitsPath;
    std::vector<KitchenPath> kitchenTopUnitsPath;
    std::vector<KitchenDrawer> kitchenDrawers;

    // kitchen lengths, dimensions, etc...
    float kitchenWorktopDepth = 0.64f;
    float kitchenWorktopHeight = 0.88f;
    float worktopThickness = 0.045f;
    float skirtingHeight = 0.12f;
    float kitchenSkirtingRecess = 0.065f;
    float kitchenUnitsRecess = 0.04f;
    float kitchenTopUnitsRecess = 0.30f;
    Vector2f drawersPadding = V2fc::ONE * .004f;
    float drawersThickness = 0.02f;
    float skirtingThickness = 0.02f;
    float topUnitsCeilingGap = 0.05f;
    Vector2f longDrawersSize = V2f{ 0.6f, 0.9f };

    // Materials
    std::string worktopMaterial = "marble,anemone";
    std::string unitsMaterial = "wood,beech";
    std::string sinkModel = "ktc,sink,double,chrome";
    std::string ovenPanelModel = "ktc,oven,flat";
    std::string microwaveModel = "ktc,microwave";
    std::string cooktopModel = "ktc,cooktop";
    std::string fridgeModel = "ktc,fridge,single";
    std::string extractorHoodModel = "ktc,extractor,hood";
    std::string drawersHandleModel = "ktc,handle,long,contemporary";

    // Indices
    uint64_t mainWorktopIndex = 2;

};

JSONDATA_H(RoomBSData, ArchStructural, hash, type, asType, bbox, bbox3d, albedo, height, width, depth, center,
           linkedHash, sequencePart, mTriangles2d, roomTypes, windows, doors, z, mHasCoving, mBBoxCoving, floorType,
           mFittedFurniture,
           mWallSegments, mWallSegmentsSorted, mPerimeterSegments, mvCovingSegments, mvSkirtingSegments,
           mMaxEnclsingBoundingBox, mLightFittingsLocators, mSocketLocators, mSwichesLocators, maxSizeEnclosedHP1,
           maxSizeEnclosedHP2, maxSizeEnclosedWP1, maxSizeEnclosedWP2, mLongestWall, mLongestWallOpposite,
           mLongestWallOppositePoint, mPerimeter, area, mCovingPerimeter, minLightFittingDistance, mArchiTravesWidth,
           defaultCeilingThickness, wallMaterial, wallColor, floorMaterial, ceilingMaterial, covingProfile,
           skirtingProfile, skirtingMaterial, covingMaterial, skirtingColor, covingColor, spotlightGeom, kitchenData)
    std::vector<ASTypeT> roomTypes{};
    std::vector<int64_t> windows;
    std::vector<int64_t> doors;
    float z = 0.0f;
    bool mHasCoving = true;
    Rect2f mBBoxCoving = Rect2f::INVALID;
    FloorMatTypeT floorType = 0;
    std::vector<FittedFurniture> mFittedFurniture;
    std::vector<std::vector<ArchSegment>> mWallSegments;
    std::vector<ArchSegment> mWallSegmentsSorted;
    std::vector<Vector2f> mPerimeterSegments;
    std::vector<std::vector<Vector2f>> mvCovingSegments;
    std::vector<std::vector<Vector2f>> mvSkirtingSegments;
    std::vector<Vector2f> mMaxEnclsingBoundingBox;

    std::vector<Vector3f> mLightFittingsLocators;
    std::vector<Vector3f> mSocketLocators; // z on this holds the rotating z-angle
    std::vector<Vector3f> mSwichesLocators; // z on this holds the rotating z-angle

    Vector2f maxSizeEnclosedHP1 = V2fc::ZERO;
    Vector2f maxSizeEnclosedHP2 = V2fc::ZERO;
    Vector2f maxSizeEnclosedWP1 = V2fc::ZERO;
    Vector2f maxSizeEnclosedWP2 = V2fc::ZERO;

    int32_t mLongestWall = -1;
    int32_t mLongestWallOpposite = -1;
    Vector2f mLongestWallOppositePoint = V2fc::ZERO;
    float mPerimeter = 0.0f;
    float area = 0.0f;
    float mCovingPerimeter = 0.0f;
    float minLightFittingDistance = 2.0f;
    float mArchiTravesWidth = 0.1f;
    float defaultCeilingThickness = 0.02f;
    std::string wallMaterial = "plaster_ultra_fine_spray";
    C4f wallColor = C4f{ 0.7f, 0.9f, 0.9f, 1.0f };
    std::string floorMaterial = "european,ash";
    std::string ceilingMaterial = "plaster_ultra_fine_spray";
    std::string covingProfile = "coving,model1";
    std::string skirtingProfile = "skirting,kensington";
    std::string skirtingMaterial = S::WHITE_PBR;
    std::string covingMaterial = S::WHITE_PBR;
    C4f skirtingColor = C4f::WHITE;
    C4f covingColor = C4f::WHITE;
    std::string spotlightGeom = "spotlight_basic";
    // Ad-hoc room type data, it's a bit redundant but I'll leave it here until I found a better place
    KitchenData kitchenData;
};

JSONDATA(RoomPreData, wallSegmentsInternal, bboxInternal, rtypes)

    RoomPreData( const std::vector<std::vector<ArchSegment>>& wallSegmentsInternal, const Rect2f& bboxInternal,
                 const std::vector<ASTypeT>& _rtypes ) : wallSegmentsInternal(wallSegmentsInternal),
                                                         bboxInternal(bboxInternal),
                                                         rtypes(_rtypes) {}

    std::vector<std::vector<ArchSegment>> wallSegmentsInternal;
    Rect2f bboxInternal;
    std::vector<ASTypeT> rtypes;
};

JSONDATA_H(FloorBSData, ArchStructural, hash, type, asType, bbox, bbox3d, albedo, height, width, depth, center,
           linkedHash, sequencePart, mTriangles2d, number, z, area, concreteHeight, hasCoving, doorHeight, windowHeight,
           windowBaseOffset, offsetFromFloorAnchor, offsetFromFloorAnchor3d, ceilingContours, mPerimeterSegments,
           perimeterArchSegments, anchorPoint, defaultCeilingMaterial, defaultCeilingColor, externalWallsColor, rds,
           walls, windows, doors, stairs, rooms, orphanedUShapes, orphanedWallSegments)

    int32_t number = -1; // As in floor number, ground floor = 1, etc...
    float z = 0.0f;
    float area = 0.0f;
    float concreteHeight = 0.0f;
    bool hasCoving = false;
    float doorHeight = 1.97f;
    float windowHeight = 1.2f;
    float windowBaseOffset = 0.6f;
    Vector2f offsetFromFloorAnchor = V2fc::ZERO;
    Vector3f offsetFromFloorAnchor3d = V2fc::ZERO;
    V3fVectorOfVector ceilingContours;
    std::vector<Vector2f> mPerimeterSegments;
    std::vector<ArchSegment> perimeterArchSegments;
    std::string defaultCeilingMaterial = "plaster_ultra_fine_spray";
    C4f defaultCeilingColor = C4f::WHITE;
    std::string externalWallsMaterial = "plaster_ultra_fine_spray";
    C4f externalWallsColor = C4f::WHITE;

    JMATH::Rect2fFeatureT anchorPoint = Rect2fFeature::bottomRight;

    std::vector<RoomPreData> rds;

    std::vector<std::shared_ptr<WallBSData>> walls;
    std::vector<std::shared_ptr<WindowBSData>> windows;
    std::vector<std::shared_ptr<DoorBSData>> doors;
    std::vector<std::shared_ptr<StairsBSData>> stairs;
    std::vector<std::shared_ptr<RoomBSData>> rooms;
    std::vector<UShape> orphanedUShapes;
    std::vector<ArchSegment> orphanedWallSegments;
};

JSONDATA_R_H(HouseBSData, ArchStructural, hash, type, asType, bbox, bbox3d, albedo, height, width, depth, center,
             linkedHash, sequencePart, mTriangles2d, version, name, source, declaredSQm, defaultSkybox, sourceData,
             bestInternalViewingPosition, bestInternalViewingAngle, walkableArea,
             doorHeight, defaultWindowHeight, defaultWindowBaseOffset, defaultCeilingHeight, windowsillExpansion,
             windowFrameThickness, defaultGroundHeight, worktopHeight, bathRoomSinkHeight, defaultWallColor, accuracy,
             mFloors)
    uint64_t version = SHouseJSONVersion;
    std::string name = "";
    std::string source = "";
    std::string declaredSQm = "";
    std::string defaultSkybox = "";
    HouseSourceData sourceData;
    V3f bestInternalViewingPosition = V3f::ZERO;
    V3f bestInternalViewingAngle = V3f::ZERO;
    V3f bestDollyViewingPosition = V3f::ZERO;
    V3f bestDollyViewingAngle = V3f::ZERO;
    float walkableArea = 0.0f;
    float doorHeight = 1.94f;
    float defaultWindowHeight = 1.30f;
    float defaultWindowBaseOffset = 0.90f;
    float defaultCeilingHeight = 2.45f;
    float defaultGroundHeight = 0.3f;
    float windowsillExpansion = 0.04f;
    float windowFrameThickness = 0.04f;
    float worktopHeight = 0.9f;
    float bathRoomSinkHeight = 0.9f;
    Color4f defaultWallColor = Color4f::WHITE;
    subdivisionAccuray accuracy = accuracyNone;
    std::vector<std::shared_ptr<FloorBSData>> mFloors;
    inline constexpr static uint64_t Version() { return SHouseJSONVersion; }
};

namespace HouseHints {

    struct WallHints {
        std::vector<const UShape *> uShapesHints{};
        float wallWidthHint = 0.0f;
    };
}
