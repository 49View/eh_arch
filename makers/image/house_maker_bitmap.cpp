//
//  House_maker_bitmap.cpp
//  sixthview
//
//  Created by Dado on 16/7/2017.
//
//

#include "house_maker_bitmap.hpp"

#include <ocr/ocr.hpp>
#include <core/util.h>
#include <core/profiler.h>
#include <core/string_util.h>
#include <core/math/path_util.h>
#include "../../models/wall_service.hpp"
#include <event_horizon/native/poly/poly_services.hpp>
#include <eh_arch/models/arch_structural_service.hpp>
#include <eh_arch/models/property_list.hpp>
#include <core/file_manager.h>

#include "machine_learning/hu_moments.hpp"

#include "../../models/room_name_mapping.hpp"
#include "opencvutils/cvmatutil.hpp"

SourceImages g_sourceImages;

constexpr size_t NumStrategies = 3;

namespace HouseMakerBitmap {

    void generateSourceFileBC( const RawImage& _ri, SourceImages& si, const HouseSourceData& bsdata ) {
        cv::Mat inputSourcePngImage;

        inputSourcePngImage = decodeRawImageIntoMat(_ri);

        add1PixelWhiteContour(inputSourcePngImage, si.sourceFileImage);
        toGrayScale(si.sourceFileImage, si.sourceFileImageGray);

        cv::Mat sourceFileImageBC;
        cv::Mat lSourceFileImageGray;
        cv::Mat sourceFileImageGraySharpened;

        // Right we re-do this cv::Mat because we need a grayscale of the original source, not the one with the modified brightness/contrast.
        // Otherwise the floor area calculation on the mat will be unstable.
        toGrayScale(si.sourceFileImage, si.sourceFileImageOriginalGray);
        threshold(si.sourceFileImageOriginalGray, si.sourceFileImageOriginalGray, 154.0f, 255.0f, cv::THRESH_BINARY);
//    cv::flip( sourceFileImageOriginalGray, sourceFileImageOriginalGray, 0 );

        float contrast = bsdata.sourceContrast;
        float brightness = bsdata.sourceBrightness;
        si.sourceFileImageGray.convertTo(sourceFileImageBC, -1, contrast, brightness);

        //imshow(std::to_string(random()), sourceFileImage);

        toGrayScale(sourceFileImageBC, sourceFileImageGraySharpened);

        cv::GaussianBlur(sourceFileImageGraySharpened, lSourceFileImageGray, cv::Size(0, 0),
                         bsdata.sourceGaussianSigma);
        cv::addWeighted(sourceFileImageGraySharpened, bsdata.sourceGaussian, lSourceFileImageGray,
                        bsdata.sourceGaussianBeta, 0,
                        lSourceFileImageGray);

        threshold(lSourceFileImageGray, si.sourceFileImageBin, bsdata.minBinThreshold, bsdata.maxBinThreshold,
                  cv::THRESH_BINARY);
    }

    float calcMaxScore( const std::vector<WallEvalData>& wed, int64_t& wt ) {
        float maxScore = 0.0f;
        for ( auto& e : wed ) {
            if ( e.score > maxScore ) {
                maxScore = e.score;
                wt = e.thickness;
            }
        }
        return maxScore;
    }

    void guessNumberOfFloors( HouseBSData *house, const SourceImages& si ) {
        std::vector<JMATH::Rect2f> floorRects = getFloorplanRects(si.sourceFileImageOriginalGray);
        for ( auto& floorRect : floorRects ) {
            house->addFloorFromData(floorRect);
        }
    }

    void gatherWallsData( std::vector<std::vector<Vector2f>>& wall_lines, WallEvalData& wed ) {
        wed.numWalls += wall_lines.size();
        int64_t ni = 0;
        float totalWallsLength = 0.0f;
        float numSLines = 0;
        for ( auto& w : wall_lines ) {
            int csize = static_cast<int>( w.size());
            ni += csize;
            for ( auto t = 0; t < csize; t++ ) {
                totalWallsLength += distance(w[t], w[getCircularArrayIndex(t + 1, csize)]);
                numSLines++;
            }
        }
        wed.numiPoints += ni;
        wed.totalWallsLength += totalWallsLength;
    }

