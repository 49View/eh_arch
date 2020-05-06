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
#include "core/htypes_shared.hpp"
#include "core/serialization.hpp"
#include "core/names.hpp"

#include "htypes.hpp"

static const uint64_t SHouseJSONVersion = 1020;
static const float defaltToBeOverwritten = 7543859749023.0f;

#define MAKE_POLYMORPHIC virtual void nullfunc() {}

JSONDATA( HouseSourceData, floorPlanSize )
	Vector2f floorPlanSize = Vector2f::ZERO;
};

#define BASE_ELEMENT ArchBase hash, type

struct ArchBase {
	MAKE_POLYMORPHIC
	int64_t hash = ++globalHash;
	uint64_t type = ArchType::GenericT; // ArchType type;
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
	Vector2f center = Vector2f::ZERO;
	int64_t linkedHash = 0;
    SequencePart sequencePart = 0;
	std::vector<Triangle2d> mTriangles2d;

	inline float h() const { return height; }
	inline float w() const { return width; }
	inline float d() const { return depth; }

	inline float hh() const { return h()*0.5f; }
	inline float wh() const { return w()*0.5f; }
	inline float dh() const { return d()*0.5f; }

};


JSONDATA_H( UShape, ArchBase
		  , hash, type
		  , indices, points, edges, middle, inwardNormals, crossNormals, width, mIsDetached )
	std::array<int32_t, 4>  indices;
	std::array<Vector2f, 4> points;
	std::array<Vector2f, 3> edges;
	std::array<Vector2f, 2> inwardNormals;
	std::array<Vector2f, 2> crossNormals;
	Vector2f				middle = Vector2f::ZERO;
	float					width = -1.0f;
	bool					mIsDetached = false;
	bool					mIsPaired = false;
};

#define TWOUSHAPEBASED_ELEMENT TwoUShapesBased, us1, us2, thickness, dirWidth, dirDepth, ceilingHeight, wallFlags

struct TwoUShapesBased : public ArchStructural {
	UShape us1;
	UShape us2;
	float thickness = defaltToBeOverwritten;
	Vector2f dirWidth = Vector2f::ZERO; // Those are the 2 directions of the element, we know the other one is always up
	Vector2f dirDepth = Vector2f::ZERO; // Those are the 2 directions of the element, we know the other one is always up
	float    ceilingHeight = 2.75f;
	uint32_t wallFlags = WallFlags::WF_None;
};

JSONDATA( ArchSegment, iFloor, iWall, iIndex, wallHash, p1, p2, middle, normal, crossNormal, tag, sequencePart )
	int32_t iFloor = 0;
	int32_t iWall = 0;
	int32_t iIndex = 0;
	int64_t wallHash = 0;

	Vector2f p1 = Vector2f::ZERO;
	Vector2f p2 = Vector2f::ZERO;
	Vector2f middle = Vector2f::ZERO;

	Vector2f normal = Vector2f::ZERO;
	Vector2f crossNormal = Vector2f::ZERO;
	uint64_t tag = 0;
	SequencePart sequencePart = 0;

    friend std::ostream& operator<<( std::ostream& os, const ArchSegment& segment ) {
        os << "iFloor: " << segment.iFloor << " iWall: " << segment.iWall << " iIndex: " << segment.iIndex
           << " wallHash: " << segment.wallHash << " p1: " << segment.p1 << " p2: " << segment.p2 << " middle: "
           << segment.middle << " normal: " << segment.normal << " crossNormal: " << segment.crossNormal << " tag: "
           << segment.tag << " sequencePart: " << segment.sequencePart;
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
               sequencePart == rhs.sequencePart;
    }

    bool operator!=( const ArchSegment& rhs ) const {
        return !( rhs == *this );
    }
};

enum FittedFurnitureFlags {
    FF_CanOverlap = 1 << 0,
    FF_CanBeHanged = 1 << 1,
    FF_CanBeDecorated = 1 << 2,
    FF_isDecoration = 1 << 3
};

