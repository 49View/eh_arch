//
//  window.cpp
//  sixthview
//
//  Created by Dado on 05/10/2015.
//
//

#include "window_service.hpp"

#include "twoushapes_service.hpp"

std::shared_ptr<WindowBSData> WindowService::createWindow( float _windowHeight, float _ceilingHeight, float _defaultWindowBaseOffset, const UShape& w1, const UShape& w2, ArchSubTypeT /*st*/ /*= ArchSubType::NotApplicable */ ) {
	std::shared_ptr<WindowBSData> d1 = std::make_shared<WindowBSData>();

	d1->asType = ASType::Window;
	d1->type = ArchType::WindowT;
	d1->wallFlags = WallFlags::WF_HasSkirting | WallFlags::WF_HasCoving;
	d1->us1 = w1;
	d1->us2 = w2;
	d1->us1.type = ArchType::WindowT;
	d1->us2.type = ArchType::WindowT;
	d1->height = _windowHeight;
	d1->sillThickness = 2.0f;
	d1->thickness = 3.0f;
	d1->mainFrameWidth = 3.0f;
	d1->minPanelWidth = 60.0f;
	d1->ceilingHeight = _ceilingHeight;
	d1->baseOffset = _defaultWindowBaseOffset;

	TwoUShapesBasedService::evalData( d1.get() );

	d1->numPanels = static_cast<int32_t>( d1->width / d1->minPanelWidth );
	if ( d1->numPanels <= 0 ) d1->numPanels = 1;
	float totalMainFramesWidths = d1->mainFrameWidth * ( d1->numPanels + 1 );
	if ( totalMainFramesWidths > d1->width ) {
		totalMainFramesWidths = d1->width / 2;
		d1->mainFrameWidth = d1->width / 4.0f;
	}
	d1->minPanelWidth = ( d1->width - totalMainFramesWidths ) / d1->numPanels;

	return d1;
}

void WindowService::rescale( WindowBSData* w, float _scale ) {
	TwoUShapesBasedService::rescale( w, _scale );
}

void WindowService::getPlasterMiddlePoints( const WindowBSData* w, std::vector<Vector3f>& mpoints ) {
	mpoints.push_back( { w->bbox3d.centre().xy(), w->baseOffset*0.5f } );
	float ch = w->ceilingHeight - ( w->baseOffset + w->height );
	mpoints.push_back( { w->bbox3d.centre().xy(), ( w->baseOffset + w->height ) + ch*0.5f } );
}