    bool
    checkWindowAndDoorsPatternInArea( HouseBSData *house, const SourceImages& si,
                                      FloorBSData *f, UShape *w1,
                                      UShape *w2, bool forceInsert = true ) {

        if ( w1->type == ArchType::DoorT || w1->type == ArchType::WindowT ) return false;
        if ( w2->type == ArchType::DoorT || w2->type == ArchType::WindowT ) return false;

        ArchType matchedType;

        // Create bounding box that fits the shape (rect) between w1, w2
        JMATH::Rect2f sbox(V2fc::HUGE_VALUE_POS, V2fc::HUGE_VALUE_NEG);
        sbox.expand(w1->points[1]);
        sbox.expand(w1->points[2]);
        sbox.expand(w2->points[1]);
        sbox.expand(w2->points[2]);

        // Create bounding box for doors check, extend it pretty much twice it's length
        JMATH::Rect2f dbox = sbox;

        // This scales 80% on the dominant axis so it takes only the middle part of the windows region, it should then all be mostly straight lines
        dbox.scaleWithClampOnMainAxes(.8f, 0.9f, 0.0f, 0.0f, static_cast<float>( si.sourceFileImageGray.cols ),
                                      static_cast<float>( si.sourceFileImageGray.rows ));

        auto crect = cv::Rect(static_cast<int>( dbox.left()), static_cast<int>( dbox.top()),
                              static_cast<int>( dbox.width()), static_cast<int>( dbox.height()));
        cv::Mat roid;
        si.sourceFileImageGray(crect).copyTo(roid);

//        auto imgStr = std::to_string( random());
//        imshow(imgStr, roid);

//        auto hus = huMomentsOnImage( roid, 84, dbox.dominant() * 0.5f );
        auto husV = HuMomentsService::huMomentsOnImage(roid);
        matchedType = HuMomentsService::isMostlyStraightLines(husV) ? ArchType::WindowT : ArchType::DoorT;

//        auto hus = HuMomentsService::huMomentsOnImageRaw( roid );
//        auto agg = HuMomentsService::compare( huWindows, hus );
//        auto aggDoor = HuMomentsService::compare( huDoors, hus );
//
//        LOGRS( "Distance of hu for " << imgStr << ": " << agg << " Door agg: " << aggDoor );
//        LOGRS( "hus: " << hus[0] << " " << hus[1]<< " " << hus[2]<< " " << hus[3]<< " " << hus[04]<< " " << hus[5]<< " " << hus[6]);
//
//        if ( !forceInsert && (agg > 4.0f && aggDoor > 4.0f) ) {
//            return false;
//        }
//        matchedType = ((aggDoor > agg) || (agg < 4.0)) ? ArchType::WindowT : ArchType::DoorT;

        if ( matchedType != ArchType::GenericT ) {
            if ( matchedType == ArchType::DoorT ) {
                LOGRS("Found Door width: " << w1->width << " middle1: " << w1->middle << " middle2: " << w2->middle);
            }
            if ( matchedType == ArchType::WindowT ) {
                LOGRS("Found Window width: " << w1->width << " middle1: " << w1->middle << " middle2: " << w2->middle);
            }

            w1->type = matchedType;
            w2->type = matchedType;

            if ( matchedType == ArchType::DoorT ) {
                FloorService::addDoorFromData(f, house->doorHeight, *w1, *w2);
            }
            if ( matchedType == ArchType::WindowT ) {
                FloorService::addWindowFromData(f, house->defaultWindowHeight, house->defaultWindowBaseOffset,
                                                *w1,
                                                *w2);
            }
            return true;
        }

        return false;
    }

    void analyseWallForDoorsOrWindows( HouseBSData *house, const SourceImages& si, FloorBSData *f ) {
        auto sp = FloorService::alignSuitableUShapesFromWalls(f);
        for ( const auto& us : sp ) {
            checkWindowAndDoorsPatternInArea(house, si, f, us.first, us.second);
        }
    }

    cv::Mat maskedRoomMat( const SourceImages& si, HouseSourceData& bsdata,
                           const std::vector<std::vector<Vector2f>>& ws,
                           const JMATH::Rect2f _bbox,
                           int resizeFactor, OCRThresholdMask useThreshold, cv::Mat& origMask ) {
        JMATH::Rect2f dbox = _bbox;
        JMATH::Rect2f dboxInv = dbox;//Rect2f( dbox.left(), invTop, dbox.left() + dbox.width(), dbox.height() + invTop );
        dboxInv.shrink(10.f);
        cv::Rect2f crect(dboxInv.left(), dboxInv.top(), dboxInv.width(), dboxInv.height());
        cv::Mat roi(si.sourceFileImageGray, crect);
//    toGrayScale( roi, roi );

        cv::Mat rmat(roi.size(), roi.type());
        roi.copyTo(rmat);

        cv::Mat mask(rmat.size(), CV_8UC1);

        // Create Polygon from vertices
        std::vector<std::vector<cv::Point>> ROI_Poly;
        for ( const auto& covs : ws ) {
            std::vector<cv::Point> np;
            for ( const auto& cov : covs ) {
                Vector2f origP = cov;
                Vector2f p = origP - dboxInv.topLeft();
                np.emplace_back(static_cast<int>( p.x()), static_cast<int>( p.y()));
            }
            ROI_Poly.push_back(np);
        }
        // Fill polygon white
        if ( !ROI_Poly.empty() ) {
            mask.setTo(cv::Scalar(0));
            fillPoly(mask, ROI_Poly, 255);
        } else {
            mask.setTo(cv::Scalar(255));
        }

        // Create new image for result storage
        cv::Mat rmatMasked(rmat.size(), rmat.type());
        rmatMasked.setTo(cv::Scalar(255));

        // Cut out ROI and store it in imageDest
        rmat.copyTo(rmatMasked, mask);


        cv::Mat rmatMasked1PixelContour;
        add1PixelWhiteContour(rmatMasked, rmatMasked1PixelContour);

        origMask = rmatMasked1PixelContour;

        cv::Mat finalImg;// = rmatMasked1PixelContour;
        cv::resize(rmatMasked1PixelContour, finalImg, rmatMasked1PixelContour.size() * resizeFactor, 0, 0,
                   cv::INTER_LANCZOS4);

//	cv::GaussianBlur(rmatMasked1PixelContour, finalImg, cv::Size(0, 0), 3);
//	cv::addWeighted(rmatMasked1PixelContour, 1.5, finalImg, -0.5, 0.5, finalImg);

        if ( useThreshold == OCRThresholdMask::True ) {
            cv::threshold(finalImg, finalImg, 100, 255, cv::THRESH_BINARY);
        }
        return finalImg;

//	return rmatMasked1PixelContour;
    }