JSONDATA( FittedFurniture, name, symbolRef, size, position3d, xyLocation, heightOffset, rotation, widthNormal, depthNormal, bbox3d, flags )
	std::string name;
    std::string symbolRef = S::SQUARE;
    Vector3f size = Vector3f::ONE;
	Vector3f position3d = V3f::ZERO;
    Vector2f xyLocation = V2f::ZERO;
	float heightOffset = 0.0f;
	Quaternion rotation{ 1.0f, V3f::ZERO};
    V2f widthNormal = V2f::ZERO;
    V2f depthNormal = V2f::ZERO;
	JMATH::AABB bbox3d = JMATH::AABB::INVALID;
	int flags = 0;
	explicit FittedFurniture( const std::tuple<std::string,V3f>& args, std::string  _symbolRef ) :
	name(std::get<0>(args)), symbolRef(std::move(_symbolRef)), size(std::get<1>(args)) {}
	FittedFurniture( std::string _name, const Vector3f& _size ) : name(std::move(_name)), size(_size) {}
	[[nodiscard]] bool checkIf( FittedFurnitureFlags _flag ) const{
        return checkBitWiseFlag( flags, _flag );
	}
	void setFlag( FittedFurnitureFlags _flag ) {
	    orBitWiseFlag( flags, _flag );
    }
};

JSONDATA_H( DoorBSData, TwoUShapesBased
		    , hash, type
			, us1, us2, thickness, dirWidth, dirDepth, ceilingHeight, wallFlags
			, asType, bbox, bbox3d, albedo, height, width, depth, center, linkedHash, sequencePart, mTriangles2d
			, subType, pivotIndex, orientation, doorGeomThickness, architraveWidth, mDoorFrameProfile, openingAngleMax, openingAngleMin, architraveProfile )
 	ArchSubTypeT subType = ArchSubType::DoorSingle;
	DoorPivotIndexT pivotIndex = DoorPivotIndex::W1;
	DoorOrientationT orientation = DoorOrientation::W1_CW;
	float doorGeomThickness = 0.045f;
	float architraveWidth = 0.1f;
	std::string mDoorFrameProfile = ""; //Profile mDoorFrameProfile;
	float openingAngleMax = M_PI;
	float openingAngleMin = 0.0f;
    std::string architraveProfile = "architrave,ovolo";
};

JSONDATA_H( WindowBSData, TwoUShapesBased
		  , hash, type
		  , us1, us2, thickness, dirWidth, dirDepth, ceilingHeight, wallFlags
		  , asType, bbox, bbox3d, albedo, height, width, depth, center, linkedHash, sequencePart, mTriangles2d
		  , numPanels, sillThickness, mainFrameWidth, minPanelWidth, baseOffset, insideRoomPointingNormal, curtainGeom
		  , curtainMaterial )
	int32_t numPanels = 0;
	float sillThickness = 0.02f;
	float mainFrameWidth = 0.06f;
	float minPanelWidth = 0.06f;
	float baseOffset = 0.2f;
	V2f   insideRoomPointingNormal = V2f::ZERO;
    std::string curtainGeom = "curtain";
    std::string curtainMaterial = "diamante,curtain";
};

JSONDATA_H( WallBSData, ArchStructural
		  , hash, type
		  , asType, bbox, bbox3d, albedo, height, width, depth, center, linkedHash, sequencePart, mTriangles2d
		  , epoints, enormals, slinesGHType, mUShapes, z, wallFlags, wrapLastPoint, material, color )

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
    std::string material = "plaster_spanish";
    std::string color = "#D0D0D0";
};

JSONDATA_H( StairsBSData, ArchStructural
		  , hash, type
		  , asType, bbox, bbox3d, albedo, height, width, depth, center, linkedHash, sequencePart, mTriangles2d
		  , name )
	std::string name = "";
};

