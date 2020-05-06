//
//  Arch Creator
//
//  Created by Dado on 14/06/2016.
//
//

#include "arch_creator.hpp"
#include <core/resources/profile.hpp>

Profile loadCovingProfile() {
    Profile profile;

    Vector2f size = Vector2f( 12.0f, 12.0f ) * 0.01f;
    float height = size.y();
    float width = size.x();
    std::vector<Vector2f> points;
    // Counterclockwise
    points.push_back( Vector2f( 0.0f, 0.0f ));
    float d = 0.0f;
    for ( int t = 0; t < 2; t++ ) {
        points.push_back( Vector2f(( width * 0.8f - ( cos( d ) * ( width * 0.6f ))), ( sin( d ) * ( height * 0.8f ))));
        d += ( 1.0f / 9.0f ) * M_PI_2;
    }
    points.push_back( Vector2f( width, height ));
    points.push_back( Vector2f( 0.0f, height ));

    profile.createArbitrary( points );

    return profile;
}

Profile loadKitchenWorktopProfile( const Vector2f& size, float ) {
    Profile profile;

    float height = size.y();
    float width = size.x();
    std::vector<Vector2f> points;
    // Counterclockwise
    points.push_back( Vector2f( 0.0f, 0.0f ));
    points.push_back( Vector2f( width, 0.0f ));
    points.push_back( Vector2f( width, height ));
    points.push_back( Vector2f( 0.0f, height ));

    for ( auto& p : points ) {
        p -= { width * 0.5f, height * 0.5f };
    }

    profile.createArbitrary( points );

    return profile;
}

Profile loadDoorFrameProfile( const float _architraveWidth ) {
    std::vector<Vector2f> points;
    // Counterclockwise
    float doorFrameWidth = _architraveWidth;
    float doorFrameHeight = 3.0f * 0.01f;
    points.push_back( Vector2f( 0.0f, 0.0f ));
    points.push_back( Vector2f( doorFrameWidth, doorFrameHeight * 0.0f ));
    points.push_back( Vector2f( doorFrameWidth, doorFrameHeight * 1.0f ));
    points.push_back( Vector2f( doorFrameWidth * 0.66f, doorFrameHeight * 1.00f ));
    points.push_back( Vector2f( doorFrameWidth * 0.66f, doorFrameHeight * 0.70f ));
    points.push_back( Vector2f( doorFrameWidth * 0.33f, doorFrameHeight * 0.70f ));
    points.push_back( Vector2f( doorFrameWidth * 0.33f, doorFrameHeight * 0.25f ));
    points.push_back( Vector2f( doorFrameWidth * 0.0f, doorFrameHeight * 0.25f ));

    Profile doorFrame_profile;
    doorFrame_profile.createArbitrary( points );

    return doorFrame_profile;
}