    void assignRoomTypeFromOcrString( HouseSourceData& bsdata, const cv::Mat& maskedRoom,
                                      const JMATH::Rect2f& _bboxCoving, std::vector<ASTypeT>& retType ) {
        std::string text = OCR::ocrTextRecognition(maskedRoom);

//        cv::dnn::Net dnnNet;
//        dnnNet = cv::dnn::readNet( "/usr/local/share/opencv4/east/frozen_east_text_detection.pb" );
//        std::string text = OCR::ocrTextDetection( dnnNet, maskedRoom );

        replaceAllStrings(text, "‘", "'");

        std::vector<std::regex> regv;
        regv.emplace_back(R"(([0-9]+\s*'\s*[0-9]+)\s*x?\s*([0-9]+\s*'\s*[0-9]+))");
        regv.emplace_back(
                R"(([0-9]+\.*[0-9]*?)\s*(?:meters|meter|m)?\s*x\s*([0-9]+\.*[0-9]*)?\s*(?:meters|meter|m)?\s*)");
        regv.emplace_back(R"(\(([0-9]+\.*[0-9]*)?\)\s*max?[^\(]+\(([0-9]+\.*[0-9]*)?\)\s*max?)");
        Vector2f lOcrSize = V2fc::ZERO;

        std::istringstream iss(text);
        std::vector<std::string> allWords(( std::istream_iterator<std::string>(iss) ),
                                          std::istream_iterator<std::string>());

        for ( auto& ws : allWords ) {
            std::string w = ws;
            std::transform(w.begin(), w.end(), w.begin(),
                           []( unsigned char c ) { return static_cast<unsigned char>(std::tolower(c)); });
            if ( RoomNameMapping::map.find(w) != RoomNameMapping::map.end() ) {
                auto roomName = RoomNameMapping::map[w];
                LOGRS("RoomName: " << w);
                retType.emplace_back(roomName);
            }
        }

        size_t expectedRegSearch = 3;

        for ( size_t i = 0; i < regv.size(); i++ ) {
            auto txt_regex_mxm = regv[i];
            std::smatch base_match;
            auto textToSearch = text;
            while ( std::regex_search(textToSearch, base_match, txt_regex_mxm) ) {
                auto nM = base_match.size();
                if ( nM == expectedRegSearch ) {
                    bool bValid = true;
                    for ( auto q = 0u; q < expectedRegSearch; q++ ) {
                        if ( base_match[q].str().empty() ) {
                            bValid = false;
                        }
                    }
                    if ( bValid ) {
                        std::string base = base_match[1].str() + " " + base_match[2].str();
                        std::cout << base << " for room \n";
                        if ( i == 0 ) { // Needs to convert from feet and inches
                            float xpos = stringFeetInchesToCm(base_match[1].str());
                            float ypos = stringFeetInchesToCm(base_match[2].str());
                            if ( xpos > 0.0f && ypos > 0.0f ) {
                                lOcrSize = { xpos, ypos };
                            } else {
                                continue;
                            }
                        } else {
                            lOcrSize = { std::stof(base_match[1].str()),
                                         std::stof(base_match[2].str()) };
                        }
                        Vector2f bboRatio = lOcrSize / Vector2f{ _bboxCoving.size().y(), _bboxCoving.size().x() };
                        if ( !isbetween(bboRatio.ratio(), 0.90f, 1.10f) ) {
                            bboRatio = lOcrSize / _bboxCoving.size();
                        }
                        if ( isbetween(bboRatio.ratio(), 0.90f, 1.10f) ) {
                            bsdata.pixelCMFromOCR.push_back(bboRatio.x());
                            bsdata.pixelCMFromOCR.push_back(bboRatio.y());
                        }
                    }
                }
                textToSearch = base_match.suffix().str();
            }
        }

        // If it has found at least a room remove the default generic room
        if ( retType.size() > 1 ) retType.erase(retType.begin());

    }