JSONDATA_H( RoomBSData, ArchStructural
		  , hash, type
		  , asType, bbox, bbox3d, albedo, height, width, depth, center, linkedHash, sequencePart, mTriangles2d
		  , z, mHasCoving, mBBoxCoving, floorType, mFittedFurniture, mWallSegments, mWallSegmentsSorted, mPerimeterSegments
		  , mvCovingSegments, mvSkirtingSegments, mMaxEnclsingBoundingBox, mLightFittingsLocators, mSocketLocators, mSwichesLocators
		  , maxSizeEnclosedHP1, maxSizeEnclosedHP2, maxSizeEnclosedWP1, maxSizeEnclosedWP2, mLongestWall, mLongestWallOpposite
		  , mLongestWallOppositePoint, mPerimeter, mCovingPerimeter, minLightFittingDistance, mArchiTravesWidth, defaultCeilingThickness
		  , ceilingMaterial, covingProfile, skirtingProfile, spotlightGeom )
	float z = 0.0f;
	bool mHasCoving = true;
	Rect2f  mBBoxCoving = Rect2f::INVALID;
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

	Vector2f maxSizeEnclosedHP1 = Vector2f::ZERO;
	Vector2f maxSizeEnclosedHP2 = Vector2f::ZERO;
	Vector2f maxSizeEnclosedWP1 = Vector2f::ZERO;
	Vector2f maxSizeEnclosedWP2 = Vector2f::ZERO;

	int32_t mLongestWall = -1;
	int32_t mLongestWallOpposite = -1;
	Vector2f mLongestWallOppositePoint = Vector2f::ZERO;
	float mPerimeter = 0.0f;
	float mCovingPerimeter = 0.0f;
	float minLightFittingDistance = 2.0f;
	float mArchiTravesWidth = 0.1f;
	float defaultCeilingThickness = 0.02f;
    std::string floorMaterial = "european,ash";
    std::string ceilingMaterial = "plaster_ultra_fine_spray";
    std::string covingProfile = "coving,model1";
    std::string skirtingProfile = "skirting,kensington";
    std::string spotlightGeom = "spotlight_basic";
};

JSONDATA( RoomPreData, wallSegmentsInternal, bboxInternal, rtype )

    RoomPreData( const std::vector<std::vector<ArchSegment>>& wallSegmentsInternal, const Rect2f& bboxInternal,
                 ASTypeT rtype ) : wallSegmentsInternal( wallSegmentsInternal ), bboxInternal( bboxInternal ),
                                   rtype( rtype ) {}

    std::vector<std::vector<ArchSegment>> wallSegmentsInternal;
    Rect2f bboxInternal;
    ASTypeT rtype;
};

JSONDATA_H( FloorBSData, ArchStructural
		  , hash, type
		  , asType, bbox, bbox3d, albedo, height, width, depth, center, linkedHash, sequencePart, mTriangles2d
		  , number, z, concreteHeight, hasCoving, doorHeight, windowHeight, windowBaseOffset, offsetFromFloorAnchor
		  , offsetFromFloorAnchor3d, ceilingContours, mPerimeterSegments, anchorPoint, defaultCeilingMaterial, rds
		  , walls, windows, doors, stairs, rooms, orphanedUShapes )

	int32_t number = -1; // As in floor number, ground floor = 1, etc...
	float z = 0.0f;
	float concreteHeight = 0.0f;
	bool  hasCoving = false;
	float doorHeight = 1.97f;
	float windowHeight = 1.2f;
	float windowBaseOffset = 0.6f;
	Vector2f offsetFromFloorAnchor = Vector2f::ZERO;
	Vector3f offsetFromFloorAnchor3d = Vector2f::ZERO;
	V3fVectorOfVector ceilingContours;
    std::vector<Vector2f> mPerimeterSegments;
    std::string defaultCeilingMaterial = "plaster_ultra_fine_spray";

	JMATH::Rect2fFeatureT anchorPoint = Rect2fFeature::bottomRight;

    std::vector<RoomPreData> rds;

	std::vector<std::shared_ptr<WallBSData>> walls;
	std::vector<std::shared_ptr<WindowBSData>> windows;
	std::vector<std::shared_ptr<DoorBSData>> doors;
	std::vector<std::shared_ptr<StairsBSData>> stairs;
	std::vector<std::shared_ptr<RoomBSData>> rooms;
    std::vector<UShape> orphanedUShapes;

};

JSONDATA_R_H( HouseBSData, ArchStructural, hash, type
		  , asType, bbox, bbox3d, albedo, height, width, depth, center, linkedHash, sequencePart, mTriangles2d
		  , version, name, source, declaredSQm, defaultSkybox, sourceData, doorHeight, defaultWindowHeight, defaultWindowBaseOffset
          , defaultCeilingHeigh, windowsillExpansion, windowFrameThickness, defaultGroundHeight, worktopHeight
		  , bathRoomSinkHeight, defaultWallColor, accuracy, mFloors )
	uint64_t version = SHouseJSONVersion;
	std::string name = "";
	std::string source = "";
	std::string declaredSQm = "";
	std::string defaultSkybox = "";
	HouseSourceData sourceData;
	float doorHeight = 1.97f;
	float defaultWindowHeight = 2.20f;
	float defaultWindowBaseOffset = 0.20f;
	float defaultCeilingHeigh = 2.75f;
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
        std::vector<const UShape*> uShapesHints{};
        float wallWidthHint = 0.0f;
    };
}
