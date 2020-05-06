//
//  Arch Creator
//
//  Created by Dado on 14/06/2016.
//
//

#pragma once

class Profile;
class Vector2f;

Profile loadCovingProfile();
Profile loadDoorFrameProfile( const float _architraveWidth );
Profile loadInnerDoorFrameProfile( float thickness, const Vector2f& bump, float trim, float doorGeomThickness );
Profile loadKitchenWorktopProfile( const Vector2f& size, float bevelAmount = 0.0f );
