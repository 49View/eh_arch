#include "wall_calculus.hpp"

#include <core/htypes_shared.hpp>
#include <core/math/triangulator.hpp>

static const float aaDelta = sqrtf( 25.0f + 25.0f );

bool isParallelWithLines( std::vector<Vector2f>& normals, Vector2f& source ) {
    for ( auto n : normals ) {
        float dotp = dot( source, n );
        if ( isScalarEqual( fabs( dotp ), 1.0f, 0.0001f ))
            return true;
    }
    return false;
}

int Wall::getSolidAround( const cv::Mat& frame, int x, int y, int n ) {
    int32_t colb;
    int32_t coln;
    int32_t consZeros = 0;
    int32_t x1, y1;

    if ( x - n - 1 < 0 ) return -1;
    if ( y - n - 1 < 0 ) return -1;
    if ( x + n >= frame.cols ) return -1;
    if ( y + n >= frame.rows ) return -1;

    colb = frame.at<int8_t>( x - n - 1, y );
    for ( x1 = x - n; x1 <= x + n; x1++ ) {
        coln = frame.at<int8_t>( x1, y );
        if ( colb == coln && coln == 0 ) {
            consZeros++;
            if ( consZeros == n ) break;
        } else {
            consZeros = 0;
        }
        colb = coln;
    }

    if ( consZeros < n ) return -1;

    consZeros = 0;
    colb = frame.at<int8_t>( x, y - n - 1 );
    for ( y1 = y - n; y1 <= y + n; y1++ ) {
        coln = frame.at<int8_t>( x, y1 );
        if ( colb == coln && coln == 0 ) {
            consZeros++;
            if ( consZeros == n ) break;
        } else {
            consZeros = 0;
        }
        colb = coln;
    }

    if ( consZeros < n ) return -1;

    return 0;
}

void getEmptyLinesFromFloorPlan( std::vector<int>& emptyCols, const cv::Mat& frame, int startRow, int endRow, int x,
                                 bool& prevWasBlack, int lastCol, bool rowMajor ) {
    int32_t colb = 0;
    bool hasGotBlack = false;
    for ( int y = startRow; y < endRow; y++ ) {
        colb = rowMajor ? frame.at<int8_t>( y, x ) : frame.at<int8_t>( x, y );
        if ( colb == 0 ) {
            hasGotBlack = true;
            prevWasBlack = true;
            break;
        }
    }
    if ( !hasGotBlack ) {
        if ( emptyCols.size() > 0 && emptyCols.back() == x - 1 ) emptyCols.pop_back();
        // We add an extra index here to "end" the vertical pair so we can loop them 2 by 2 later on and also it get the exact boundaries for the rect
        if ( emptyCols.size() > 0 && prevWasBlack ) {
            emptyCols.push_back( x );
            prevWasBlack = false;
        }
        // Remove the last col as it's probably the last row
        if ( x != lastCol ) emptyCols.push_back( x );
    }
}

void addFloorRectIfBigEnough( const JMATH::Rect2f& candidate, float fullscreenAreaThreshold,
                              std::vector<JMATH::Rect2f>& rects ) {
    float candidateArea = candidate.area();
    if ( candidateArea > fullscreenAreaThreshold ) {
        rects.push_back( candidate );
    }
}

std::vector<JMATH::Rect2f> getFloorplanRects( const cv::Mat& frame, const float scale ) {
    std::vector<JMATH::Rect2f> rects;
    std::vector<int> emptyCols;
    std::vector<int> emptyRows;
    bool prevWasBlack = false;
    JMATH::Rect2f fullScreen = Rect2f( 0.0f, 0.0f, static_cast<float>( frame.cols ), static_cast<float>( frame.rows ));
    float fullscreenAreaThreshold = fullScreen.area() * 0.05f;

    // Ok ladies and gents, lets gather the vertical empty lines first
    for ( int x = 0; x < frame.cols; x++ ) {
        getEmptyLinesFromFloorPlan( emptyCols, frame, 0, frame.rows, x, prevWasBlack, frame.cols - 1, true );
    }

    // If empty cols is empty  (well less than 2 lines gap) it means the floorplan has only 1 floor and it's completely full of pixels on every row
    if ( emptyCols.size() < 2 ) {
        rects.push_back( JMATH::Rect2f( Vector2f::ZERO, Vector2f( frame.cols, frame.rows )) * scale );
        return rects;
    }

    // Now for each group of vertical line pairs (which could define a rect) let's intersect them with an horizontal scan
    // we will combine those intersections and creates beautiful rects
    for ( size_t ec = 0; ec < emptyCols.size() - 1; ec += 2 ) {
        emptyRows.clear();
        for ( int y = 0; y < frame.rows; y++ ) {
            getEmptyLinesFromFloorPlan( emptyRows, frame, emptyCols[ec], emptyCols[ec + 1], y, prevWasBlack,
                                        frame.rows - 1, false );
        }
        // if no more than 1 gap is found the make it fully vertical
        if ( emptyRows.size() < 2 ) {
            addFloorRectIfBigEnough(
                    JMATH::Rect2f( Vector2f( emptyCols[ec], 0 ), Vector2f( emptyCols[ec + 1], frame.rows )),
                    fullscreenAreaThreshold, rects );
        } else {
            // Ok gather the emptyRows pair and make rects
            for ( size_t m = 0; m < emptyRows.size() - 1; m += 2 ) {
                addFloorRectIfBigEnough( JMATH::Rect2f( Vector2f( emptyCols[ec], emptyRows[m] ),
                                                        Vector2f( emptyCols[ec + 1], emptyRows[m + 1] )),
                                         fullscreenAreaThreshold, rects );
            }
        }
    }

    // converts in meters/pixel rather than cm/pixel
    for ( auto& r : rects ) r *= scale;
    return rects;
}

