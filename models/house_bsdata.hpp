//
//  HouseBSData.hpp
//  sixthview
//
//  Created by Dado on 05/10/2025.
//
//

#pragma once

#include <utility>
#include <vector>
#include <string>
#include <memory>
#include <array>
#include <ostream>
#include <core/raw_image.h>
#include <core/math/vector4f.h>
#include <core/math/quaternion.h>
#include <core/hashing/incremental_hash.hpp>
#include <core/htypes_shared.hpp>
#include <core/resources/material_and_color_property.hpp>
#include <core/serialization.hpp>
#include <core/names.hpp>
#include <core/math/camera_spatials.hpp>

#include "htypes.hpp"

static const uint64_t SHouseJSONVersion = 2152;

// Version log
//
// 2020-08-23 -    #2152 - Added planeOffset and elevation to ArchStructural
// 2020-08-21 -    #2151 - Added HouseBSData geoCoordinates and elevation
// 2020-08-21 -    #2150 - Added BalconyBSData floorHeight
// 2020-08-20 -    #2149 - Added balconyFloorMaterial
// 2020-08-19 -    #2148 - Adding balconies and removing unnecessary default materials from floors
// 2020-08-17 -    #2147 - Added pos to ArchSpatial
// 2020-08-10 -    #2146 - Remove unnecessary spatial variables from FittedFurniture
// 2020-08-10 -    #2145 - Promoted rotation to ArchSpatial and removed it from FittedFurniture
// 2020-08-10 -    #2144 - First draft of new ArchSpatial intermediate derived struct, to be expended for 2150 probably
// 2020-08-27 -    #2143 - Fixed various clangd warnings and spelling mistakes before it was too late
// 2020-07-29 -    #2142 - Moved northCompassAngle to HouseSourceData, as it's an edit param as original input
// 2020-07-29 -    #2141 - Finally added northCompassAngle
// 2020-07-28 -    #2140 - Added LightFittings class
// 2020-07-24 -    #2134 - Added keyTag and tags to FittedFurniture
// 2020-07-22 -    #2133 - Added dependantHash to FittedFurniture

static const float defaultToBeOverwritten = 7543859749023.0f;

#define MAKE_POLYMORPHIC virtual void nullfunc() {}

JSONDATA(HouseSourceData, floorPlanSourceName, northCompassAngle, sourceGaussianSigma, sourceGaussianBeta,
         sourceGaussian, sourceContrast, sourceBrightness, minBinThreshold, maxBinThreshold, sourceSharpen,
         rescaleFactor, maxUShapeLengthRatio, minPerimeterLength, winningStrategy, winningMargin, pixelCMFromOCR)
    std::string floorPlanSourceName{};

    float northCompassAngle = 0.0f;
    int sourceGaussianSigma = 3; // Must be odd? I think so
    float sourceGaussianBeta = -0.75f;
    float sourceGaussian = 1.75f;
    float sourceContrast = 1.8f;
    float minBinThreshold = 254.0f;
    float maxBinThreshold = 255.0f;
    float sourceBrightness = 30.0f;
    float sourceSharpen = 0.0f;
    float rescaleFactor = 0.01f; // This is the default value for a "normal" floorplan of 1000x1000px in which 1px = 1cm
    float maxUShapeLengthRatio = 1.75f;
    float minPerimeterLength = 1.2f;
    int winningStrategy = -1;
    float winningMargin = 0.0f;
    std::vector<float> pixelCMFromOCR;
};

#define BASE_ELEMENT ArchBase hash, type

struct ArchBase {
    MAKE_POLYMORPHIC
    HashEH hash = HashInc();
    uint64_t type = ArchType::GenericT; // ArchType type;

    template<typename T>
    [[nodiscard]] std::string hashFeature( const std::string& _base, T _sf ) const {
        return std::to_string(hash) + _base + std::to_string(_sf);
    }
};

using SequencePart = int64_t;

struct ArchSpatial : public ArchBase {
public:
    [[nodiscard]] float Width() const;
    [[nodiscard]] float Height() const;
    [[nodiscard]] float Depth() const;

    [[nodiscard]] float HalfWidth() const;
    [[nodiscard]] float HalfHeight() const;
    [[nodiscard]] float HalfDepth() const;

