//
//  htypes.hpp
//  sixthview
//
//  Created by Dado on 01/10/2015.
//
//

#pragma once

#include <cstdint>
#include <utility>
#include <vector>
#include <string>
#include <tuple>
#include <core/math/vector2f.h>

struct ArchBase;
struct ArchStructural;
struct FloorBSData;
struct WallBSData;
struct WindowBSData;
struct UShape;
struct ArchSegment;
struct RoomBSData;
struct HouseBSData;
struct RoomPreData;
struct RoomPreDataResult;
class FurnitureMapStorage;
class FeatureIntersection;

using GenericCallback = std::function<void()>;

[[maybe_unused]] constexpr float inch = 2.54f;
[[maybe_unused]] constexpr float oneOverInch = 1.0f / 2.54f;

namespace MeasureUnitType {
	[[maybe_unused]] constexpr uint64_t Inch = 0;
	[[maybe_unused]] constexpr uint64_t Centimeter = 1;
	[[maybe_unused]] constexpr uint64_t DoNotConvert = 2;
};

enum class WallType {
	Invalid = -1,
	Unknown = 0,
	Interior,
	Exterior
};

enum WallFlags {
	WF_None = 0,
	WF_HasSkirting = 1u,
	WF_HasCoving = 1u << 1u,
	WF_IsDoorPart = 1u << 2u,
	WF_IsWindowPart = 1u << 3u,
};

enum class ArchStructuralFeature {
    ASF_None,
    ASF_Point,
    ASF_Edge,
    ASF_Poly
};

namespace FeatureIntersectionFlags {
    [[maybe_unused]] constexpr uint64_t FIF_Floors          = 1u << 0u;
    [[maybe_unused]] constexpr uint64_t FIF_Walls           = 1u << 1u;
    [[maybe_unused]] constexpr uint64_t FIF_Windows         = 1u << 2u;
    [[maybe_unused]] constexpr uint64_t FIF_Doors           = 1u << 3u;
    [[maybe_unused]] constexpr uint64_t FIF_Ceilings        = 1u << 4u;
    [[maybe_unused]] constexpr uint64_t FIF_Furnitures      = 1u << 5u;
    [[maybe_unused]] constexpr uint64_t FIF_All             = 0xffffffffffffffff;
}

using FeatureIntersectionFlagsT = uint64_t;

struct FloorMatType {
	static const uint64_t Wood = 0;
	static const uint64_t Tiles = 1;
	static const uint64_t TilesOffset = 2;
	static const uint64_t Carpet = 3;
	static const uint64_t Quad = 4;
};

using FloorMatTypeT = uint64_t;

enum class CeilingType {
	Flat,
	Loft,
	KitchenExtension,
	Custom
};

enum class CeilingMatType {
	Plaster
};

enum class WallMatType {
	Plaster,
	Tiles,
	TilesOffset,
};

enum class FloorRectType {
	Ceiling,
	Ground
};

namespace WallLastPointWrap {
	[[maybe_unused]] constexpr bool Yes = true;
	[[maybe_unused]] constexpr bool No = false;
};

using WallLastPointWrapT = bool;

namespace DoorPivotIndex {
	[[maybe_unused]] constexpr uint64_t W1 = 0;
	[[maybe_unused]] constexpr uint64_t W2 = 1;
};

// This contains information on how the door will open, specifically:
// W1 or W2 is where the pivot point is
// CW or CCW, is whether the door will open clockwise or anticlockwise
namespace DoorOrientation {
	[[maybe_unused]] constexpr uint64_t W1_CW = 0;
	[[maybe_unused]] constexpr uint64_t W1_CCW = 1;
	[[maybe_unused]] constexpr uint64_t W2_CW = 2;
	[[maybe_unused]] constexpr uint64_t W2_CCW = 3;
};

namespace ASType {
	[[maybe_unused]] constexpr uint64_t Floor = 0;
	[[maybe_unused]] constexpr uint64_t Wall = 1;
	[[maybe_unused]] constexpr uint64_t Door = 2;
	[[maybe_unused]] constexpr uint64_t Window = 3;
	[[maybe_unused]] constexpr uint64_t Stairs = 4;
	[[maybe_unused]] constexpr uint64_t GenericRoom = 5;
	[[maybe_unused]] constexpr uint64_t LivingRoom = 6;
	[[maybe_unused]] constexpr uint64_t Kitchen = 7;
	[[maybe_unused]] constexpr uint64_t BedroomSingle = 8;
	[[maybe_unused]] constexpr uint64_t BedroomDouble = 9;
	[[maybe_unused]] constexpr uint64_t BedroomMaster = 10;
	[[maybe_unused]] constexpr uint64_t Bathroom = 11;
	[[maybe_unused]] constexpr uint64_t ShowerRoom = 12;
	[[maybe_unused]] constexpr uint64_t ToiletRoom = 13;
	[[maybe_unused]] constexpr uint64_t Conservatory = 14;
	[[maybe_unused]] constexpr uint64_t GamesRoom = 15;
	[[maybe_unused]] constexpr uint64_t Laundry = 16;
	[[maybe_unused]] constexpr uint64_t Hallway = 17;
	[[maybe_unused]] constexpr uint64_t Garage = 18;
	[[maybe_unused]] constexpr uint64_t EnSuite = 19;
	[[maybe_unused]] constexpr uint64_t DiningRoom = 20;
	[[maybe_unused]] constexpr uint64_t Studio = 21;
	[[maybe_unused]] constexpr uint64_t Cupboard = 22;
	[[maybe_unused]] constexpr uint64_t Storage = 23;
	[[maybe_unused]] constexpr uint64_t BoilerRoom = 24;
	[[maybe_unused]] constexpr uint64_t LastRoom = 25; // if you add any room make sure LastRoom is last index + 1
};