    void roomOCRScan( const SourceImages& si, HouseSourceData& bsdata, std::vector<RoomPreData>& rws,
                      bool doScaling = true ) {
        // Init OCR, dont worry, the OCR init will done only once per run...
        OCR::ocrInitEngine();
        // OCR for rooms
        for ( auto& r : rws ) {
            auto cs = RoomService::calcCovingSegments(r.wallSegmentsInternal);
            if ( !cs.empty() && !cs[0].empty() ) {
                Rect2f csBBox = getContainingBBox(cs);
                LOGRS(" Room OCR box: " << csBBox);
                cv::Mat origMasked;
                auto maskedRoom = maskedRoomMat(si, bsdata, cs, csBBox, 1, OCRThresholdMask::False, origMasked);
                assignRoomTypeFromOcrString(bsdata, maskedRoom, csBBox, r.rtypes);
//            cv::imwrite(std::to_string(random()) + ".png", maskedRoom);
                //guessHuMomentsOfRoom( origMasked, r );
            }
        }

        // Check if we have to scale the floorplan
        // Scale everything if OCR has detected proper sizes
        auto pcmSize = bsdata.pixelCMFromOCR.size();
        if ( pcmSize > 0 ) {
            float median = 0.0f;
            if ( pcmSize <= 2 ) {
                for ( auto& pcm :  bsdata.pixelCMFromOCR ) {
                    median += pcm;
                }
                median /= static_cast<float>( pcmSize );
            } else {
                std::sort(bsdata.pixelCMFromOCR.begin(), bsdata.pixelCMFromOCR.end(),
                          []( const float& a, const float& b ) -> bool { return a > b; });
                median = lerp(0.5f, bsdata.pixelCMFromOCR[pcmSize / 2],
                              bsdata.pixelCMFromOCR[( pcmSize / 2 ) - 1]);
            }
            auto medianNotStupid = []( float _value ) -> bool {
                return ( _value < 0.1f && _value > 0.001f );
            };
            if ( medianNotStupid(median) ) {
                bsdata.rescaleFactor = median;
            }
            LOGRS("rescaleFactor: " << bsdata.rescaleFactor);
            bsdata.pixelCMFromOCR.clear();
        }
    }

    std::vector<std::vector<Vector2f>>
    evaluateWalls( const SourceImages& si, const Rect2f& _floorBoundaries, int strategyIndex,
                   float thickness ) {
        cv::Mat gray_mw;

        Wall::thicknessThreshold(si.sourceFileImageBin, gray_mw, _floorBoundaries, thickness);
        return Wall::simplifyWalls(gray_mw, strategyIndex, thickness);
    }

    bool checkAllWedCalculated( const std::vector<WallEvalData>& wed ) {
        auto percCalculated = 0u;
        for ( auto& e : wed ) {
            if ( e.elaborated ) ++percCalculated;
        }
        bool allCalculated = percCalculated == wed.size();

        //float fPercCalculated = static_cast<float>( percCalculated ) / static_cast<float>( wed.size() );
        //EvalProgress().externalWalls = fPercCalculated;

        return allCalculated;
    }

    void guessHuMomentsOfRoom( const cv::Mat& src_gray, RoomPreData& r, int thresh ) {

//    auto hus = huMomentsOnImage( src_gray, thresh );
//
//    std::vector<HuMomentsBSData> toilets;
//    readFS( "HuMomentsBSData", "toilets", "type", toilets );
//
//    for ( auto& hm : toilets ) {
//        double c1 = HuMomentsService::compare( hm, hus );
//        if ( c1 < 1.0e-25 ) {
//            r.rtype = ASType::ToiletRoom;
//            FittedFurniture ft;
//            ft.name = "toilet";
//            r.fittedFurnitures.push_back( ft );
//            break;
//        }
//    }
    }

    float polygonArea( const V2fVector& points ) {
        float area = 0.0f;   // Accumulates area
        size_t j = points.size() - 1;

        for ( size_t i = 0; i < points.size(); i++ ) {
            area += ( points[j].x() + points[i].x() ) * ( points[j].y() - points[i].y() );
            j = i;  //j is previous vertex to i
        }
        return area / 2.0f;
    }

    void calculateScores( HouseBSData *house, HMBScores& bsdata, const RoomPreDataResultContainer& rdss ) {

        float maxScoreFloor = 0.0f;
        size_t totalWalls = 0;
        float totalArea = 0.0f;
        constexpr float areaWeight = 0.01f;
        constexpr float roomWeight = 15.0f;
        constexpr float windowsWeight = 10.0f;
        constexpr float doorWeight = 10.0f;
        for ( const auto& f : house->mFloors ) {
            maxScoreFloor += f->windows.size() * windowsWeight;
            maxScoreFloor += f->doors.size() * doorWeight;
        }
        bool allAreClosed = true;
        for ( const auto& rdsc : rdss ) {
            allAreClosed &= rdsc.isValidPreRoom;
            for ( const auto& rr : rdsc.rds ) {
                V2fVector perimeterSegments{};
                float perimeter = 0.0f;
                WallService::perimeterFromSegments(rr.wallSegmentsInternal, perimeterSegments, perimeter);
                float lArea = fabs(polygonArea(perimeterSegments));
                totalArea += lArea;
                LOGRS("Area of room: " << lArea);
            }

            maxScoreFloor += totalArea * areaWeight;
            maxScoreFloor += rdsc.rds.size() * roomWeight;
            for ( const auto& rw : rdsc.rds ) {
                for ( const auto& rww : rw.wallSegmentsInternal ) {
                    totalWalls += rww.size();
                }
            }
        }

        maxScoreFloor += totalWalls > 0.0f ? 1.0f / (float) totalWalls : 0.0f;
        bsdata.roomScore = maxScoreFloor;
        bsdata.allRoomsAreFullyClosed = allAreClosed;
        LOGRS("[MAX_ROOM_SCORE] " << maxScoreFloor);
    }

