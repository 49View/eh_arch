//
//  window.hpp
//  sixthview
//
//  Created by Dado on 05/10/2015.
//
//

#pragma once

#include "house_bsdata.hpp"

class WindowService {
public:
	// Query
	static void getPlasterMiddlePoints( const WindowBSData* w, std::vector<Vector3f>& mpoints );
};