void
Wall::thicknessThreshold2Passes( const cv::Mat& source, cv::Mat& dest, const JMATH::Rect2f& area, float thickness ) {
    cv::Mat accX = cv::Mat( source.rows, source.cols, source.type(), 255 );
    cv::Mat accY = cv::Mat( source.rows, source.cols, source.type(), 255 );

    int64_t ithickness = static_cast<int64_t>( thickness );

    // Horizontal scan
    for ( int y = static_cast<int>( area.top()); y < static_cast<int>( area.bottom()); y++ ) {
        int64_t accR = 0;
        const uchar *sourceY = source.ptr<uchar>( y );
        uchar *accXY = accX.ptr<uchar>( y );
        for ( int x = static_cast<int>( area.left()); x < static_cast<int>( area.right()); x++ ) {
            sourceY[x] == 0 ? ++accR : accR = 0;
            if ( accR >= ithickness ) {
                while ( ++x < area.right() && sourceY[x] == 0 ) {
                    ++accR;
                }
                // This accR+2 is to avoid a strange artifact of probably aliasing on vertical lines, resulting in smaller vertical contours compare to the horizontal one...
                memset( &accXY[x - accR], 0, accR );
            }
        }
    }

    // Vertical scan
    for ( int x = static_cast<int>( area.left()); x < static_cast<int>( area.right()); x++ ) {
        int64_t accR = 0;
        for ( int y = static_cast<int>( area.top()); y < static_cast<int>( area.bottom()); y++ ) {
            const uchar *sourceY = source.ptr<uchar>( y );
            sourceY[x] == 0 ? ++accR : accR = 0;
            if ( accR >= ithickness ) {
                while ( ++y < area.bottom() && ( source.ptr<uchar>( y ))[x] == 0 ) {
                    ++accR;
                }
                for ( int m = 0; m <= accR; m++ ) {
                    if ( y - m >= 0 && y - m < accY.rows ) {
                        accY.ptr<uchar>( y - m )[x] = 0;
                    }
                }
            }
        }
    }

    dest = accX | accY;
}

void Wall::thicknessThreshold( const cv::Mat& source, cv::Mat& dest, const JMATH::Rect2f& area, float thickness ) {
    cv::Mat temp;
    thicknessThreshold2Passes( source, temp, area, thickness );
    thicknessThreshold2Passes( temp, dest, area, thickness );

    //	calculated = true;
}

void removeCollinear( std::vector<std::vector<Vector2f> >& contoursSmooth ) {
    for ( auto& cs : contoursSmooth ) {
        removeCollinear( cs, accuracy1cm );
    }
    // Remove (again) contours that now have less than 4 verts, in case some contours were "downgraded" in the optimization process above
    contoursSmooth.erase( remove_if( contoursSmooth.begin(), contoursSmooth.end(),
                                     []( std::vector<Vector2f> const& sc ) -> bool { return sc.size() < 4; } ),
                          contoursSmooth.end());
}

void approxDadoP( std::vector<std::vector<Vector2f> >& contoursSmooth ) {
    // Angle aware version from Dado of approxPolyDP (Douglas Peucker)

    for ( auto& c : contoursSmooth ) {
        int csize = static_cast<int>( c.size());
        if ( csize < 4 ) continue;

        // start from the longest segment
        float longestS = 0.0f;
        int startIndex = 0;
        for ( auto i = 0; i < csize - 1; i++ ) {
            float cdist = distance( c[i], c[i + 1] );
            if ( cdist > longestS ) {
                startIndex = i;
                longestS = cdist;
            }
        }

        auto startI = 1;
        auto numPieces = 0;
        Vector2f slope1 = normalize( c[getCircularArrayIndex( startIndex + 1, csize )] - c[startIndex] );
        Vector2f slopePrev = slope1;
        for ( auto i = startIndex + 1; i < startIndex + 1 + csize - 1; i++ ) {
            auto i1 = getCircularArrayIndex( i, csize );
            auto i2 = getCircularArrayIndex( i + 1, csize );

            Vector2f slope2 = normalize( c[i2] - c[i1] );

            if ( fabs( dot( slope1, slope2 )) < 0.15f || fabs( dot( slope1, slope2 )) > 0.85f ||
                 fabs( dot( slopePrev, slope2 )) < 0.05f ) {
                for ( auto d = startI + 1; d < startI + numPieces; d++ )
                    c[getCircularArrayIndex( d, csize )] = Vector2f::HUGE_VALUE_POS;
                slope1 = slope2;
                startI = i2;
                numPieces = 0;
            } else {
                ++numPieces;
            }
            slopePrev = slope2;
        }
        c.erase( remove_if( c.begin(), c.end(),
                            []( Vector2f const& sc ) -> bool { return sc == Vector2f::HUGE_VALUE_POS; } ), c.end());
    }
}

