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
#include <eh_arch/models/room_service.hpp>

static constexpr float fullDotOpacityValue = 0.85f;
static constexpr float outerDotSize = 0.2f;
static constexpr float outerDotBorderSize = 0.015f;
static constexpr float smallDotRatio = 0.8f;
static constexpr float fadeOutNearDistance = 1.0f; // If dot is closer than this from the camera, linearly fade it out.
static constexpr float minSafeDistance = 0.75f; // Deduct this distance from clicked dot position to avoid being to close to walls/objects
static constexpr float slowingDownTimeOnRotationFactor = 2.0f; // When we travel to walls we'll also rotate the camera M_PI to not ending up facing the wall, that rotation needs to be slower than the usual goto time.
static constexpr float gotoAndFadeTime = 0.5f; // Time it takes to travel to point clicked _and_ to fade in/out from it
static constexpr float dotFadeTime = 0.15f;

ArchPositionalDot::ArchPositionalDot() {
    positionalDotAlphaAnim = std::make_shared<AnimType<float >>(fullDotOpacityValue, "positionalDotAlphaAnim");
}

void ArchPositionalDot::updateRender( RenderOrchestrator& rsg ) {

    if ( fd.hasHit() ) {
        float safeDist = fd.nearV > minSafeDistance ? fd.nearV - minSafeDistance : 0.0f;
        hitPosition = rsg.DC()->getPosition() + ( dir * safeDist );

        float alphaDistanceAttenuation = min(distance(hitPosition, rsg.DC()->getPosition()), fadeOutNearDistance);
        float finalAlphaValue = positionalDotAlphaAnim->value * alphaDistanceAttenuation;
        bool isBlueDot = !furnitureSelected;// (isNewPositionWalkingOnFloor || !antiWallRotation);
        C4f outerDotColor = isBlueDot ? V4f::SKY_BLUE.A(finalAlphaValue) : V4f::SPRING_GREEN.A(
                finalAlphaValue);
        auto sm3 = DShaderMatrix{ DShaderMatrixValue3dColor };
        std::stringstream stream;
        stream << std::fixed << std::setprecision(2) << finalAlphaValue;
        std::string nameTag = stream.str() + ( isBlueDot ? "1" : "0" );
        rsg.RR().draw<DCircleFilled>(CommandBufferLimits::CameraMousePointers, hitPosition,
                                     V4f::WHITE.A(finalAlphaValue),
                                     outerDotSize + outerDotBorderSize, sm3, RDSRotationNormalAxis{ fd.normal },
                                     RDSArchSegments{ 36 }, "d1" + nameTag);
        rsg.RR().draw<DCircleFilled>(CommandBufferLimits::CameraMousePointers, hitPosition, outerDotColor,
                                     outerDotSize, sm3, RDSRotationNormalAxis{ fd.normal }, RDSArchSegments{ 36 },
                                     "d2" + nameTag);
        rsg.RR().draw<DCircleFilled>(CommandBufferLimits::CameraMousePointers, hitPosition,
                                     V4f::WHITE.A(finalAlphaValue),
                                     outerDotSize * smallDotRatio, sm3, RDSRotationNormalAxis{ fd.normal },
                                     RDSArchSegments{ 36 }, "d3" + nameTag);

        hitPosition.setY(rsg.DC()->getPosition().y());
    }
}

void ArchPositionalDot::touchMoveWithModKeyCtrl( const HouseBSData *_house, RenderOrchestrator& rsg ) {
    if ( furnitureSelected && fd.hasHit() ) {
        hitPosition = rsg.DC()->getPosition() + ( dir * fd.nearV );
        if ( prevFurnitureMovePosition != V3f::HUGE_VALUE_NEG ) {
            V3f off = hitPosition - prevFurnitureMovePosition;
            auto potentialBBox = furnitureSelected->bbox;
            potentialBBox.translate(XZY::C2(off));
            if ( !bRoomBboxCheck || RS::checkBBoxInsideRoom(fd.room, potentialBBox) ) {
                auto node = rsg.SG().Nodes().find(furnitureSelected->linkedUUID);
                if ( node->second ) {
                    node->second->move(off);
                }
                furnitureSelected->position3d += off;
                furnitureSelected->bbox3d.translate(off);
                furnitureSelected->bbox.translate(XZY::C2(off));
            }
        }
        prevFurnitureMovePosition = hitPosition;
    }
}