    RoomPreDataResultContainer guessRooms( HouseBSData *house ) {
        RoomPreDataResultContainer ret;
        for ( auto& f : house->mFloors ) {
            ret.emplace_back(FloorService::roomRecognition(f.get()));
        }
        return ret;
    }

    bool findApproxAreaStrings( const std::string& _val ) {

        std::vector<std::string> singleWords = split(_val, ' ');
        for ( const auto& ws : singleWords ) {
            if ( ws.find("approx") != std::string::npos ) return true;
            if ( ws.find("area") != std::string::npos ) return true;
            if ( ws.find("gross") != std::string::npos ) return true;
            if ( ws.find("internal") != std::string::npos ) return true;
        }

        return false;
    }

    void gatherGeneralTextInformations( HouseBSData *house, const SourceImages& si, HouseSourceData& bsdata ) {
//        float newSqm = -1.0f;
//    std::vector<std::vector<Vector2f>> ws;
//    cv::Mat origMat;
//    auto maskedRoom = maskedRoomMat( si, ws, JMATH::Rect2f( 0, 0, sourceFileImage.cols * bsdata.PixelCM(),
//                                                        sourceFileImage.rows * PixelCM()), 3, OCRThresholdMask::False,
//                                     origMat );
//
//    std::string allText = OCR.setImage( maskedRoom );
//
//    if ( !allText.empty()) {
//        std::string regSQFSQM = "([0-9]+(\\.[0-9]+)*)*\\s+(sq|sqm|sqf|square)(\\s)*(m|ft|meter(s)?)*";
//        std::regex txt_regex_mxm( regSQFSQM );// + regSpace + regAllAlpha + regSQFSQM );
//
//        std::smatch base_match;
//
//        std::stringstream ss( allText );
//        std::string line;
//        std::vector<std::string> allLines;
//        while ( std::getline( ss, line, '\n' )) {
//            std::string origLine = line;
//            std::transform( line.begin(), line.end(), line.begin(),
//                            []( unsigned char c ) { return static_cast<unsigned char>(std::tolower( c )); } );
//            stripUnicode( line );
//            std::regex_search( line, base_match, txt_regex_mxm );
//            if ( base_match.size() == 7 ) {
//                int q = 0;
//                for ( auto& s : base_match ) std::cout << "[" << q++ << "] " << s << std::endl;
//                if ( findApproxAreaStrings( line )) {
//                    float meterFeetScale = 0.092903f; // SQ FT to SQ METERS
//                    if ( base_match[3].str() == "sqm" ) meterFeetScale = 1.0f;
//                    if ( base_match[5].str() == "m" || base_match[5].str() == "meters" ||
//                         base_match[5].str() == "meter" )
//                        meterFeetScale = 1.0f;
//                    try {
//                        newSqm = std::stof( base_match[1].str());
//                    } catch (const std::exception& e) {
//                        continue;
//                    }
//
//                    newSqm *= meterFeetScale;
//                    house->declaredSQm = sqmToString( newSqm );
//                    if ( allLines.size() > 0 ) {
//                        if ( allLines[allLines.size() - 1].size() >
//                             5 ) { // check that the string has a meaningful name, at least 5 chars length
//                            house->name = allLines[allLines.size() - 1];
//                        }
//                    }
//                }
//            }
//            allLines.push_back( origLine );
//        }
//    }
//
//        if ( newSqm > 0.0f && bsdata.medianPCM == 0.0f ) {
//            float houseArea = HouseService::area( house );
//            bsdata.medianPCM = sqrt( newSqm / houseArea );
//        }
    }

    std::mutex g_pages_mutex;

    void
    floorPlanEvaluateWallThickness( HouseBSData *house, const SourceImages& si, WallsEvaluation& weval,
                                    int strategyIndex,
                                    int thickness ) {
        WallEvalData& wed = weval.mWallEvalDataExt[thickness];
        wed.thickness = thickness;

        std::vector<JMATH::Rect2f> stairsCC;

        for ( auto& f : house->mFloors ) {
            auto ws = evaluateWalls(si, f->BBox(), strategyIndex, static_cast<float>( thickness ));
            std::lock_guard<std::mutex> guard(g_pages_mutex);
            weval.mWallEvaluationMapping[std::make_tuple(f->number, strategyIndex, thickness)] = ws;
            gatherWallsData(ws, wed);
        }

        wed.averageWallLength = wed.totalWallsLength / wed.numWalls;
        wed.score = ( wed.numiPoints * wed.averageWallLength ) * ( static_cast<float>( wed.thickness ) );
        wed.elaborated = true;
    }

