//
//  House_maker_bespoke_data.hpp
//  sixthview
//
//  Created by Dado on 16/7/2017.
//
//

#pragma once

#include <string>
#include <memory>

#include "../../models/house_bsdata.hpp"
#include "../../models/house_service.hpp"
#include "../../models/floor_service.hpp"
#include "../../models/room_service.hpp"

template <typename T>
struct SegmentStrip {
    T strip{};
    ArchTypeT type = ArchType::GenericT;
};

using SegmentStrip3d = SegmentStrip<V3fVector>;
using SegmentStrip2d = SegmentStrip<V2fVector>;
using SegmentStripVector3d = std::vector<SegmentStrip3d>;
using SegmentStripVector2d = std::vector<SegmentStrip2d>;

template<typename T>
using TwoShapePair = std::pair<T, T>;

template<typename T>
using TwoShapeVector = std::vector<TwoShapePair<T>>;

struct UshapeBespokeSet {
    UshapeBespokeSet( ArchTypeT type ) : type( type ) {}
    UshapeBespokeSet( ArchTypeT type, const V2f& dw1, const V2f& dw2, float ww ) : type( type ) {
        auto ndw = rotate90(normalize( dw1 - dw2 ));
        us2.points[1] = dw1 + ndw*-ww*0.5f;
        us2.points[2] = dw1 + ndw*ww*0.5f;
        us2.middle = dw1;
        us2.width = ww;
        us2.type = type;

        us1.points[2] = dw2 + ndw*-ww*0.5f;
        us1.points[1] = dw2 + ndw*ww*0.5f;
        us1.middle =  dw2;
        us1.width = ww;
        us1.type = type;
    }

    UShape us1;
    UShape us2;
    ArchTypeT type;
};

struct ArchHouseBespokeData {
    std::vector<V2fVectorOfVector>  floorPoints;
    V3fVector                       bsInnerPerimeter;
    V3fVector                       bsOuterPerimeter;
    V3fVector                       bsMiddlePerimeter;
    std::vector<UshapeBespokeSet>   twoShapePoints;
    float                           wallWidthHint = 0.0;
};

class HouseMakerBespokeData {
public:
	std::shared_ptr<HouseBSData> make( ArchHouseBespokeData&& _data );

private:
	void guessRooms();
	void guessFittings();

private:
//	HMBBSData bsdata;

	std::shared_ptr<HouseBSData> mHouse;
};
