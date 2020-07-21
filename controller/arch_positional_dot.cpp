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
static constexpr float outerDotBorderRatio = 1.0f/13.333333333333333333333333f;
static constexpr float smallDotRatio = 0.8f;
static constexpr float fadeOutNearDistance = 1.0f; // If dot is closer than this from the camera, linearly fade it out.
static constexpr float minSafeDistance = 0.75f; // Deduct this distance from clicked dot position to avoid being to close to walls/objects
static constexpr float slowingDownTimeOnRotationFactor = 2.0f; // When we travel to walls we'll also rotate the camera M_PI to not ending up facing the wall, that rotation needs to be slower than the usual goto time.
static constexpr float gotoAndFadeTime = 0.5f; // Time it takes to travel to point clicked _and_ to fade in/out from it
static constexpr float dotFadeTime = 0.15f;

ArchPositionalDot::ArchPositionalDot() {
    positionalDotAlphaAnim = std::make_shared<AnimType<float >>(fullDotOpacityValue, "positionalDotAlphaAnim");
}

void drawDotCircled( RenderOrchestrator& rsg, float dotSize, const V3f& hitPosition, const V3f& normal, float finalAlphaValue ) {
    C4f outerDotColor = V4f::SKY_BLUE.A(finalAlphaValue);
    auto sm3 = DShaderMatrix{ DShaderMatrixValue3dColor };
    std::stringstream stream;
    stream << std::fixed << std::setprecision(2) << finalAlphaValue;
    std::string nameTag = stream.str();
    rsg.RR().draw<DCircleFilled>(CommandBufferLimits::CameraMousePointers, hitPosition,
                                 V4f::WHITE.A(finalAlphaValue),
                                 dotSize + outerDotSize*outerDotBorderRatio, sm3, RDSRotationNormalAxis{ normal },
                                 RDSArchSegments{ 36 }, "d1" + nameTag);
    rsg.RR().draw<DCircleFilled>(CommandBufferLimits::CameraMousePointers, hitPosition, outerDotColor,
                                 dotSize, sm3, RDSRotationNormalAxis{ normal }, RDSArchSegments{ 36 },
                                 "d2" + nameTag);
    rsg.RR().draw<DCircleFilled>(CommandBufferLimits::CameraMousePointers, hitPosition,
                                 V4f::WHITE.A(finalAlphaValue),
                                 dotSize * smallDotRatio, sm3, RDSRotationNormalAxis{ normal },
                                 RDSArchSegments{ 36 }, "d3" + nameTag);
}

void ArchPositionalDot::updateDot( RenderOrchestrator& rsg ) {
    float alphaDistanceAttenuation = min(distance(hitPosition, rsg.DC()->getPosition()), fadeOutNearDistance);
    float finalAlphaValue = positionalDotAlphaAnim->value * alphaDistanceAttenuation;
    
    drawDotCircled( rsg, outerDotSize,  hitPosition, fd.normal, finalAlphaValue );

    hitPosition.setY(rsg.DC()->getPosition().y());
}

void ArchPositionalDot::updateFurnitureSelection( RenderOrchestrator& rsg, const V3f& centerBottomPos ) {

    drawDotCircled( rsg, 0.15f, centerBottomPos, V3f::UP_AXIS, 1.0f );

    rsg.RR().draw<DLine>(CommandBufferLimits::CameraMousePointers, furnitureSelected->bbox.points3d_xzy(),
                         V4f::SKY_BLUE.A(0.33f), "furnbbox", true, 0.015f);

}

void ArchPositionalDot::touchMoveWithModKeyCtrl( const HouseBSData *_house,  const V3f& _dir, RenderOrchestrator& rsg ) {
    if ( bFurnitureTargetLocked ) {
        bool inters = false;
        auto planeHit = furniturePlane.intersectRay( rsg.DC()->getPosition(), _dir, inters );
        V3f off = planeHit - prevFurnitureMovePosition;
        auto potentialBBox = furnitureSelected->bbox;
        potentialBBox.translate(XZY::C2(off));
        if ( !bRoomBboxCheck || RS::checkBBoxInsideRoom(fd.room, potentialBBox) ) {
            auto node = rsg.SG().Nodes().find(furnitureSelected->linkedUUID);
            if ( node->second ) {
                node->second->move(off);
            }
            centerBottomFurnitureSelected += off;
            furnitureSelected->position3d += off;
            furnitureSelected->bbox3d.translate(off);
            furnitureSelected->bbox = potentialBBox;
        }
        prevFurnitureMovePosition = planeHit;
    }
}