    void guessWallsByIncrementalSearch( HouseBSData *house, const SourceImages& si, const HMBScores& bsdata ) {
        WallsEvaluation weval;
        WallEvalData wed;
        int minPixelWidth = bsdata.minWallPixelWidth;
        int maxPixelWidth = bsdata.maxWallPixelWidth;

        int bestStrategy = 0;
        float maxScore = 0;
        weval.mMaxExternalScore = 0.0f;
        int q = bsdata.mainWallStrategyIndex;
        weval.mWallEvalDataExt.clear();
        for ( int t = 0; t < maxPixelWidth; t++ ) weval.mWallEvalDataExt.push_back(wed);
        for ( int t = 0; t < minPixelWidth; t++ ) weval.mWallEvalDataExt[t].elaborated = true;

        weval.mWallsThreadsExt.clear();
        for ( auto t = minPixelWidth; t < maxPixelWidth; t++ ) {
            weval.mWallsThreadsExt.emplace_back(
                    [si, house, &weval, t, q]() {
                        floorPlanEvaluateWallThickness(house, si, weval, q, t);
                    });
        }
        std::for_each(weval.mWallsThreadsExt.begin(), weval.mWallsThreadsExt.end(),
                      []( std::thread& t ) { t.join(); });
        weval.mMaxExternalScore = calcMaxScore(weval.mWallEvalDataExt, weval.mExtEvalWallThickness);

        if ( weval.mMaxExternalScore > maxScore ) {
            maxScore = weval.mMaxExternalScore;
            bestStrategy = q;
        }
        LOGRS("[MAXSCORE] " << weval.mMaxExternalScore);

        if ( weval.mExtEvalWallThickness > 0 ) {
            for ( auto& f : house->mFloors ) {
                auto wallPoints = weval.mWallEvaluationMapping[{ static_cast<int>( f->number ),
                                                                 bestStrategy,
                                                                 static_cast<int>( weval.mExtEvalWallThickness ) }];
                FloorService::addWallsFromData(f.get(), wallPoints, WallLastPointWrap::Yes);
            }
        }
    }

    void guessDoorsAndWindowsWithMachineLearning( FloorBSData *f, HouseBSData *house, const SourceImages& si ) {
        analyseWallForDoorsOrWindows(house, si, f);
        FloorService::removeUnPairedUShapes(f);

        if ( f->doors.size() > 2 ) {
            std::vector<std::shared_ptr<DoorBSData>> doorsWidth{};
            for ( auto& door : f->doors ) {
                doorsWidth.emplace_back(door);
            }
            std::sort(doorsWidth.begin(), doorsWidth.end(), []( const auto& l, const auto& r ) -> bool {
                return l->Width() < r->Width();
            });
            for ( const auto& door : doorsWidth ) {
                if ( door->Width() / doorsWidth[doorsWidth.size() / 2 - 1]->Width() > 2.0f ) {
                    FloorService::swapWindowOrDoor(f, house, door->hash);
                }
            }
        }
    }

    RoomPreDataResult guessDoorsWindowsRoomsOnFloor( FloorBSData *f, HouseBSData *house, const SourceImages& si ) {
        guessDoorsAndWindowsWithMachineLearning(f, house, si);
        return FloorService::roomRecognition(f);
    }

    void rollbackStage1TyringToCloseRooms( std::array<std::shared_ptr<HouseBSData>, NumStrategies>& houses,
                                           std::array<RoomPreDataResultContainer, NumStrategies>& rdscs,
                                           const SourceImages& si ) {
        for ( auto t = 0u; t < NumStrategies; t++ ) {
            auto house = houses[t].get();
            for ( auto q = 0u; q < house->mFloors.size(); q++ ) {
                auto rdsc = rdscs[t][q];
                auto f = house->mFloors[q].get();
                // Querying potentially missing ushapes that closes rooms
                for ( auto& seg : f->orphanedUShapes ) {
                    V2f i{ V2fc::ZERO };
                    V2f p1 = seg.middle + seg.crossNormals[0] * 0.01f;
                    V2f p2 = seg.middle + seg.crossNormals[0] * 10000.0f;
                    auto archHit = FloorService::intersectLine2dMin(f, p1, p2, i,
                                                                    WallFlags::WF_IsWindowPart |
                                                                    WallFlags::WF_IsDoorPart);
                    if ( archHit.hit && !FloorService::isInsideRoomRDS(lerp(0.5f, p1, i), rdsc.rds) ) {
                        V2fVector ushapeBump{};
                        ushapeBump.resize(4);
                        float dist = distance(seg.middle, i);
                        float sensibleUShapeBumpHeight = dist - ( seg.width / 5.0f );
                        float finalUShapeBump =
                                sensibleUShapeBumpHeight <= 0.0f ? dist * 0.99f : sensibleUShapeBumpHeight;
                        ushapeBump[2] = seg.points[1] + seg.crossNormals[0] * finalUShapeBump;
                        ushapeBump[1] = seg.points[2] + seg.crossNormals[0] * finalUShapeBump;
                        ushapeBump[3] = seg.points[1] + seg.crossNormals[0] * ( dist + 1.0f );
                        ushapeBump[0] = seg.points[2] + seg.crossNormals[0] * ( dist + 1.0f );
                        auto wallhit = dynamic_cast<WallBSData *>(archHit.arch);
                        if ( wallhit ) {
                            auto newPoints = PolyServices::clipAgainst(wallhit->epoints, ushapeBump,
                                                                       PolyServices::ClipMode::Union);
                            WallService::update(wallhit, newPoints);
                        }
                    }
                }
                FloorService::rollbackToCalculatedWalls(f);
                rdscs[t][q] = guessDoorsWindowsRoomsOnFloor(f, house, si);
            }
        }
    }

