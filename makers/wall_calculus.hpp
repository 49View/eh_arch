//
//  wallCalculus.hpp
//  sixthview
//
//  Created by Dado on 05/10/2015.
//
//

#pragma once

#include "core/math/vector3f.h"
#include "core/math/rect2f.h"

#include "opencvutils/cvmatutil.hpp"

class WallEvalData {
public:
	WallEvalData() {
		reset();
	}

	void reset() {
		thickness = 1;
		numWalls = 0;
		numiPoints = 0;
		averageWallLength = 0.0f;
		totalWallsLength = 0.0f;
		score = 0.0f;
		elabTime = 0.0;
		linesVector.clear();
		elaborated = false;
	}

	int64_t thickness;
	int64_t numWalls;
	int64_t numiPoints;
	float   averageWallLength;
	float   totalWallsLength;
	float   score;
	double  elabTime;
	bool    elaborated;
	std::vector<std::pair<Vector3f, Vector3f>> linesVector;
};

std::vector<JMATH::Rect2f> getFloorplanRects( const cv::Mat& frame );

class Wall {
public:
	static void thicknessThreshold( const cv::Mat& source, cv::Mat& dest, const JMATH::Rect2f& area, float thickness );
	static std::vector<std::vector<Vector2f>> simplifyWalls( const cv::Mat& source, int strategyIndex, float thickness );
	static int getSolidAround( const cv::Mat& frame, int x, int y, int n );
	static void thicknessThreshold2Passes( const cv::Mat& source, cv::Mat& dest, const JMATH::Rect2f& area, float thickness );
	static int triangulate( const std::vector<Vector2f>& points );
};