    [[nodiscard]] V3f Position() const;
    [[nodiscard]] V3f PositionReal3d() const;
    [[maybe_unused]] [[nodiscard]] float PositionX() const;
    [[maybe_unused]] [[nodiscard]] float PositionY() const;
    [[maybe_unused]] [[nodiscard]] float PositionZ() const;
    [[nodiscard]] V2f Position2d() const;
    [[nodiscard]] V3f Center() const;
    [[nodiscard]] float Elevation() const;
    [[nodiscard]] V2f PlaneOffset() const;
//    [[nodiscard]] V2f Center2d() const { return centre.xz(); }
    [[nodiscard]] const V3f& Size() const;
    [[nodiscard]] const V3f& Scale() const;

    [[nodiscard]] const Quaternion& Rotation() const;

    [[nodiscard]] const JMATH::Rect2f& BBox() const;
    [[nodiscard]] const JMATH::AABB& BBox3d() const;

    [[maybe_unused]] [[nodiscard]] JMATH::AABB& BBox3dEmergencyWrite();

    [[nodiscard]] const std::vector<Triangle2d>& Triangles2d() const;

    virtual void move( const V3f& _off );
    virtual void move( const V2f& _off );
    virtual void position( const V3f& _pos );
    virtual void position( const V2f& _pos );
    virtual void center( const V3f& _pos );
    virtual void rotate( const Quaternion& _rot );
    virtual void scale( const V3f& _scale );
    virtual void offset( const V2f& );
    virtual void elevate( float );
    virtual void reRoot( float, ArchRescaleSpaceT );

protected:
    [[nodiscard]] float& w();
    [[nodiscard]] float& h();
    [[nodiscard]] float& d();
    [[nodiscard]] V3f& position();
//    [[nodiscard]] V3f& center() { return centre; }
    [[nodiscard]] Quaternion& rot();
    [[nodiscard]] V3f& scale();
    virtual void calcBBox();

private:
    void posBBox();
    void rotateBBox( const Quaternion& _rot );
    void scaleBBox( const V3f& _scale );

protected:
    JMATH::Rect2f bbox = JMATH::Rect2f::INVALID;
    JMATH::AABB bbox3d = JMATH::AABB::INVALID;
    V3f size{ V3f::ZERO };
    V3f pos{ V3f::ZERO };
    V3f centre{ V3f::ZERO };
    Quaternion rotation{ V3f::ZERO, 1.0f };
    Vector3f scaling = Vector3f::ONE;
    V2f planeOffset{ 0.0f,
                     0.0f }; // These are extra offsets required to be able to convert from 2d floorplan (with floors that span the whole image) to a real 3d space (floors stacked on top of each other not sideways as in 2d)
    float elevation = 0.0f; // These are extra offsets required to be able to convert from 2d floorplan (with floors that span the whole image) to a real 3d space (floors stacked on top of each other not sideways as in 2d)
    std::vector<Triangle2d> mTriangles2d;
};

struct ArchStructural : public ArchSpatial {
    std::string linkedUUID{}; // NB This won't get serialized as it's a runtime value only
    Color4f albedo = Color4f::WHITE;
    HashEH linkedHash = 0;
    SequencePart sequencePart = 0;
};

JSONDATA_H(UShape, ArchBase, hash, type, indices, points, edges, middle, inwardNormals, crossNormals, width,
           mIsDetached)
    std::array<int32_t, 4> indices{};
    std::array<Vector2f, 4> points{};
    std::array<Vector2f, 3> edges{};
    std::array<Vector2f, 2> inwardNormals{};
    std::array<Vector2f, 2> crossNormals{};
    Vector2f middle = V2fc::ZERO;
    float width = -1.0f;
    bool mIsDetached = false;
    bool mIsPaired = false;

    void reRoot( float, ArchRescaleSpaceT );
};

struct TwoUShapesBased : public ArchStructural {
    UShape us1;
    UShape us2;
    float thickness = defaultToBeOverwritten;
    Vector2f dirWidth = V2fc::ZERO; // Those are the 2 directions of the element, we know the other one is always up
    Vector2f dirDepth = V2fc::ZERO; // Those are the 2 directions of the element, we know the other one is always up
    float ceilingHeight = 2.75f;
    uint32_t wallFlags = WallFlags::WF_None;

    void reRoot( float, ArchRescaleSpaceT ) override;
    void calcBBox() override;
};