/// @brief
/// This is working nicely, only thing left to fix is the positionChangedIn which is not implemented fully because of
/// the way we do intersections every frame, we would need to store a last known good intersection but it will be stuck
/// to that single point so I think it will look horrible, needs to provide a better solution.
/// \param _aid
void
ArchPositionalDot::tick( const HouseBSData *_house, const V3f& _dir, const InputMods& _mods, RenderOrchestrator& rsg ) {

    dir = _dir;

//    if ( _aid.isMouseTouchedUp(TOUCH_ZERO) && bHitFurniture && isControlKeyDown ) {
//        asg.MakeHouse3d{}(asg, rsg, arc);
//        asg.pushHouseChange();
//    }

    if ( !_mods.isControlKeyDown ) furnitureSelected = nullptr;

    if ( !isFlying ) {
        FeatureIntersectionFlagsT featureIntersectionFlags = furnitureSelected ? FeatureIntersectionFlags::FIF_Floors :
                                                             FeatureIntersectionFlags::FIF_Walls |
                                                             FeatureIntersectionFlags::FIF_Floors;

        fd = HouseService::rayFeatureIntersect(_house, RayPair3{ rsg.DC()->getPosition(), dir },
                                               featureIntersectionFlags);
        if ( _mods.isControlKeyDown && !furnitureSelected ) {
            auto fdFurniture = HouseService::rayFeatureIntersect(_house, RayPair3{ rsg.DC()->getPosition(), dir },
                                                                 FeatureIntersectionFlags::FIF_Furnitures);
            if ( fdFurniture.hasHit() ) {
                furnitureSelected = dynamic_cast<FittedFurniture *>(fdFurniture.arch);
                prevFurnitureMovePosition = V3f::HUGE_VALUE_NEG;
            }
        }
    }

    updateRender(rsg);
}

void ArchPositionalDot::singleTap( RenderOrchestrator& rsg ) {
    if ( fd.hasHit() && !isFlying ) {
        bool isNewPositionWalkingOnFloor = fd.normal.dominantElement() == 1;
        float speedFactor = ( isNewPositionWalkingOnFloor || !antiWallRotation ) ? 1.0f
                                                                                 : slowingDownTimeOnRotationFactor;
        isFlying = true;
        Timeline::stop(positionalDotAlphaAnim, positionalDotAlphaFadeInAnimKey, positionalDotAlphaAnim->value);
        Timeline::stop(positionalDotAlphaAnim, positionalDotAlphaFadeOutAnimKey, positionalDotAlphaAnim->value);
        positionalDotAlphaFadeOutAnimKey = Timeline::play(positionalDotAlphaAnim, 0,
                                                          KeyFramePair{ dotFadeTime, 0.0f,
                                                                        AnimVelocityType::Cosine });
        Timeline::play(rsg.DC()->PosAnim(), 0, KeyFramePair{ gotoAndFadeTime * speedFactor, hitPosition },
                       AnimEndCallback{ [&]() {
                           positionalDotAlphaFadeInAnimKey = Timeline::play(positionalDotAlphaAnim, 0,
                                                                            KeyFramePair{ dotFadeTime,
                                                                                          fullDotOpacityValue,
                                                                                          AnimVelocityType::Cosine });
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
        Timeline::stop(positionalDotAlphaAnim, positionalDotAlphaFadeInAnimKey, positionalDotAlphaAnim->value);
        positionalDotAlphaFadeOutAnimKey = Timeline::play(positionalDotAlphaAnim, 0,
                                                          KeyFramePair{ dotFadeTime, 0.0f, AnimVelocityType::Cosine });
    }
}

void ArchPositionalDot::touchUp() {
    if ( !isFlying ) {
        Timeline::stop(positionalDotAlphaAnim, positionalDotAlphaFadeOutAnimKey, positionalDotAlphaAnim->value);
        positionalDotAlphaFadeInAnimKey = Timeline::play(positionalDotAlphaAnim, 0,
                                                         KeyFramePair{ dotFadeTime, fullDotOpacityValue,
                                                                       AnimVelocityType::Cosine });
    }
}
