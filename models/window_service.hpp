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
	// Create
	static std::shared_ptr<WindowBSData> createWindow( float _windowHeight, float _ceilingHeight, float _defaultWindowBaseOffset, const UShape& w1, const UShape& w2, ArchSubTypeT st = ArchSubType::NotApplicable );

	// Query
	static void getPlasterMiddlePoints( const WindowBSData* w, std::vector<Vector3f>& mpoints );

	// Update
	static void rescale( WindowBSData* w, float _scale );
};