    RoomPreDataResultContainer guessDoorsWindowsRooms( HouseBSData *house, const SourceImages& si ) {
        RoomPreDataResultContainer ret;
        for ( auto& f : house->mFloors ) {
            ret.emplace_back(guessDoorsWindowsRoomsOnFloor(f.get(), house, si));
        }
        return ret;
    }

    RoomPreDataResultContainer strategiesLoop( HouseBSData *house, const SourceImages& si, HMBScores& bsscores ) {
        guessNumberOfFloors(house, si);
        guessWallsByIncrementalSearch(house, si, bsscores);
        auto ret = guessDoorsWindowsRooms(house, si);
        calculateScores(house, bsscores, ret);
        return ret;
    }

    std::shared_ptr<HouseBSData> bestScoringHouse( std::array<std::shared_ptr<HouseBSData>, NumStrategies>& houses,
                                                   std::array<HMBScores, NumStrategies>& bsscores ) {
        std::shared_ptr<HouseBSData> house = nullptr;
        float maxScore = -1.0f;
        float winningMargin = 0.0f;
        int winningStrategy = -1;
        for ( auto si = 0u; si < NumStrategies; si++ ) {
            float score = bsscores[si].roomScore;
            if ( score > maxScore ) {
                winningMargin = score - maxScore;
                maxScore = score;
                house = houses[si];
                winningStrategy = si;
            }
        }
        house->sourceData.winningStrategy = winningStrategy;
        house->sourceData.winningMargin = winningMargin;
        return house;
    }

    bool needsRollbacks( const std::array<HMBScores, NumStrategies>& bsscores ) {
        bool isAtLeastARoomOk = false;
        for ( const auto& elem : bsscores ) {
            isAtLeastARoomOk |= elem.allRoomsAreFullyClosed;
        }
        return !isAtLeastARoomOk;
    }

    HouseStrategyResult runWallStrategies( const SourceImages& sourceImages ) {
        PROFILE_BLOCK("Wall strategies");

        std::array<std::shared_ptr<HouseBSData>, NumStrategies> houses{};
        std::array<HMBScores, NumStrategies> bsscores{};
        std::array<RoomPreDataResultContainer, NumStrategies> rdscs{};
        for ( auto t = 0u; t < NumStrategies; t++ ) {
            houses[t] = std::make_shared<HouseBSData>();
            bsscores[t].mainWallStrategyIndex = t;
            rdscs[t] = strategiesLoop(houses[t].get(), sourceImages, bsscores[t]);
        }

        if ( needsRollbacks(bsscores) ) {
            rollbackStage1TyringToCloseRooms(houses, rdscs, sourceImages);
        }

        auto bestHouse = bestScoringHouse(houses, bsscores);
        return { bestHouse, rdscs[bestHouse->sourceData.winningStrategy] };
    }

    void addAndFinaliseRooms( HouseBSData *house, const SourceImages& sourceImages, RoomPreDataResultContainer& rdsc ) {
        for ( auto t = 0u; t < house->mFloors.size(); t++ ) {
            auto f = house->mFloors[t];
            LOGRS("PreRooms count: " << rdsc[t].rds.size());
            roomOCRScan(sourceImages, house->sourceData, rdsc[t].rds);
            FloorService::addRoomsFromData(f.get(), house, rdsc[t].rds);
            FloorService::calcWhichRoomDoorsAndWindowsBelong(f.get(), house);
        }
        erase_if(house->mFloors, []( const auto& elem ) -> bool {
            return !FloorService::hasAnyWall(elem.get());
        });
    }

    const SourceImages& prepareImages( HouseBSData *newHouse, const RawImage& sourceImage ) {
        generateSourceFileBC(sourceImage, g_sourceImages, newHouse->sourceData);
        return g_sourceImages;
    }