JSONDATA(ArchSegment, iFloor, iWall, iIndex, wallHash, p1, p2, middle, normal, crossNormal, color, tag, sequencePart,
         quads, wallMaterial)
    int32_t iFloor = 0;
    int32_t iWall = 0;
    int32_t iIndex = 0;
    int64_t wallHash = 0;

    Vector2f p1 = V2fc::ZERO;
    Vector2f p2 = V2fc::ZERO;
    Vector2f middle = V2fc::ZERO;

    Vector2f normal = V2fc::ZERO;
    Vector2f crossNormal = V2fc::ZERO;
    C4f color = C4f::WHITE;
    uint64_t tag = 0;
    SequencePart sequencePart = 0;
    std::vector<QuadVector3f> quads;
    MaterialAndColorProperty wallMaterial{};

    friend std::ostream& operator<<( std::ostream& os, const ArchSegment& segment );
    bool operator==( const ArchSegment& rhs ) const;
    bool operator!=( const ArchSegment& rhs ) const;

    [[nodiscard]] float length() const;
    void reRoot( float, ArchRescaleSpaceT );
};

JSONDATA_H(FittedFurniture, ArchStructural, hash, type, bbox, bbox3d, albedo, size, centre, pos, rotation, scaling,
           planeOffset, elevation, mTriangles2d, linkedHash, sequencePart, name, keyTag, tags, symbolRef,
           dependantHashList, widthNormal, depthNormal, flags)
    std::string name;
    std::string keyTag;
    std::vector<std::string> tags;
    std::string symbolRef = S::SQUARE;
    std::vector<HashEH> dependantHashList; // This represents a link between IE a table and few glasses placed over it.
    V2f widthNormal = V2fc::ZERO;
    V2f depthNormal = V2fc::ZERO;
    FittedFurnitureFlagsT flags = 0;

    explicit FittedFurniture( const std::tuple<std::string, AABB>& args, std::string _keyTag, std::string _symbolRef );
    [[nodiscard]] bool checkIf( FittedFurnitureFlagsT _flag ) const;
    void calcBBox() override;
};

JSONDATA_H(DoorBSData, TwoUShapesBased, hash, type, us1, us2, thickness, dirWidth, dirDepth, ceilingHeight, wallFlags,
           bbox, bbox3d, albedo, size, centre, pos, rotation, scaling, planeOffset, elevation, mTriangles2d, linkedHash,
           sequencePart,
           rooms, subType, isMainDoor, isDoorTypicallyShut, architraveProfile,
           dIndex, doorInnerBumpSize, doorGeomThickness, doorTrim, openingAngleMax, openingAngleMin,
           hingesPivot, doorHandlePivotLeft, doorHandlePivotRight, doorHandleAngle, frameHingesPivot,
           doorHandlePlateDoorSidePivot, doorHandlePlateFrameSidePivot, doorPivot, doorHandleRot, doorGeomPivot,
           doorSize)
    std::vector<int64_t> rooms;
    ArchSubTypeT subType = ArchSubType::DoorSingle;
    bool isMainDoor = false;
    bool isDoorTypicallyShut = false;
    std::string architraveProfile{ "architrave,ovolo" };

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

    void calcBBox() override;
    DoorBSData( float _doorHeight, float _ceilingHeight, const UShape& w1, const UShape& w2,
                ArchSubTypeT st = ArchSubType::NotApplicable );
    void reRoot( float, ArchRescaleSpaceT ) override;
};

JSONDATA_H(WindowBSData, TwoUShapesBased, hash, type, us1, us2, thickness, dirWidth, dirDepth, ceilingHeight,
           wallFlags,
           bbox, bbox3d, albedo, size, centre, pos, rotation, scaling, planeOffset, elevation, mTriangles2d, linkedHash,
           sequencePart,
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
    std::string curtainGeom{ "curtain" };
    std::string curtainMaterial{ "curtain02" };

    WindowBSData( float _windowHeight, float _ceilingHeight, float _defaultWindowBaseOffset, const UShape& w1,
                  const UShape& w2, ArchSubTypeT = ArchSubType::NotApplicable );
    void calcBBox() override;
    void reRoot( float, ArchRescaleSpaceT ) override;
};

