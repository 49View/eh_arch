//
//  House_maker_bitmap.hpp
//  sixthview
//
//  Created by Dado on 16/7/2017.
//
//

#pragma once

#include <string>
#include <memory>
#include <thread>
#include <core/observable.h>
#include <core/raw_image.h>

#include "../wall_calculus.hpp"
#include "../../models/house_bsdata.hpp"
#include "../../models/house_service.hpp"
#include "../../models/floor_service.hpp"
#include "../../models/room_service.hpp"

namespace cv { class Mat; };
namespace JMATH { class Rect2f; };
struct PropertyListing;

enum OCRThresholdMask {
    True, False
};

struct HMBScores {
    uint64_t minWallPixelWidth = 5;
    uint64_t maxWallPixelWidth = 15;
    float roomScore = 0.0f;
    bool allRoomsAreFullyClosed = true;
    int mainWallStrategyIndex = 1;
};

struct RoomOCR {
    cv::Mat maskedRoom;
    std::string allText;
};

struct SourceImages {
    cv::Mat sourceFileImage;
    cv::Mat sourceFileImageOriginalGray;
    cv::Mat sourceFileImageGray;
    cv::Mat sourceFileImageBin;
};

struct WallsEvaluation {
    std::vector<WallEvalData> mWallEvalDataExt;
    std::vector<std::thread> mWallsThreadsExt;
    std::map<std::tuple<int, int, int>, std::vector<std::vector<Vector2f>>> mWallEvaluationMapping;
    WallEvalData mCurrExtEvalWallData;
    int64_t mExtEvalWallThickness = 1;
    float mMaxExternalScore = 0;
};

namespace HouseMakerBitmap {
    const SourceImages& getSourceImages();
    void createSourceDataImage( HouseBSData* house, const PropertyListing& property );
    const SourceImages& prepareImages( HouseBSData *newHouse );
    void rescale( HouseBSData *house, float rescaleFactor, float floorPlanRescaleFactor );
    std::shared_ptr<HouseBSData> makeEmpty( const PropertyListing& property );
    std::shared_ptr<HouseBSData> make( HouseBSData* sourceHouse, const SourceImages& sourceImages, FurnitureMapStorage& furnitureMap );
    std::shared_ptr<HouseBSData> make( HouseBSData* sourceHouse, FurnitureMapStorage& furnitureMap );
    void makeFromWalls( HouseBSData *house );
    void makeAddDoor( HouseBSData *house, const FloorUShapesPair& fus );
    void makeFromSwapDoorOrWindow( HouseBSData *house, HashEH hash );
};
