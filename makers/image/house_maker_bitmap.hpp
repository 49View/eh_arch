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

JSONDATA(HMBBSData, filename, sourceGuassianSigma, sourceGuassianBeta, sourceGuassian, sourceContrast, sourceBrightness,
         minBinThreshold, maxBinThreshold, sourceSharpen, rescaleFactor, maxUShapeLengthRatio,
         minPerimeterLength, pixelCMFromOCR)

    std::string filename{};
    RawImage image = RawImage::WHITE4x4();
    int sourceGuassianSigma = 3; // Must be odd? I think so
    float sourceGuassianBeta = -0.75f;
    float sourceGuassian = 1.75f;
    float sourceContrast = 1.8f;
    float minBinThreshold = 254.0f;
    float maxBinThreshold = 255.0f;
    float sourceBrightness = 30.0f;
    float sourceSharpen = 0.0f;
    float rescaleFactor = 0.01f; // This is the default value for a "normal" floorplan of 1000x1000px in which 1px = 1cm
    float maxUShapeLengthRatio = 1.75f;
    float minPerimeterLength = 1.2f;
    std::vector<float> pixelCMFromOCR;

    HMBBSData( const std::string& filename, const RawImage& image ) : filename(filename), image(image) {}
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
    SourceImages prepareImages( const HMBBSData& bsdata );
    std::shared_ptr<HouseBSData> makeEmpty( HMBBSData &bsdata );
    std::shared_ptr<HouseBSData> make( HMBBSData& mHMBBSData );
    std::shared_ptr<HouseBSData> make( HMBBSData& mHMBBSData, const SourceImages& sourceImages );
    std::shared_ptr<HouseBSData> makeFromWalls( const V2fVectorOfVector& wallPoints, HMBBSData &bsdata, const SourceImages& sourceImages );
};