using ASTypeT = uint64_t;

enum uShapePairPosition {
	USPP_Start = 1u,
	USPP_End = 1u << 1u,
};

struct uShapeiPointCheck {
	std::vector<int32_t>* sl;
	bool isStartUShape;
	bool isEndUShape;
};

typedef std::pair<int32_t, int32_t> roomTypeIndex;

struct ArchIntersection {
    ArchStructural* arch = nullptr;
    bool hit = false;
    V2f i = V2fc::ZERO;
    V2f p1 = V2fc::ZERO;
    V2f p2 = V2fc::ZERO;
    V2f pn = V2fc::ZERO;
};

namespace ArchRescaleSpace {
    [[maybe_unused]] constexpr uint64_t FloorplanScaling = 1;
    [[maybe_unused]] constexpr uint64_t FullScaling = 2;
}

using ArchRescaleSpaceT = uint64_t ;

struct FloorUShapesPair {
    FloorBSData* f = nullptr;
    UShape* us1 = nullptr;
    UShape* us2 = nullptr;
};

enum ArchType : uint64_t {
	GenericT = 1u,
	WallT = 1u << 1u,
	SkirtingT = 1u << 2u,
	FloorT = 1u << 3u,
	StairStepT = 1u << 4u,
	HandRailT = 1u << 5u,
	StringerT = 1u << 6u,
	Window_SillT = 1u << 7u,
	WindowT = 1u << 8u,
	DoorT = 1u << 9u,
	WallPointT = 1u << 10u,
	DoorAnchorT = 1u << 11u,
	WindowAnchorT = 1u << 12u,
	StairsT = 1u << 13u,
	RoomT = 1u << 14u,
	CurtainT = 1u << 15u,
	CeilingT = 1u << 16u,
	FittedFurnitureT = 1u << 17u
};

using ArchTypeT = uint64_t;

namespace FittedFurnitureFlags {
    [[maybe_unused]] constexpr uint64_t FF_CanOverlap = 1u << 0u;
    [[maybe_unused]] constexpr uint64_t FF_CanBeHanged = 1u << 1u;
    [[maybe_unused]] constexpr uint64_t FF_CanBeDecorated = 1u << 2u;
    [[maybe_unused]] constexpr uint64_t FF_isDecoration = 1u << 3u;
};

using FittedFurnitureFlagsT = uint64_t;

namespace ArchSubType {
    [[maybe_unused]] constexpr int64_t NotApplicable = -1;
    [[maybe_unused]] constexpr int64_t DoorSingle = 0;
    [[maybe_unused]] constexpr int64_t DoorDouble = 1;
};

using ArchSubTypeT = int64_t;

// ************************************************************
// Warning: any changes here needs to update **GHTypeToString**
// ************************************************************
namespace GHType {
	[[maybe_unused]] constexpr uint64_t None = 0u;
	[[maybe_unused]] constexpr uint64_t Generic = 1u;
	[[maybe_unused]] constexpr uint64_t Wall = 1u << 1u;
	[[maybe_unused]] constexpr uint64_t Floor = 1u << 2u;
	[[maybe_unused]] constexpr uint64_t Stairs = 1u << 3u;
	[[maybe_unused]] constexpr uint64_t Window = 1u << 4u;
	[[maybe_unused]] constexpr uint64_t Door = 1u << 5u;
	[[maybe_unused]] constexpr uint64_t DoorRect = 1u << 6u;
	[[maybe_unused]] constexpr uint64_t DoorFrame = 1u << 15u;
	[[maybe_unused]] constexpr uint64_t Ceiling = 1u << 7u;
	[[maybe_unused]] constexpr uint64_t Ground = 1u << 8u;
	[[maybe_unused]] constexpr uint64_t Skirting = 1u << 9u;
	[[maybe_unused]] constexpr uint64_t Coving = 1u << 10u;
	[[maybe_unused]] constexpr uint64_t WallPlaster = 1u << 11u;
	[[maybe_unused]] constexpr uint64_t WallPlasterUShape = 1u << 12u;
	[[maybe_unused]] constexpr uint64_t WallPlasterExternal = 1u << 13u;
	[[maybe_unused]] constexpr uint64_t WallPlasterInternal = 1u << 14u;
	[[maybe_unused]] constexpr uint64_t WallTilesInternal = 1u << 16u;
	[[maybe_unused]] constexpr uint64_t KitchenWorktop = 1u << 17u;
	[[maybe_unused]] constexpr uint64_t KitchenCabinet = 1u << 18u;
	[[maybe_unused]] constexpr uint64_t KitchenSink = 1u << 19u;
	[[maybe_unused]] constexpr uint64_t KitchenOven = 1u << 20u;
	[[maybe_unused]] constexpr uint64_t KitchenHob = 1u << 21u;
	[[maybe_unused]] constexpr uint64_t LightFitting = 1u << 22u;
	[[maybe_unused]] constexpr uint64_t Locator = 1u << 23u;
	[[maybe_unused]] constexpr uint64_t PowerSocket = 1u << 24u;
	[[maybe_unused]] constexpr uint64_t LightSwitch = 1u << 25u;
	[[maybe_unused]] constexpr uint64_t Room = 1u << 26u;
    [[maybe_unused]] constexpr uint64_t KitchenBackSplash = 1u << 27u;
    [[maybe_unused]] constexpr uint64_t Furniture = 1u << 28u;
    [[maybe_unused]] constexpr uint64_t Fitting = 1u << 29u;
};
// ************************************************************
// Warning: any changes here needs to update **GHTypeToString**
// ************************************************************

using GHTypeT = uint64_t;