void removeDeltaCorners( std::vector<std::vector<Vector2f> >& contoursSmooth ) {
    // Remove Delta from corners IE:
    // ___
    //    \   <- this delta here
    //     |
    //     |
    //

    Vector2f pi = Vector2f::ZERO;
    for ( auto& c : contoursSmooth ) {
        int csize = static_cast<int>( c.size());
        if ( csize < 4 ) continue;
        for ( auto i = 0; i < csize; i++ ) {
            auto i1 = getCircularArrayIndex( i, csize );
            auto i2 = getCircularArrayIndex( i + 1, csize );
            float dist = distance( c[i2], c[i1] );

            auto im1 = getCircularArrayIndex( i - 1, csize );
            float distP = distance( c[i1], c[im1] );
            Vector2f slopePrev = normalize( c[i1] - c[im1] );

            auto im2 = getCircularArrayIndex( i + 2, csize );
            float distN = distance( c[im2], c[i2] );
            Vector2f slopeNext = normalize( c[im2] - c[i2] );

            if ( dist > aaDelta || dist > distP || dist > distN ) continue;

            if ( isScalarEqual( dot( slopeNext, slopePrev ), 0.0f, 0.03f )) {
                intersection( c[i1] - slopePrev * 10000.0f, c[i1] + slopePrev * 100000.0f, c[i2] + slopeNext * 10000.0f,
                              c[i2] - slopeNext * 10000.0f, pi );
                c[i1] = pi;
                c.erase( c.begin() + i2 );
                --csize;
            }
        }
    }

    // Remove collinear
    removeCollinear( contoursSmooth );

}

void removeDeltaCornersAlmostRight( std::vector<std::vector<Vector2f> >& contoursSmooth, float lowThreshold,
                                    float hiThreshold ) {
/*
	// Remove slight off corners
	// ___
	//    \\
	//     //---
	//
*/
// 	return;
    for ( auto& c : contoursSmooth ) {
        int csize = static_cast<int>( c.size());
        if ( csize < 4 ) continue;
        for ( auto i = 0; i < csize; i++ ) {
            auto i1 = getCircularArrayIndex( i, csize );
            auto i2 = getCircularArrayIndex( i + 1, csize );
            auto im2 = getCircularArrayIndex( i + 2, csize );
            auto im3 = getCircularArrayIndex( i + 3, csize );
            auto im4 = getCircularArrayIndex( i + 4, csize );

            Vector2f slope = normalize( c[i2] - c[i1] );
            Vector2f slope4 = normalize( c[im4] - c[im3] );

            float extermesSloep = dot( slope, slope4 );
            bool hasParallelSlope = isScalarEqual( extermesSloep, 1.0f );
            bool hasPerpendicularSlope = isScalarEqual( extermesSloep, 0.0f );
            if ( hasParallelSlope || hasPerpendicularSlope ) {
                Vector2f slope2 = normalize( c[im2] - c[i2] );
                Vector2f slope3 = normalize( c[im3] - c[im2] );

                float internalSlopesDot = fabs( dot( slope2, slope3 ));
                if ( isbetween( internalSlopesDot, lowThreshold, hiThreshold )) {
                    Vector2f pi1 = Vector2f::ZERO;
                    Vector2f pi2 = Vector2f::ZERO;
                    Vector2f slope90 = Vector2f::ZERO;

                    bool selfIntersecting = false;
                    if ( hasParallelSlope ) {
                        slope90 = rotate90( slope4 );
                        intersection( c[i2] - slope * 10000.0f, c[i2] + slope * 100000.0f, c[im3] + slope90 * 10000.0f,
                                      c[im3] - slope90 * 10000.0f, pi1 );
                        slope90 = rotate90( slope );
                        intersection( c[i2] - slope90 * 10000.0f, c[i2] + slope90 * 100000.0f,
                                      c[im3] + slope4 * 10000.0f, c[im3] - slope4 * 10000.0f, pi2 );
                    } else {
                        intersection( c[i2] - slope * 10000.0f, c[i2] + slope * 100000.0f, c[im3] + slope4 * 10000.0f,
                                      c[im3] - slope4 * 10000.0f, pi1 );
                        intersection( c[i2] - slope4 * 10000.0f, c[i2] + slope4 * 100000.0f, c[im3] + slope * 10000.0f,
                                      c[im3] - slope * 10000.0f, pi2 );
                        Vector2f pisi = Vector2f::ZERO;
                        selfIntersecting |= intersection( c[i1], c[i2], c[im3] + slope4 * 10000.0f,
                                                          c[im3] - slope4 * 10000.0f, pisi );
                        if ( !selfIntersecting )
                            selfIntersecting |= intersection( c[im3], c[im4], c[i2] + slope * 10000.0f,
                                                              c[i2] - slope * 10000.0f, pisi );
                    }

                    float dist1 = distance( c[im2], pi1 );
                    float dist2 = distance( c[im2], pi2 );
                    if ( dist1 <= dist2 && dist1 < aaDelta ) {
                        if ( selfIntersecting ) {
                            c[im2] = pi1;
                        } else {
                            c[i2] = pi1;
                            c[im2] = Vector2f::HUGE_VALUE_POS;
                        }
                        i += 3;
                    } else if ( dist2 < dist1 && dist2 < aaDelta ) {
                        if ( selfIntersecting ) {
                            c[im2] = pi2;
                        } else {
                            c[im3] = pi2;
                            c[im2] = Vector2f::HUGE_VALUE_POS;
                        }
                        i += 3;
                    }
                }
            }
            //if (  )
        }
        c.erase( remove_if( c.begin(), c.end(),
                            []( Vector2f const& sc ) -> bool { return sc == Vector2f::HUGE_VALUE_POS; } ), c.end());
    }
    //removeCollinear( contoursSmooth );
}