JSONDATA_H(WallBSData, ArchStructural, hash, type,
           bbox, bbox3d, albedo, size, centre, pos, rotation, scaling, planeOffset, elevation, mTriangles2d, linkedHash,
           sequencePart,
           epoints, enormals, slinesGHType, mUShapes, wallFlags, wrapLastPoint)

    // epoints are needed when the wall structure has got non-simplifiable shape lie boxing or curved walls, in those cases we just save the contours of the wall shape
    std::vector<Vector2f> epoints;
    // normals of pairs of epoints
    std::vector<Vector2f> enormals;
    // Every wall plaster pair render type (starting index hence why it's not std::pair, for optimization only)
    std::vector<uint64_t> slinesGHType;
    // Change of plan again, UShapes are now store per wall!
    std::vector<UShape> mUShapes;

    uint32_t wallFlags = 0;
    WallLastPointWrapT wrapLastPoint = true;

    WallBSData( const std::vector<Vector2f>& epts, float _height, WallLastPointWrapT wlpw = WallLastPointWrap::Yes,
                float _z = 0.0f,
                uint32_t wf = WallFlags::WF_HasSkirting | WallFlags::WF_HasCoving,
                int64_t _linkedHash = 0,
                SequencePart _sequencePart = 0 );
    void reRoot( float, ArchRescaleSpaceT ) override;
    void calcBBox() override;
private:
    void makeTriangles2d();
};

JSONDATA_H(StairsBSData, ArchStructural, hash, type,
           bbox, bbox3d, albedo, size, centre, pos, rotation, scaling, planeOffset, elevation, mTriangles2d, linkedHash,
           sequencePart,
           name)
    std::string name;
};

JSONDATA_H(BalconyBSData, ArchStructural, hash, type,
           bbox, bbox3d, albedo, size, centre, pos, rotation, scaling, planeOffset, elevation, mTriangles2d, linkedHash,
           sequencePart,
           epoints, planeOffset, elevation, floorHeight, balconyFloorMaterial)

    std::vector<Vector2f> epoints{};

    float floorHeight = 0.1f;
    MaterialAndColorProperty balconyFloorMaterial{ "wood,beech" };

    explicit BalconyBSData( const std::vector<Vector2f>& epts );
    void reRoot( float, ArchRescaleSpaceT ) override;
    void calcBBox() override;
private:
    void makeTriangles2d();
};

JSONDATA(KitchenPathFlags, mainSegment, hasCooker, hasSink, hasFridge)
    bool mainSegment = false;
    bool hasCooker = false;
    bool hasSink = false;
    bool hasFridge = false;
};

JSONDATA(KitchenPath, p1, p2, normal, crossNormal, depth, cookerPos, cookerPosDelta, sinkPos, fridgePos, flags)
    V2f p1{ V2fc::ZERO };
    V2f p2{ V2fc::ZERO };
    V2f normal{ V2fc::ZERO };
    V2f crossNormal{ V2fc::ZERO };
    float depth = 0.0f;
    V2f cookerPos = V2fc::ZERO;
    float cookerPosDelta = 0.0f;
    V2f sinkPos = V2fc::ZERO;
    V2f fridgePos = V2fc::ZERO;
    KitchenPathFlags flags{};
    KitchenPath( const V2f& p1, const V2f& p2, const V2f& normal, const V2f& crossNormal, float depth ) :
            p1(p1), p2(p2), normal(normal), crossNormal(crossNormal), depth(depth) {}
};

namespace KitchenDrawerType {
    [[maybe_unused]] constexpr uint64_t Custom = 1;
    [[maybe_unused]] constexpr uint64_t Filler = 2;
    [[maybe_unused]] constexpr uint64_t FlatBasic = 3;
    [[maybe_unused]] constexpr uint64_t FlatVertical25_75 = 4;
};

using KitchenDrawerTypeT = uint64_t;

JSONDATA(KitchenDrawerCustomShapeCol, size, profileName)
    V2f size{ V2fc::ZERO };
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
    explicit KitchenDrawerShape( KitchenDrawerTypeT type = KitchenDrawerType::FlatBasic ) : type(type) {}
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
                                                                                   shape(std::move(shape)),
                                                                                   color(color) {}
};

enum KitchenShape {
    KS_Straight = 0,
    KS_LShape = 1,
    KS_UShape = 2,
    KS_Custom = 3
};

