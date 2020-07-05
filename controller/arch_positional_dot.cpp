//
// Created by dado on 05/07/2020.
//

#include "arch_positional_dot.hpp"
#include <core/math/anim.h>
#include <core/camera.h>
#include <graphics/ghtypes.hpp>
#include <graphics/renderer.h>
#include <graphics/mouse_input.hpp>
#include <render_scene_graph/render_orchestrator.h>

#include <eh_arch/models/house_service.hpp>

static constexpr float fullDotOpacityValue = 0.85f;

ArchPositionalDot::ArchPositionalDot() {
    positionalDotAlphaAnim = std::make_shared<AnimType<float>>(fullDotOpacityValue, "positionalDotAlphaAnim");
}

/// @brief
/// This is working nicely, only thing left to fix is the positionChangedIn which is not implemented fully because of
/// the way we do intersections every frame, we would need to store a last known good intersection but it will be stuck
/// to that single point so I think it will look horrible, needs to provide a better solution.
/// \param _aid
void ArchPositionalDot::update( const HouseBSData *_house, const AggregatedInputData& _aid, RenderOrchestrator& rsg ) {
    constexpr float gotoAndFadeTime = 0.5f; // Time it takes to travel to point clicked _and_ to fade in/out from it
    constexpr float outerDotSize = 0.2f;
    constexpr float outerDotBorderSize = 0.015f;
    constexpr float smallDotRatio = 0.8f;
    constexpr float fadeOutNearDistance = 1.0f; // If dot is closer than this from the camera, linearly fade it out.
    constexpr float minSafeDistance = 0.25f; // Deduct this distance from clicked dot position to avoid being to close to walls/objects
    auto dir = _aid.mouseViewportDir(TouchIndex::TOUCH_ZERO, rsg.DC());
    fd = HouseService::rayFeatureIntersect(_house, RayPair3{ rsg.DC()->getPosition(), dir });
    if ( _aid.isMouseTouchedDownFirstTime(TOUCH_ZERO) || positionChangedOut ) {
        Timeline::stop(positionalDotAlphaAnim, positionalDotAlphaFadeInAnimKey, positionalDotAlphaAnim->value);
        positionalDotAlphaFadeOutAnimKey = Timeline::play(positionalDotAlphaAnim, 0,
                                                          KeyFramePair{ gotoAndFadeTime, 0.0f });
        positionChangedOut = false;
    }
    if ( _aid.isMouseTouchedUp(TOUCH_ZERO) || positionChangedIn ) {
        Timeline::stop(positionalDotAlphaAnim, positionalDotAlphaFadeOutAnimKey, positionalDotAlphaAnim->value);
        positionalDotAlphaFadeInAnimKey = Timeline::play(positionalDotAlphaAnim, 0,
                                                         KeyFramePair{ gotoAndFadeTime, fullDotOpacityValue });
        positionChangedIn = false;
    }
    if ( fd.hasHit() ) {
        if ( !currentHit ) positionChangedIn = true;
        currentHit = true;
        float safeDist = fd.nearV > minSafeDistance ? fd.nearV - minSafeDistance : 0.0f;
        V3f ic = rsg.DC()->getPosition() + ( dir * safeDist );

        float alphaDistanceAttenuation = min(distance(ic, rsg.DC()->getPosition()), fadeOutNearDistance);
        float finalAlphaValue = positionalDotAlphaAnim->value * alphaDistanceAttenuation;
        auto sm3 = DShaderMatrix{ DShaderMatrixValue3dColor };
        std::string key = std::string{ "mouseFloorPoint" } + std::to_string(finalAlphaValue);
        std::string key2 = key + "2";
        std::string key3 = key + "3";
        rsg.RR().draw<DCircleFilled>(CommandBufferLimits::CameraLocator, ic, V4f::WHITE.A(finalAlphaValue),
                                     outerDotSize + outerDotBorderSize, sm3, RDSRotationNormalAxis{ fd.normal },
                                     RDSArchSegments{ 36 }, key3);
        rsg.RR().draw<DCircleFilled>(CommandBufferLimits::CameraLocator, ic, V4f::SKY_BLUE.A(finalAlphaValue),
                                     outerDotSize, sm3, RDSRotationNormalAxis{ fd.normal }, RDSArchSegments{ 36 }, key);
        rsg.RR().draw<DCircleFilled>(CommandBufferLimits::CameraLocator, ic, V4f::ROYAL_BLUE.A(finalAlphaValue),
                                     outerDotSize * smallDotRatio, sm3, RDSRotationNormalAxis{ fd.normal },
                                     RDSArchSegments{ 36 }, key2);

        ic.setY(rsg.DC()->getPosition().y());

        if ( _aid.isMouseSingleTap(TOUCH_ZERO) ) {
            Timeline::play(rsg.DC()->PosAnim(), 0, KeyFramePair{ gotoAndFadeTime, ic });
//            auto quatr = rsg.DC()->quatAngle();
//            quatr = quatr * Quaternion{M_PI, V3f::UP_AXIS};
//            Timeline::play(rsg.DC()->QAngleAnim(), 0, KeyFramePair{ gotoAndFadeTime, quatr }, AnimEndCallback{[&](){
//                rsg.DC()->setIncrementQuatAngles(rsg.DC()->quatAngle().euler());
//            }});
        }
    } else {
        if ( currentHit ) positionChangedOut = true;
        currentHit = false;
    }
}