void removeSpikyBumps( std::vector<std::vector<Vector2f> >& contoursSmooth ) {
    // Remove spiky bumps like from corners IE:
    //
    // ___/\____
    //

    for ( auto& c : contoursSmooth ) {
        int csize = static_cast<int>( c.size());
        if ( csize >= 4 ) {
            for ( auto i = 0; i < csize; i++ ) {
                auto i1 = cai( i, csize );
                auto i2 = cai( i + 1, csize );
                auto im2 = cai( i + 2, csize );
                auto im3 = cai( i + 3, csize );

                Vector2f slope = normalize( c[i2] - c[i1] );
                Vector2f slope3 = normalize( c[im3] - c[i2] );

                if ( isScalarEqual( fabs( dot( slope, slope3 )), 1.0f )) {
                    if ( distanceFromLine( c[im2], c[i1], c[im3] ) <= 2.0f ) {
                        c.erase( c.begin() + i2 );
                        --csize;
                        c.erase( c.begin() + cai( i2, csize ));
                        --csize;
                    }
                }
            }
        }
    }
}

void removeSpikyBumps2( std::vector<std::vector<Vector2f> >& contoursSmooth, float thickness ) {
    // Remove spiky bumps like from corners IE:
    //
    // ___/\____
    //

    for ( auto& c : contoursSmooth ) {
        int csize = static_cast<int>( c.size());
        if ( csize >= 8 ) {
            for ( auto i = 0; i < csize; i++ ) {
                auto i1 = cai( i, csize );
                auto i2 = cai( i + 1, csize );
                auto i3 = cai( i + 2, csize );
                auto i4 = cai( i + 3, csize );
                auto i5 = cai( i + 4, csize );

                Vector2f outerSegments1 = normalize( c[i2] - c[i1] );
                Vector2f outerSegments2 = normalize( c[i5] - c[i4] );

                float outerSegmentsDot = dot( outerSegments1, outerSegments2 );
                if ( isbetween( outerSegmentsDot, 0.99f, 1.0f )) {
                    Vector2f slope = normalize( c[i3] - c[i2] );
                    Vector2f slope3 = normalize( c[i3] - c[i4] );
                    float dotSlope = fabs( dot( slope, slope3 ));
                    if ( isbetween( dotSlope, 0.80f, 1.0f ) && distance(c[i3], c[i2]) + distance(c[i3], c[i4]) < thickness*2.0f ) {
                        c.erase( c.begin() + i3 );
                        --csize;
                        c.erase( c.begin() + cai( i3, csize ));
                        --csize;
                    }
                }
            }
        }
    }

    // Remove collinear
    removeCollinear( contoursSmooth );
}

