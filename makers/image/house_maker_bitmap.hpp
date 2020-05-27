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
    bool  allRoomsAreFullyClosed = true;
    int   mainWallStrategyIndex = 1;
};

JSONDATA( HMBBSData, filename, sourceGuassian, sourceContrast, sourceBrightness, sourceSharpen, pixelCM, pixel1CM, medianPCM,
          maxUShapeLengthRatio, minPerimeterLength, pixelCMFromOCR )

    std::string filename{};
    RawImage image = RawImage::WHITE4x4();
    float sourceGuassian = 1.75f;
    float sourceContrast = 1.8f;
    float sourceBrightness = 30.0f;
    float sourceSharpen = 0.0f;
    float pixelCM = 0.01f;
    float pixel1CM = 100.0f;
    float medianPCM = 0.0f;
    float maxUShapeLengthRatio = 1.75f;
    float minPerimeterLength = 1.2f;
    std::vector<float> pixelCMFromOCR;

    HMBBSData( const std::string& filename, const RawImage& image ) : filename(filename), image(image) {}

    static constexpr float defaultPixelCMValue = 0.01f;

    void resetPCM() {
        pixelCM = defaultPixelCMValue;
        pixel1CM = 1.0f / defaultPixelCMValue;
        medianPCM = 0.0f;
    }

    void PixelCM( float val ) {
        pixelCM = val;
        pixel1CM = 1.0f / pixelCM;
    }

    [[nodiscard]] float PixelCM() const {
        return pixelCM;
    }

    [[nodiscard]] float Pixel1CM() const {
        return pixel1CM;
    }

    static constexpr float defaultPixelCM() {
        return defaultPixelCMValue;
    }

    static constexpr float defaultPixel1CM() {
        return 1.0f / defaultPixelCMValue;
    }

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
    std::shared_ptr<HouseBSData> make( HMBBSData& mHMBBSData );
};