    void setHouseSourceDataSection( HouseBSData *dest, const HouseBSData *source ) {
        dest->sourceData = source->sourceData;
        dest->propertyId = source->propertyId;
        dest->name = source->name;
    }


    std::shared_ptr<HouseBSData>
    make( HouseBSData *sourceHouse, const RawImage& sourceImage, FurnitureMapStorage& furnitureMap ) {
        PROFILE_BLOCK("House service elaborate");

        prepareImages(sourceHouse, sourceImage);

        auto[house, rdsc] = runWallStrategies(g_sourceImages);
        addAndFinaliseRooms(house.get(), g_sourceImages, rdsc);

//        gatherGeneralTextInformations( house.get(), g_sourceImages, house->sourceData );
        setHouseSourceDataSection(house.get(), sourceHouse);
        house->reRoot(house->sourceData.rescaleFactor, ArchRescaleSpace::FloorplanScaling);

        HouseService::guessFittings(house.get(), furnitureMap);

        return house;
    }

    std::shared_ptr<HouseBSData>
    make( HouseBSData *sourceHouse, const SourceImages& sourceImages, FurnitureMapStorage& furnitureMap ) {
        PROFILE_BLOCK("House service elaborate");
        g_sourceImages = sourceImages;

        auto[house, rdsc] = runWallStrategies(g_sourceImages);
        addAndFinaliseRooms(house.get(), g_sourceImages, rdsc);

//        gatherGeneralTextInformations( house.get(), g_sourceImages, house->sourceData );
        setHouseSourceDataSection(house.get(), sourceHouse);
        house->reRoot(house->sourceData.rescaleFactor, ArchRescaleSpace::FloorplanScaling);

        HouseService::guessFittings(house.get(), furnitureMap);

        return house;
    }

    void makeFromWalls( HouseBSData *house ) {
        PROFILE_BLOCK("House from wall service elaborate walls housebsdata");
        HouseService::clearHouseExcludingFloorsAndWalls(house);

        house->reRoot( 1.0f / house->sourceData.rescaleFactor, ArchRescaleSpace::FloorplanScaling);
        RoomPreDataResultContainer rdsc = guessDoorsWindowsRooms(house, g_sourceImages);
        addAndFinaliseRooms(house, g_sourceImages, rdsc);
        house->reRoot( house->sourceData.rescaleFactor, ArchRescaleSpace::FloorplanScaling);
    }

    void makeFromWallsWindowsDoors( HouseBSData *house ) {
        PROFILE_BLOCK("House from wall service elaborate walls housebsdata");
        HouseService::clearHouseRooms(house);

        house->reRoot( 1.0f / house->sourceData.rescaleFactor, ArchRescaleSpace::FloorplanScaling);

        RoomPreDataResultContainer rdsc;
        for ( auto& f : house->mFloors ) {
            rdsc.emplace_back(FloorService::roomRecognition( f.get() ));
        }

        addAndFinaliseRooms(house, g_sourceImages, rdsc);
        house->reRoot( house->sourceData.rescaleFactor, ArchRescaleSpace::FloorplanScaling);
    }

    void makeAddDoor( HouseBSData *house, const FloorUShapesPair& fus ) {
        PROFILE_BLOCK("House from wall service elaborate house make door");

        FloorService::addDoorFromData(fus.f, house->doorHeight, *fus.us1, *fus.us2);
        HouseService::clearHouseRooms(house);

        house->reRoot(  1.0f / house->sourceData.rescaleFactor, ArchRescaleSpace::FloorplanScaling);
        auto rdsc = guessRooms(house);
        addAndFinaliseRooms(house, g_sourceImages, rdsc);
        house->reRoot(  house->sourceData.rescaleFactor, ArchRescaleSpace::FloorplanScaling);
    }

    void makeFromSwapDoorOrWindow( HouseBSData *house, HashEH hash ) {
        PROFILE_BLOCK("House from wall service elaborate from swap doors and windows");
        HouseService::swapWindowOrDoor(house, hash);
        HouseService::clearHouseRooms(house);

        house->reRoot(  1.0f / house->sourceData.rescaleFactor, ArchRescaleSpace::FloorplanScaling);
        auto rdsc = guessRooms(house);
        addAndFinaliseRooms(house, g_sourceImages, rdsc);
        house->reRoot(house->sourceData.rescaleFactor, ArchRescaleSpace::FloorplanScaling);
    }

    std::shared_ptr<HouseBSData> makeEmpty( const PropertyListing& property, const RawImage& sourceImage ) {
        auto floorPlanBBox = Rect2f{ V2fc::ZERO, V2f{ sourceImage.width, sourceImage.height } * 0.01f };
        std::shared_ptr<HouseBSData> newHouse = std::make_shared<HouseBSData>(floorPlanBBox);
        newHouse->propertyId = property._id;
        newHouse->name = property.addressLine1 + " " + property.addressLine2 + " " + property.name;
        return newHouse;
    }

    const SourceImages& getSourceImages() {
        return g_sourceImages;
    }

}