void straightenPossibleUShapes( std::vector<std::vector<Vector2f> >& contoursSmooth, float thickness ) {
    // Straighten shapes like this that if 90 degrees can be ushapes
    //
    //     __
    // ___/
    //

    for ( auto& c : contoursSmooth ) {
        int csize = static_cast<int>( c.size());
        if ( csize >= 6 ) {
            for ( auto i = 0; i < csize; i++ ) {
                auto i1 = cai( i, csize );
                auto i2 = cai( i + 1, csize );
                auto i3 = cai( i + 2, csize );
                auto i4 = cai( i + 3, csize );

                Vector2f outerSegments1 = normalize( c[i2] - c[i1] );
                Vector2f outerSegments2 = normalize( c[i4] - c[i3] );

                float outerSegmentsDot = dot( outerSegments1, outerSegments2 );
                if ( isbetween( outerSegmentsDot, 0.99f, 1.0f )) {
                    Vector2f slope = normalize( c[i3] - c[i2] );
                    float dotSlope = fabs( dot( slope, outerSegments1 ));
                    if ( isbetween( dotSlope, 0.001f, 0.43f )) {
                        auto ei = slope.leastDominantElement();
                        c[i2][ei] = c[i3][ei];
                    }
                }
            }
        }
    }

    // This case also:
    //
    //  /|
    // | |
    //
    for ( auto& c : contoursSmooth ) {
        int csize = static_cast<int>( c.size());
        if ( csize >= 6 ) {
            for ( auto i = 0; i < csize; i++ ) {
                auto i1 = cai( i, csize );
                auto i2 = cai( i + 1, csize );
                auto i3 = cai( i + 2, csize );
                auto i4 = cai( i + 3, csize );

                Vector2f outerSegments1 = normalize( c[i2] - c[i1] );
                Vector2f outerSegments2 = normalize( c[i4] - c[i3] );

                float outerSegmentsDot = dot( outerSegments1, outerSegments2 );
                if ( isbetween( outerSegmentsDot, -0.99f, -1.0f )) {
                    Vector2f slope = normalize( c[i3] - c[i2] );
                    float dotSlope = fabs( dot( slope, outerSegments1 ));
                    if ( isbetween( dotSlope, 0.001f, 0.43f )) {
                        auto ei = slope.leastDominantElement();
                        auto i3New = c[i3];
                        i3New[ei] = c[i2][ei];
                        if ( distance( i3New, c[i2] ) < thickness * 1.57f ) {
                            c[i3] = i3New;
                        }
                    }
                }
            }
        }
    }

    // and this case also:
    //
    //  ~~
    // | |
    //
    return;
    for ( auto& c : contoursSmooth ) {
        int csize = static_cast<int>( c.size());
        if ( csize >= 6 ) {
            for ( auto i = 0; i < csize; i++ ) {
                auto i1 = cai( i, csize );
                auto i2 = cai( i + 1, csize );
                Vector2f outerSegments1 = normalize( c[i2] - c[i1] );

                for ( auto q = i + 3; q < i + csize; q++ ) {
                    auto ie1 = cai( q, csize );
                    auto ie2 = cai( q + 1, csize );
                    Vector2f outerSegments2 = normalize( c[ie2] - c[ie1] );
                    float outerSegmentsDot = dot( outerSegments1, outerSegments2 );
                    if ( isbetween( outerSegmentsDot, -0.99f, -1.0f )) {
                        auto startI = q >= csize ? i1 : i2;
                        auto endI = q >= csize ? ie1 : ie2;
                        float dist = distance( c[startI], c[endI] );
                        float lpDistance = distanceFromLine(c[ie2], c[ie1], c[i1] );
                        if ( dist < lpDistance * 2.0f && abs(startI-endI) > 1 ) {
                            if ( q < csize-1 ) {
                                auto delta = endI - startI+1;
                                c.erase( c.begin() + startI );
//                                c.erase( c.begin() + startI, c.begin() + endI-1 );
                                csize -= delta;
                            } else {
                                c.erase( c.begin() + cai(startI, csize) );
                                csize -= 1;
                            }
                            i = q;
                            break;
                        }
                    }
                }
            }
        }
    }

}

void removeDP( std::vector<std::vector<Vector2f> >& contoursSmooth ) {
/*
	// Remove single bump irrespective of angles IE:
	//
	// /\
	//
*/
    for ( auto& c : contoursSmooth ) {
        int csize = static_cast<int>( c.size());
        if ( csize >= 4 ) {
            for ( auto i = 0; i < csize; i++ ) {
                auto i1 = getCircularArrayIndex( i, csize );
                auto i2 = getCircularArrayIndex( i + 1, csize );
                auto i3 = getCircularArrayIndex( i + 2, csize );

                Vector2f slope = normalize( c[i2] - c[i1] );
                Vector2f slope3 = normalize( c[i3] - c[i2] );

                if ( fabs( dot( slope, slope3 )) > 0.11f ) {
                    if ( distanceFromLine( c[i2], c[i1], c[i3] ) <= 2.0f ) {
                        c.erase( c.begin() + i2 );
                        --csize;
                        --i;
                    }
                }
            }
        }
    }

    // Remove collinear
    removeCollinear( contoursSmooth );

}

void straightenSegments( std::vector<std::vector<Vector2f> >& contoursSmooth ) {
    for ( auto& c : contoursSmooth ) {
        int csize = static_cast<int>( c.size());
        for ( auto i = 0; i < csize; i++ ) {
            auto i1 = getCircularArrayIndex( i, csize );
            auto i2 = getCircularArrayIndex( i + 1, csize );
            Vector2f slope = normalize( c[i2] - c[i1] );
            float minAngle = 0.1f;
            if ( isbetween( slope.x(), -minAngle, minAngle )) {
                c[i2].setX( c[i1].x());
            } else if ( isbetween( slope.y(), -minAngle, minAngle )) {
                c[i2].setY( c[i1].y());
            }
        }
    }
}

