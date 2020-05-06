//
//  htypes.hpp
//  sixthview
//
//  Created by Dado on 01/10/2015.
//
//

#pragma once

#include <stdint.h>
#include <utility>
#include <vector>
#include <string>

const static float inch = 2.54f;
const static float inch1o = 1.0f / 2.54f;
struct ArchStructural;

enum class MeasureUnitType {
	Inch = 0,
	Centimeter = 1,
	DoNotConvert
};

enum class WallType {
	Invalid = -1,
	Unknown = 0,
	Interior,
	Exterior
};

enum WallFlags {
	WF_None = 0,
	WF_HasSkirting = 1,
	WF_HasCoving = 1 << 1,
	WF_IsDoorPart = 1 << 2,
	WF_IsWindowPart = 1 << 3,
};

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
	Custon
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
	const static bool Yes = true;
	const static bool No = false;
};

using WallLastPointWrapT = bool;

struct DoorPivotIndex {
	static const uint64_t W1 = 0;
	static const uint64_t W2 = 1;
};

using DoorPivotIndexT = uint64_t;

// This contains information on how the door will open, specifically:
// W1 or W2 is where the pivot point is
// CW or CCW, is whether the door will open clockwise or anticlockwise
struct DoorOrientation {
	static const uint64_t W1_CW = 0;
	static const uint64_t W1_CCW = 1;
	static const uint64_t W2_CW = 2;
	static const uint64_t W2_CCW = 3;
};

using DoorOrientationT = uint64_t;

namespace ASType {
	const static uint64_t Floor = 0;
	const static uint64_t Wall = 1;
	const static uint64_t Door = 2;
	const static uint64_t Window = 3;
	const static uint64_t Stairs = 4;
	const static uint64_t GenericRoom = 5;
	const static uint64_t LivingRoom = 6;
	const static uint64_t Kitchen = 7;
	const static uint64_t BedroomSingle = 8;
	const static uint64_t BedroomDouble = 9;
	const static uint64_t BedroomMaster = 10;
	const static uint64_t Bathroom = 11;
	const static uint64_t ShowerRoom = 12;
	const static uint64_t ToiletRoom = 13;
	const static uint64_t Conservatory = 14;
	const static uint64_t GamesRoom = 15;
	const static uint64_t Laundry = 16;
	const static uint64_t Hallway = 17;
	const static uint64_t Garage = 18;
	const static uint64_t Ensuite = 19;
	const static uint64_t DiningRoom = 20;
	const static uint64_t Studio = 21;
};

using ASTypeT = uint64_t;

static const float z_eps = 0.0f;

enum uShapePairPosition {
	USPP_None = 0,
	USPP_Start = 1,
	USPP_End = 1 << 1,
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
};

enum ArchType : uint64_t {
	GenericT = 1,
	WallT = 1 << 1,
	SkirtingT = 1 << 2,
	FloorT = 1 << 3,
	StairStepT = 1 << 4,
	HandRailT = 1 << 5,
	StringerT = 1 << 6,
	Window_SillT = 1 << 7,
	WindowT = 1 << 8,
	DoorT = 1 << 9,
	WallPointT = 1 << 10,
	DoorAnchorT = 1 << 11,
	WindowAnchorT = 1 << 12,
	StairsT = 1 << 13,
	RoomT = 1 << 14,
	CurtainT = 1 << 15,
	CeilingT = 1 << 16
};

using ArchTypeT = uint64_t;

struct ArchSubType {
    const static int64_t NotApplicable = -1;
    const static int64_t DoorSingle = 0;
    const static int64_t DoorDouble = 1;
};

using ArchSubTypeT = int64_t;

namespace GHType {
	const static uint64_t None = 0;
	const static uint64_t Generic = 1;
	const static uint64_t Wall = 1 << 1;
	const static uint64_t Floor = 1 << 2;
	const static uint64_t Stairs = 1 << 3;
	const static uint64_t Window = 1 << 4;
	const static uint64_t Door = 1 << 5;
	const static uint64_t DoorRect = 1 << 6;
	const static uint64_t DoorFrame = 1 << 15;
	const static uint64_t Ceiling = 1 << 7;
	const static uint64_t Ground = 1 << 8;
	const static uint64_t Skirting = 1 << 9;
	const static uint64_t Coving = 1 << 10;
	const static uint64_t WallPlaster = 1 << 11;
	const static uint64_t WallPlasterUShape = 1 << 12;
	const static uint64_t WallPlasterExternal = 1 << 13;
	const static uint64_t WallPlasterInternal = 1 << 14;
	const static uint64_t WallTilesInternal = 1 << 16;
	const static uint64_t KitchenWorktop = 1 << 17;
	const static uint64_t KitchenCabinet = 1 << 18;
	const static uint64_t KitchenSink = 1 << 19;
	const static uint64_t KitchenOven = 1 << 20;
	const static uint64_t KitchenHob = 1 << 21;
	const static uint64_t LightFitting = 1 << 22;
	const static uint64_t Locator = 1 << 23;
	const static uint64_t PowerSocket = 1 << 24;
	const static uint64_t LightSwitch = 1 << 25;
	const static uint64_t Room = 1 << 26;
};