JSONDATA(KitchenData, kitchenWorktopPath, kitchenSkirtingPath, kitchenUnitsPath, kitchenTopUnitsPath, kitchenDrawers,
         kitchenWorktopDepth, kitchenWorktopHeight, worktopThickness, skirtingHeight, kitchenSkirtingRecess,
         kitchenUnitsRecess, kitchenTopUnitsRecess, drawersPadding, drawersThickness, skirtingThickness,
         topUnitsCeilingGap, longDrawersSize, worktopMaterial, unitsMaterial, backSplashMaterial, sinkModel,
         ovenPanelModel, microwaveModel, cooktopModel, fridgeModel, extractorHoodModel, drawersHandleModel,
         kitchenIndexMainWorktop)

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
    MaterialAndColorProperty worktopMaterial{ "marble,anemone" };
    MaterialAndColorProperty unitsMaterial{ "wood,beech" };
    MaterialAndColorProperty backSplashMaterial{ "yule,flemish,tiles" };
    std::string sinkModel{ "ktc,sink,double,chrome" };
    std::string ovenPanelModel{ "ktc,oven,flat" };
    std::string microwaveModel{ "ktc,microwave" };
    std::string cooktopModel{ "ktc,cooktop" };
    std::string fridgeModel{ "ktc,fridge,single" };
    std::string extractorHoodModel{ "ktc,extractor,hood" };
    std::string drawersHandleModel{ "ktc,handle,long,contemporary" };

    // Settings and Indices
    KitchenShape kitchenShape = KS_Straight;
    int kitchenIndexMainWorktop = -1;
};

JSONDATA(LightFittings, key, lightPosition)
    LightFittings( std::string key, const V3f& lightPosition ) : key(std::move(key)), lightPosition(lightPosition) {}
    std::string key;
    V3f lightPosition;
};

JSONDATA_H(RoomBSData, ArchStructural, hash, type,
           bbox, bbox3d, albedo, size, centre, pos, rotation, scaling, planeOffset, elevation, mTriangles2d, linkedHash,
           sequencePart,
           roomTypes, windows, doors, planeOffset, elevation, mHasCoving, mBBoxCoving, floorType, mFittedFurniture,
           mWallSegments, mWallSegmentsSorted, mPerimeterSegments, mvCovingSegments, mvSkirtingSegments,
           mMaxEnclosingBoundingBox, mLightFittings, mSocketLocators, mSwitchesLocators, maxSizeEnclosedHP1,
           maxSizeEnclosedHP2, maxSizeEnclosedWP1, maxSizeEnclosedWP2, mLongestWall, mLongestWallOpposite,
           mLongestWallOppositePoint, mPerimeter, area, mCovingPerimeter, minLightFittingDistance, mArchitravesWidth,
           defaultCeilingThickness, spotLightYOffset, wallsMaterial, floorMaterial, ceilingMaterial, covingProfile,
           skirtingProfile, skirtingMaterial, covingMaterial, spotlightGeom, kitchenData)
    std::vector<ASTypeT> roomTypes{};
    std::vector<int64_t> windows;
    std::vector<int64_t> doors;

    bool mHasCoving = true;
    Rect2f mBBoxCoving = Rect2f::INVALID;
    FloorMatTypeT floorType = 0;
    std::vector<std::shared_ptr<FittedFurniture>> mFittedFurniture;
    std::vector<std::vector<ArchSegment>> mWallSegments;
    std::vector<ArchSegment> mWallSegmentsSorted;
    std::vector<Vector2f> mPerimeterSegments;
    std::vector<std::vector<Vector2f>> mvCovingSegments;
    std::vector<std::vector<Vector2f>> mvSkirtingSegments;
    std::vector<Vector2f> mMaxEnclosingBoundingBox;

    std::vector<LightFittings> mLightFittings;
    std::vector<Vector3f> mSocketLocators; // z on this holds the rotating z-angle
    std::vector<Vector3f> mSwitchesLocators; // z on this holds the rotating z-angle

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
    float mArchitravesWidth = 0.1f;
    float defaultCeilingThickness = 0.02f;
    float spotLightYOffset = 0.5f;
    MaterialAndColorProperty wallsMaterial{ "plaster_ultra_fine_spray", C4f{ 0.93f, 0.91f, 0.89f, 1.0f } };
    MaterialAndColorProperty floorMaterial{ "european,ash" };
    MaterialAndColorProperty ceilingMaterial{ "plaster_ultra_fine_spray", C4f{ 0.99f, 0.99f, 0.99f, 1.0f } };
    MaterialAndColorProperty skirtingMaterial{ S::WHITE_PBR, C4f::WHITE };
    MaterialAndColorProperty covingMaterial{ S::WHITE_PBR, C4f::PASTEL_GRAYLIGHT };
    std::string covingProfile{ "coving,model1" };
    std::string skirtingProfile{ "skirting,kensington" };
    std::string spotlightGeom{ "spotlight_basic" };
    // Ad-hoc room type data, it's a bit redundant but I'll leave it here until I found a better place
    KitchenData kitchenData;

    RoomBSData( const RoomPreData& _preData, float _floorHeight, float _z );
    void reRoot( float, ArchRescaleSpaceT ) override;
    void calcBBox() override;