//void removeSmallContours( std::vector<std::vector<Vector2f> >& contoursSmooth ) {
//	// Remove contours that have perimeters of less than FP()->minPerimeterLength
//	contoursSmooth.erase( remove_if( contoursSmooth.begin(), contoursSmooth.end(), []( std::vector<Vector2f> const& sc ) -> bool {
//		float perimeter = 0.0f;
//		int csize = static_cast<int>( sc.size() );
//		if ( csize > 4 ) return false;
//		for ( auto t = 0; t < csize; t++ ) {
//			perimeter += distance( sc[getCircularArrayIndex( t, csize )], sc[getCircularArrayIndex( t + 1, csize )] );
//		}
//		return perimeter * HH.FP()->PixelCM() < HH.FP()->minPerimeterLength;
//	} ), contoursSmooth.end() );
//}

void removeConsecutivePoints( std::vector<std::vector<Vector2f> >& contoursSmooth ) {
    // *******************************************************************************************************************************************************************************
    // Remove two consecutive points that have simply the same value
    for ( auto& cs : contoursSmooth ) {
        int csize = static_cast<int>( cs.size());
        std::vector<bool> toBeDeleted;
        for ( auto t = 0; t < csize; t++ ) {
            Vector2f currPoint1 = cs[t];
            Vector2f currPoint2 = cs[getCircularArrayIndex( t + 1, csize )];
            if ( distance( currPoint1, currPoint2 ) < 1.0f ) {
                cs[t] = Vector2f::HUGE_VALUE_POS;
            }
        }
        cs.erase( remove_if( cs.begin(), cs.end(),
                             [toBeDeleted]( Vector2f const& sc ) -> bool { return sc == Vector2f::HUGE_VALUE_POS; } ),
                  cs.end());
    }
    // ******************************************************************

    // Remove collinear
    removeCollinear( contoursSmooth );
}

void removeZigZag( std::vector<std::vector<Vector2f> >& contoursSmooth, const float _pixelCM ) {
    // *******************************************************************************************************************************************************************************
    //                                        ______
    // Remove slopes zig zags like this: ____/
    //
    float maxDiagonalWallLengthToBeOptimizedOut = 0.35f;

    for ( auto& cs : contoursSmooth ) {
        int csize = static_cast<int>( cs.size());
        for ( auto t = 0; t < csize; t++ ) {
            int t1 = getCircularArrayIndex( t + 1, csize );
            int t2 = getCircularArrayIndex( t + 2, csize );
            int t3 = getCircularArrayIndex( t + 3, csize );
            Vector2f p1 = cs[t];
            Vector2f p2 = cs[t1];
            Vector2f p3 = cs[t2];
            Vector2f p4 = cs[t3];

            Vector2f longE1 = normalize( p2 - p1 );
            Vector2f longE2 = normalize( p4 - p3 );
            if ( isVerySimilar( longE1, longE2 ) &&
                 ( distance( p2, p3 ) * _pixelCM < maxDiagonalWallLengthToBeOptimizedOut )) {
                Vector2f newPoint = Vector2f::ZERO;
                Vector2f perpNormal = rotate90( longE2 );
                if ( intersection( p3 + perpNormal * 10000.0f, p3 - perpNormal * 10000.0f, p2 + longE1 * 10000.0f,
                                   p2 - longE1 * 10000.0f, newPoint )) {
                    cs[t1] = newPoint;
                }
            }
        }
    }

    removeCollinear( contoursSmooth );
}

void remove1pxNoise( std::vector<std::vector<Vector2f> >& contoursSmooth ) {
    // *******************************************************************************************************************************************************************************
    //                                             ______
    // Remove 1 pixel only slopes  like this: ____|
    //
    for ( auto& cs : contoursSmooth ) {
        int csize = static_cast<int>( cs.size());
        for ( auto t = 0; t < csize; t++ ) {
            int t1 = getCircularArrayIndex( t + 1, csize );
            int t2 = getCircularArrayIndex( t + 2, csize );
            int t3 = getCircularArrayIndex( t + 3, csize );
            Vector2f p1 = cs[t];
            Vector2f p2 = cs[t1];
            Vector2f p3 = cs[t2];
            Vector2f p4 = cs[t3];

            Vector2f longE1 = normalize( p2 - p1 );
            Vector2f longE2 = normalize( p4 - p3 );
            Vector2f smallE = normalize( p2 - p3 );
            float smallSegmDist = distance( p2, p3 );
            if ( isVerySimilar( longE1, longE2 ) && isScalarEqual( dot( smallE, longE2 ), 0.0f ) &&
                 ( smallSegmDist <= 1.0f )) {
                Vector2f perpNormal = smallE;
                cs[t2] += perpNormal * smallSegmDist;
                cs[t3] += perpNormal * smallSegmDist;
            }
        }
    }

    //	removeCollinear( contoursSmooth );
}