/// @brief
/// This is working nicely, only thing left to fix is the positionChangedIn which is not implemented fully because of
/// the way we do intersections every frame, we would need to store a last known good intersection but it will be stuck
/// to that single point so I think it will look horrible, needs to provide a better solution.
/// \param _aid
void ArchPositionalDot::tick( const HouseBSData *_house, const V3f& _dir, RenderOrchestrator& rsg ) {
    if ( !isFlying ) {
        fd = HouseService::rayFeatureIntersect(_house, RayPair3{ rsg.DC()->getPosition(), _dir },
                                               FeatureIntersectionFlags::FIF_Walls |
                                               FeatureIntersectionFlags::FIF_Floors);
        if ( fd.hasHit() ) {
            float safeDist = fd.nearV > minSafeDistance ? fd.nearV - minSafeDistance : 0.0f;
            hitPosition = rsg.DC()->getPosition() + ( _dir * safeDist );
            updateDot(rsg);
        }
    }
}

void ArchPositionalDot::tickControlKey( const HouseBSData *_house, const V3f& _dir, RenderOrchestrator& rsg ) {

//    if ( _aid.isMouseTouchedUp(TOUCH_ZERO) && bHitFurniture && isControlKeyDown ) {
//        asg.MakeHouse3d{}(asg, rsg, arc);
//        asg.pushHouseChange();
//    }

    if ( !isFlying ) {
        if ( !bFurnitureTargetLocked) {
            fdFurniture = HouseService::rayFeatureIntersect(_house, RayPair3{ rsg.DC()->getPosition(), _dir },
                                                            FeatureIntersectionFlags::FIF_Furnitures);
            if ( fdFurniture.hasHit() ) {
                furnitureSelected = dynamic_cast<FittedFurniture *>(fdFurniture.arch);
                centerBottomFurnitureSelected = furnitureSelected->bbox3d.centreBottom();
            }
        }
        if ( furnitureSelected ) {
            updateFurnitureSelection(rsg, centerBottomFurnitureSelected);
        }
    }
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

void ArchPositionalDot::firstTimeTouchDownCtrlKey(const V3f& _dir, RenderOrchestrator& rsg) {
    if ( furnitureSelected && !isFlying ) {
        V3f ext = V3f{0.15f, 0.01f, 0.15f};
        JMATH::AABB centerBottomBBox{ centerBottomFurnitureSelected - ext, centerBottomFurnitureSelected + ext };

        float farV = std::numeric_limits<float>::max();
        float vNear = 0.0f;
        bFurnitureTargetLocked = centerBottomBBox.intersectLine( rsg.DC()->getPosition(), _dir, vNear, farV);
        if ( bFurnitureTargetLocked ) {
            auto topDownOutline = centerBottomBBox.topDownOutline();
            furniturePlane = Plane3f{centerBottomFurnitureSelected, centerBottomFurnitureSelected+V3f::X_AXIS, centerBottomFurnitureSelected+V3f::Z_AXIS};
            bool inters = false;
            prevFurnitureMovePosition = furniturePlane.intersectRay( rsg.DC()->getPosition(), _dir, inters );
        }

//        Timeline::stop(positionalDotAlphaAnim, positionalDotAlphaFadeInAnimKey, positionalDotAlphaAnim->value);
//        positionalDotAlphaFadeOutAnimKey = Timeline::play(positionalDotAlphaAnim, 0,
//                                                          KeyFramePair{ dotFadeTime, 0.0f, AnimVelocityType::Cosine });
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

void ArchPositionalDot::touchUpWithModKeyCtrl() {
    furnitureSelected = nullptr;
    bFurnitureTargetLocked = false;
}
