//
// Created by dado on 05/07/2020.
//

#include "arch_positional_dot.hpp"
#include <core/camera.h>
#include <graphics/renderer.h>
#include <graphics/mouse_input.hpp>
#include <render_scene_graph/render_orchestrator.h>

#include <eh_arch/models/house_service.hpp>
#include <eh_arch/models/room_service.hpp>

static constexpr float fadeOutNearDistance = 1.0f; // If dot is closer than this from the camera, linearly fade it out.
static constexpr float minSafeDistance = 0.75f; // Deduct this distance from clicked dot position to avoid being to close to walls/objects
static constexpr float slowingDownTimeOnRotationFactor = 2.0f; // When we travel to walls we'll also rotate the camera M_PI to not ending up facing the wall, that rotation needs to be slower than the usual goto time.
static constexpr float gotoAndFadeTime = 0.5f; // Time it takes to travel to point clicked _and_ to fade in/out from it

inline const V4f& defaultDotColor() {
    return C4fc::SKY_BLUE;
}

void ArchPositionalDot::updateDot( RenderOrchestrator& rsg ) {
    static constexpr float outerDotSize = 0.2f;

    float alphaDistanceAttenuation = min(distance(hitPosition, rsg.DC()->getPosition()), fadeOutNearDistance);
    float finalAlphaValue = positionalDotAlphaAnim.value() * alphaDistanceAttenuation;

    rsg.RR().drawDotCircled( outerDotSize, hitPosition, fd.normal, defaultDotColor(), finalAlphaValue, "d1");
}

void ArchPositionalDot::singleTap( RenderOrchestrator& rsg ) {
    if ( fd.hasHit() && !isFlying ) {
        bool isNewPositionWalkingOnFloor = fd.normal.dominantElement() == 1;
        float speedFactor = ( isNewPositionWalkingOnFloor || !antiWallRotation ) ? 1.0f
                                                                                 : slowingDownTimeOnRotationFactor;
        isFlying = true;
        positionalDotAlphaAnim.fadeOut();
        Timeline::play(rsg.DC()->PosAnim(), 0, KeyFramePair{ gotoAndFadeTime * speedFactor, hitPosition },
                       AnimEndCallback{ [&]() {
                           positionalDotAlphaAnim.fadeIn();
                           isFlying = false;
                       } });
        if ( !isNewPositionWalkingOnFloor && antiWallRotation ) {
            auto quat = rsg.DC()->quatAngle();
            quat = quat * Quaternion{ M_PI, V3f::UP_AXIS };
            Timeline::play(rsg.DC()->QAngleAnim(), 0, KeyFramePair{ gotoAndFadeTime * speedFactor, quat });
        }
    }
}

void ArchPositionalDot::firstTimeTouchDown() {
    if ( fd.hasHit() && !isFlying ) {
        positionalDotAlphaAnim.fadeOut();
    }
}

void ArchPositionalDot::touchUp() {
    if ( !isFlying ) {
        positionalDotAlphaAnim.fadeIn();
    }
}

void
ArchPositionalDot::tick( const HouseBSData *_house, const V3f& _dir, RenderOrchestrator& rsg ) {
    if ( !isFlying ) {
        fd = HouseService::rayFeatureIntersect(_house, RayPair3{ rsg.DC()->getPosition(), _dir },
                                               FeatureIntersectionFlags::FIF_Walls |
                                               FeatureIntersectionFlags::FIF_Windows |
                                               FeatureIntersectionFlags::FIF_Floors);
        if ( fd.hasHit() ) {
            float safeDist = fd.nearV > minSafeDistance ? fd.nearV - minSafeDistance : 0.0f;
            hitPosition = rsg.DC()->getPosition() + ( _dir * safeDist );
            updateDot(rsg);
            // NDDado: this resets the hit target to the camera height, to keep the camera at the same height level
            // (The flying code uses hitPosition as internal variable, I know, it's a bit non-functionally programming
            // but yeah...
            hitPosition.setY(rsg.DC()->getPosition().y());
        }
    }
}