void
straightenContour( std::vector<std::vector<Vector2f> >& contoursSmooth, float thickness ) {
    // remove walls which length is less than a min perimeter
//	removeSmallContours( contoursSmooth );
    // Poor version of collinear, but faster
//	removeConsecutivePoints( contoursSmooth );
    // Straighten walls
//    straightenSegments( contoursSmooth );
    // Remove deltas
    approxDadoP( contoursSmooth );
    // Remove spiky bumps
    removeSpikyBumps( contoursSmooth );
    //// Straighten walls
//    straightenSegments( contoursSmooth );
    // Remove zig zag
//    removeZigZag( contoursSmooth, _pixelCM );
//    // Remove 1px noise
    remove1pxNoise( contoursSmooth );

    // Remove single DPs
    removeDP( contoursSmooth );

    //     Remove delta just off
//    removeDeltaCornersAlmostRight( contoursSmooth );

    // Remove deltas
    removeDeltaCorners( contoursSmooth );

    // Straighten walls
    straightenSegments( contoursSmooth );

    //
    straightenPossibleUShapes( contoursSmooth, thickness );

    // Remove collinear
//    removeCollinear( contoursSmooth );
}

void
straightenContour2( std::vector<std::vector<Vector2f> >& contoursSmooth, float thickness ) {
    // remove walls which length is less than a min perimeter
//	removeSmallContours( contoursSmooth );
    // Poor version of collinear, but faster
//	removeConsecutivePoints( contoursSmooth );
    // Straighten walls
    straightenSegments( contoursSmooth );
    // Remove deltas
    approxDadoP( contoursSmooth );
    // Remove spiky bumps
    removeSpikyBumps( contoursSmooth );
    //// Straighten walls
    straightenSegments( contoursSmooth );
    // Remove zig zag
    straightenPossibleUShapes( contoursSmooth, thickness );
//    removeZigZag( contoursSmooth, 0.01f ); // _pixelCM = 0.01f;
//    // Remove 1px noise
    remove1pxNoise( contoursSmooth );

    //     Remove delta just off
    removeDeltaCornersAlmostRight( contoursSmooth, 0.01f, 0.909f );

    // Remove single DPs
    removeDP( contoursSmooth );


    // Remove deltas
//    removeDeltaCorners( contoursSmooth );

    // Straighten walls
    straightenSegments( contoursSmooth );

    //
//    straightenPossibleUShapes( contoursSmooth, thickness );

    // Remove collinear
//    removeCollinear( contoursSmooth );
}

void
straightenContour3( std::vector<std::vector<Vector2f> >& contoursSmooth, float thickness ) {
    // remove walls which length is less than a min perimeter
//	removeSmallContours( contoursSmooth );
    // Poor version of collinear, but faster
//	removeConsecutivePoints( contoursSmooth );
    // Straighten walls
//    straightenSegments( contoursSmooth );

    // Remove zig zag
//    removeZigZag( contoursSmooth, _pixelCM );
//    // Remove 1px noise
    remove1pxNoise( contoursSmooth );

    // Remove deltas
    approxDadoP( contoursSmooth );
    //// Straighten walls
//    straightenSegments( contoursSmooth );

    // Remove single DPs
    removeDP( contoursSmooth );

    // Remove spiky bumps
//    removeSpikyBumps( contoursSmooth );
    // Remove deltas
    removeDeltaCorners( contoursSmooth );

    // Remove deltas
//    approxDadoP( contoursSmooth );

    //     Remove delta just off
    removeDeltaCornersAlmostRight( contoursSmooth, 0.3f, 0.7f );

    // Straighten walls
    straightenSegments( contoursSmooth );

    // Remove spiky bumps
    removeSpikyBumps2( contoursSmooth, thickness );

    // Remove zig zag
//    removeZigZag( contoursSmooth, _pixelCM );

    //
    straightenPossibleUShapes( contoursSmooth, thickness );

}

void eraseRedundantVerts( std::vector<std::vector<Vector2f> >& /*contoursSmooth*/ ) {
    // *******************************************************************************************************************************************************************************
    // Remove small zig zags like this: ____----____
    //for ( auto& cs : contoursSmooth ) {
    //	bool bRestart = true;
    //	while ( bRestart ) {
    //		int csize = static_cast<int>( cs.size() );
    //		bRestart = false;
    //		for ( auto t = 0; t < csize; t++ ) {
    //			int t1 = getCircularArrayIndex( t + 1, csize );
    //			int t2 = getCircularArrayIndex( t + 2, csize );
    //			int t3 = getCircularArrayIndex( t + 3, csize );
    //			Vector2f p1 = cs[t];
    //			Vector2f p2 = cs[t1];
    //			Vector2f p3 = cs[t2];
    //			Vector2f p4 = cs[t3];

    //			Vector2f longE1 = normalize( p2 - p1 );
    //			Vector2f longE2 = normalize( p4 - p3 );
    //			if ( isVerySimilar( longE1, longE2 ) ) {
    //				float dMiddle = distance( p2, p3 );
    //				if ( dMiddle <= 2.0f ) {
    //					cs[t3] = p1 + ( longE1 * ( distance( p2, p1 ) + distance( p3, p4 ) ) );
    //					// Remove in between verts; PS being a circular index we need to handle cased in which last index is _before_ prev index
    //					if ( t2 > t1  ) {
    //						cs.erase( cs.begin() + t1, cs.begin() + t2 + 1 );
    //					} else {
    //						cs.erase( cs.begin() + t1, cs.end() );
    //						cs.erase( cs.begin(), cs.begin() + t2 + 1 );
    //					}
    //					bRestart = true;
    //					break;
    //				}
    //			}
    //		}
    //	}
    //}
}