private:
    void makeTriangles2d();
};

JSONDATA_H(FloorBSData, ArchStructural, hash, type,
           bbox, bbox3d, albedo, size, centre, pos, rotation, scaling, planeOffset, elevation, mTriangles2d, linkedHash,
           sequencePart,
           number, planeOffset, elevation, area, concreteHeight, hasCoving, doorHeight, windowHeight,
           windowBaseOffset, offsetFromFloorAnchor, offsetFromFloorAnchor3d, ceilingContours, mPerimeterSegments,
           perimeterArchSegments, anchorPoint, walls, windows, doors, stairs, rooms, balconies, orphanedUShapes)

    int32_t number = -1; // As in floor number, ground floor = 1, etc...

    float area = 0.0f;
    float concreteHeight = 0.0f;
    bool hasCoving = true;
    float doorHeight = 1.97f;
    float windowHeight = 1.2f;
    float windowBaseOffset = 0.6f;
    Vector2f offsetFromFloorAnchor = V2fc::ZERO;
    Vector3f offsetFromFloorAnchor3d = V3f::ZERO;
    V3fVectorOfVector ceilingContours;
    std::vector<Vector2f> mPerimeterSegments;
    std::vector<ArchSegment> perimeterArchSegments;
    MaterialAndColorProperty externalWallsMaterial{ "plaster_ultra_fine_spray" };

    JMATH::Rect2fFeatureT anchorPoint = Rect2fFeature::bottomRight;

    std::vector<std::shared_ptr<WallBSData>> walls;
    std::vector<std::shared_ptr<WindowBSData>> windows;
    std::vector<std::shared_ptr<DoorBSData>> doors;
    std::vector<std::shared_ptr<StairsBSData>> stairs;
    std::vector<std::shared_ptr<RoomBSData>> rooms;
    std::vector<std::shared_ptr<BalconyBSData>> balconies;
    std::vector<UShape> orphanedUShapes;

    // Debugging only, maybe put on a debug flag or something
    std::vector<ArchSegment> orphanedWallSegments;

    FloorBSData( const JMATH::Rect2f& _rect, int _floorNumber, float _defaultCeilingHeight, float _defaultGroundHeight,
                 float _doorHeight, float _defaultWindowHeight, float _defaultWindowBaseOffset );
    void reRoot( float, ArchRescaleSpaceT ) override;
    void calcBBox() override;
};

JSONDATA_R_H(HouseBSData, ArchStructural, hash, type,
             bbox, bbox3d, albedo, size, centre, pos, rotation, scaling, planeOffset, elevation, mTriangles2d, linkedHash,
             sequencePart,
             version, propertyId, name, source, declaredSQm, defaultSkybox,
             planeOffset, elevation, sourceData, bestInternalViewingPosition, bestInternalViewingAngle,
             bestDollyViewingPosition, bestDollyViewingAngle, walkableArea, doorHeight, defaultWindowHeight,
             defaultWindowBaseOffset, defaultCeilingHeight, windowsillExpansion, windowFrameThickness,
             defaultGroundHeight, worktopHeight, bathRoomSinkHeight, defaultWallColor, accuracy, tourPaths, mFloors)
    uint64_t version = SHouseJSONVersion;
    std::string propertyId;
    std::string name;
    std::string source;
    std::string declaredSQm;
    std::string defaultSkybox;
    HouseSourceData sourceData;
    V3f bestInternalViewingPosition = V3f::ZERO;
    Quaternion bestInternalViewingAngle;
    V3f bestDollyViewingPosition = V3f::ZERO;
    Quaternion bestDollyViewingAngle;
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
    std::vector<CameraPath> tourPaths;
    std::vector<std::shared_ptr<FloorBSData>> mFloors;

public:
    explicit HouseBSData( const JMATH::Rect2f& _floorPlanBBox );
    constexpr static uint64_t Version() { return SHouseJSONVersion; }
    void calcBBox() override;
    void reRoot( float, ArchRescaleSpaceT ) override;
    void reElevate( float _elevation );
    FloorBSData *addFloorFromData( const JMATH::Rect2f& _rect );
};

