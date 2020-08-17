//
//  window.cpp
//  sixthview
//
//  Created by Dado on 05/10/2015.
//
//

#include "window_service.hpp"
#include <core/util_follower.hpp>

void WindowService::getPlasterMiddlePoints( const WindowBSData* w, std::vector<Vector3f>& mPoints ) {
	mPoints.emplace_back( w->Position2d(), w->baseOffset*0.5f );
	float ch = w->ceilingHeight - ( w->baseOffset + w->Height() );
	mPoints.emplace_back( w->Position2d(), ( w->baseOffset + w->Height() ) + ch*0.5f );
}