int Wall::triangulate( const std::vector<Vector2f>& points ) {

    Triangulator tri( points, 0.000001f );

    std::vector<std::vector<Vector2f>> tri_neighbours;
    tri.gather3dTriangularizationWithNeighbours( tri_neighbours );

    int numQuads = 0;
    for ( auto& v : tri_neighbours ) {
        auto e1 = normalize( rotate90( v[0] - v[2] ));
        auto e2 = normalize( rotate90( v[1] - v[0] ));
        auto e3 = normalize( rotate90( v[2] - v[1] ));

        if ( isScalarEqual( dot( e1, e2 ), 0.0f ) || isScalarEqual( dot( e1, e3 ), 0.0f ) ||
             isScalarEqual( dot( e3, e2 ), 0.0f )) {
            for ( size_t t = 3; t < v.size(); t += 3 ) {
                auto ve1 = normalize( rotate90( v[t] - v[t + 2] ));
                auto ve2 = normalize( rotate90( v[t + 1] - v[t] ));
                auto ve3 = normalize( rotate90( v[t + 2] - v[t + 1] ));

                if ( isScalarEqual( dot( ve1, ve2 ), 0.0f ) || isScalarEqual( dot( ve1, ve3 ), 0.0f ) ||
                     isScalarEqual( dot( ve3, ve2 ), 0.0f )) {
                    auto sum = e1 + ve1 + e2 + ve2 + e3 + ve3;
                    if ( isVerySimilar( sum, Vector2f::ZERO )) {
                        ++numQuads;
                    }
                }
            }
        }
    }

    return numQuads;
}

void approxDP( const std::vector<std::vector<cv::Point2i> >& contoursi,
               const std::vector<std::vector<cv::Point2i> >& contoursiOrig,
               std::vector<std::vector<cv::Point2i> >& contoursiMiddle ) {

    contoursiMiddle.clear();
    for ( size_t t = 0; t < contoursi.size(); t++ ) {
        auto ci = contoursi[t];
        auto ciorig = contoursiOrig[t];
        float sourceArea = cv::contourArea( ciorig );
        std::vector<cv::Point2i> cmi = ci;
        float arcLength = cv::arcLength( ciorig, true );
        std::vector<float> thres2{};
//        thres2.push_back( arcLength*0.006f );
        for ( int q = 1; q < 10; q++ ) {
            thres2.push_back(( arcLength * 0.000099f ) * (float) ( q ));
        }

        float max_score = std::numeric_limits<float>::max();
        for ( const auto thre : thres2 ) {
            std::vector<cv::Point2i> cm;
            cv::approxPolyDP( ci, cm, thre, true );
            float newArea = cv::contourArea( cm );
            float ratio = sourceArea / newArea;
//            float score = fabs( 1.0f - ratio ) * pow(((float) cm.size() / (float) ci.size()), 6.0f );
            float score = fabs( 1.0f - ratio );
            if ( score < max_score ) {
                max_score = score;
                cmi = cm;
            }
            LOGRS( ci.size() << cm.size() << " Diff of contours for " << thre << ": " << ratio << " FINAL SCORE: "
                             << score );
        }
        contoursiMiddle.emplace_back( cmi );
    }
}

std::vector<std::vector<Vector2f>> Wall::simplifyWalls( const cv::Mat& source, int strategyIndex, float thickness ) {
    cv::Mat edges;

    std::vector<std::vector<Vector2f> > contours;
    std::vector<std::vector<Vector2f> > contoursOpt;
    std::vector<std::vector<cv::Point2i> > contoursi;
    std::vector<std::vector<cv::Point2i> > contoursiMiddle;
    std::vector<std::vector<cv::Point2i> > contoursiMiddle2;
    std::vector<std::vector<cv::Point2i> > contoursiOpt;

    Laplacian( source, edges, CV_8U, 1 );
    cv::findContours( edges, contoursi, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_TC89_KCOS );
    // Convert into floats
    convertContoursArrayToFloats( contoursi, contours );
    switch ( strategyIndex ) {
        case 0:
            straightenContour( contours, thickness );
            break;
        case 1:
            straightenContour2( contours, thickness );
            break;
        case 2:
            straightenContour3( contours, thickness );
            break;
        default:
            break;
    }
    return contours;
}
