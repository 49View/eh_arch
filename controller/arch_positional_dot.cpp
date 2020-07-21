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

static constexpr float fullDotOpacityValue = 0.75f;
static constexpr float outerDotSize = 0.2f;
static constexpr float outerDotBorderRatio = 1.0f / 13.333333333333333333333333f;
static constexpr float smallDotRatio = 0.8f;
static constexpr float fadeOutNearDistance = 1.0f; // If dot is closer than this from the camera, linearly fade it out.
static constexpr float minSafeDistance = 0.75f; // Deduct this distance from clicked dot position to avoid being to close to walls/objects
static constexpr float slowingDownTimeOnRotationFactor = 2.0f; // When we travel to walls we'll also rotate the camera M_PI to not ending up facing the wall, that rotation needs to be slower than the usual goto time.
static constexpr float gotoAndFadeTime = 0.5f; // Time it takes to travel to point clicked _and_ to fade in/out from it
static constexpr float dotFadeTime = 0.15f;

inline V4f defaultDotColor() {
    return C4f::SKY_BLUE;
}

inline V4f furnitureMoveDotColor() {
    return C4f::PASTEL_GREEN;
}

FadeInOutContainer::FadeInOutContainer() {
    floatAnim = std::make_shared<AnimType<float >>(fullDotOpacityValue, UUIDGen::make());
}

float FadeInOutContainer::value() const {
    return floatAnim->value;
}

floata& FadeInOutContainer::anim() {
    return floatAnim;
}

void FadeInOutContainer::setValue( float _value ) {
    floatAnim->value = _value;
}

void FadeInOutContainer::fade( FadeInternalPhase phase ) {
    auto hasStopped = Timeline::stop(anim(), phase == FadeInternalPhase::In ? outPhaseId : inPhaseId,
                                     value());
    if ( !anim()->isAnimating || hasStopped ) {
        if ( phase == FadeInternalPhase::In ) {
            inPhaseId = Timeline::play(anim(), 0,
                                                KeyFramePair{ dotFadeTime, fullDotOpacityValue,AnimVelocityType::Cosine });
        } else {
            outPhaseId = Timeline::play(anim(), 0,
                                                 KeyFramePair{ dotFadeTime, 0.0f, AnimVelocityType::Cosine });
        }
    }
}

void FadeInOutContainer::fadeIn() {
    fade(FadeInternalPhase::In);
}
void FadeInOutContainer::fadeOut() {
    fade(FadeInternalPhase::Out);
}


// *********************************************************************************************************************
// *********************************************************************************************************************
// Moving around Dot
// *********************************************************************************************************************
// *********************************************************************************************************************


void drawDotCircled( RenderOrchestrator& rsg, float dotSize, const V3f& centrePoint, const V3f& normal,
                     const C4f& _dotColor, float finalAlphaValue ) {
    C4f outerDotColor = _dotColor.A(finalAlphaValue);
    auto sm3 = DShaderMatrix{ DShaderMatrixValue3dColor };
    std::stringstream stream;
    stream << std::fixed << std::setprecision(2) << finalAlphaValue;
    std::string nameTag = _dotColor.toString() + stream.str();
    rsg.RR().draw<DCircleFilled>(CommandBufferLimits::CameraMousePointers, centrePoint,
                                 V4f::WHITE.A(finalAlphaValue),
                                 dotSize + outerDotSize * outerDotBorderRatio, sm3, RDSRotationNormalAxis{ normal },
                                 RDSArchSegments{ 36 }, "d1" + nameTag);
    rsg.RR().draw<DCircleFilled>(CommandBufferLimits::CameraMousePointers, centrePoint, outerDotColor,
                                 dotSize, sm3, RDSRotationNormalAxis{ normal }, RDSArchSegments{ 36 },
                                 "d2" + nameTag);
    rsg.RR().draw<DCircleFilled>(CommandBufferLimits::CameraMousePointers, centrePoint,
                                 V4f::WHITE.A(finalAlphaValue),
                                 dotSize * smallDotRatio, sm3, RDSRotationNormalAxis{ normal },
                                 RDSArchSegments{ 36 }, "d3" + nameTag);
}

void ArchPositionalDot::updateDot( RenderOrchestrator& rsg, const C4f& _dotColor ) {
    float alphaDistanceAttenuation = min(distance(hitPosition, rsg.DC()->getPosition()), fadeOutNearDistance);
    float finalAlphaValue = positionalDotAlphaAnim.value() * alphaDistanceAttenuation;

    drawDotCircled(rsg, outerDotSize, hitPosition, fd.normal, _dotColor, finalAlphaValue);

    hitPosition.setY(rsg.DC()->getPosition().y());
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
ArchPositionalDot::tick( const HouseBSData *_house, const V3f& _dir, RenderOrchestrator& rsg, bool isComingFromInternal, const C4f& _dotColor ) {
    if ( !isFlying ) {
        if ( !isComingFromInternal ) {
            rsg.setMICursorCapture(true);
            furnitureSelectionAlphaAnim.setValue( 0.0f );
        }
        fd = HouseService::rayFeatureIntersect(_house, RayPair3{ rsg.DC()->getPosition(), _dir },
                                               FeatureIntersectionFlags::FIF_Walls |
                                               FeatureIntersectionFlags::FIF_Floors);
        if ( fd.hasHit() ) {
            float safeDist = fd.nearV > minSafeDistance ? fd.nearV - minSafeDistance : 0.0f;
            hitPosition = rsg.DC()->getPosition() + ( _dir * safeDist );
            updateDot(rsg, _dotColor);
        }
    }
}

// *********************************************************************************************************************
// *********************************************************************************************************************
// Furniture selector
// *********************************************************************************************************************
// *********************************************************************************************************************

void ArchPositionalDot::updateFurnitureSelection( RenderOrchestrator& rsg, const V3f& centerBottomPos, const C4f& _dotColor ) {

    auto sm3 = DShaderMatrix{ DShaderMatrixValue3dColor };

    std::stringstream stream;
    stream << std::fixed << std::setprecision(2) << furnitureSelectionAlphaAnim.value();
    std::string nameTag = _dotColor.toString() + "furnitureBBox" + stream.str();

    rsg.RR().drawTriangleQuad(CommandBufferLimits::CameraMousePointers, furnitureSelectionOutline,
                            _dotColor.A(furnitureSelectionAlphaAnim.value()*0.5f), nameTag+"a" );

    rsg.RR().draw<DLine>(CommandBufferLimits::CameraMousePointers, furnitureSelectionOutline,
                         _dotColor.A(furnitureSelectionAlphaAnim.value()), sm3, nameTag, true, 0.015f);
}

bool ArchPositionalDot::isMouseOverFurnitureInnerSelector(const V3f& _origin, const V3f& _dir) const {
    float farV = std::numeric_limits<float>::max();
    float vNear = 0.0f;
    return centerBottomBBox.intersectLine(_origin, _dir, vNear, farV);
}

void ArchPositionalDot::touchMoveWithModKeyCtrl( [[maybe_unused]] const HouseBSData *_house, const V3f& _dir, RenderOrchestrator& rsg ) {
    if ( bFurnitureTargetLocked ) {
        bool inters = false;
        auto planeHit = furniturePlane.intersectRay(rsg.DC()->getPosition(), _dir, inters);
        V3f off = planeHit - prevFurnitureMovePosition;
        auto potentialBBox = furnitureSelected->bbox;
        potentialBBox.translate(XZY::C2(off));
        if ( !bRoomBboxCheck || RS::checkBBoxInsideRoom(fd.room, potentialBBox) ) {
            auto node = rsg.SG().Nodes().find(furnitureSelected->linkedUUID);
            if ( node->second ) {
                node->second->move(off);
            }
            centerBottomFurnitureSelected += off;
            for ( auto& v : furnitureSelectionOutline ) v+=off;
            furnitureSelected->position3d += off;
            furnitureSelected->bbox3d.translate(off);
            furnitureSelected->bbox = potentialBBox;
        }
        prevFurnitureMovePosition = planeHit;
    }
}

void ArchPositionalDot::firstTimeTouchDownCtrlKey( const V3f& _dir, RenderOrchestrator& rsg ) {
    if ( furnitureSelected && !isFlying ) {
        bFurnitureTargetLocked = isMouseOverFurnitureInnerSelector(rsg.DC()->getPosition(), _dir);
        if ( bFurnitureTargetLocked ) {
            if ( furnitureSelected->checkIf(FittedFurnitureFlags::FF_CanBeHanged) ) {
                furniturePlane = Plane3f{ fd.normal, centerBottomFurnitureSelected };
            } else {
                furniturePlane = Plane3f{ centerBottomFurnitureSelected, centerBottomFurnitureSelected + V3f::X_AXIS,
                                          centerBottomFurnitureSelected + V3f::Z_AXIS };
            }
            bool inters = false;
            prevFurnitureMovePosition = furniturePlane.intersectRay(rsg.DC()->getPosition(), _dir, inters);
        }
    }
}

void ArchPositionalDot::touchUpWithModKeyCtrl() {
    furnitureSelected = nullptr;
    bFurnitureTargetLocked = false;
}

std::vector<V3f> createBBoxOutline( const V3f& input, const V3f& axis1, const V3f& axis2) {
    std::vector<V3f> ret{};
    ret.push_back( input + axis1*0.5f - axis2*0.5f );
    ret.push_back( input + axis1*0.5f + axis2*0.5f );
    ret.push_back( input - axis1*0.5f + axis2*0.5f );
    ret.push_back( input - axis1*0.5f - axis2*0.5f );
    return ret;
}

void ArchPositionalDot::tickControlKey( const HouseBSData *_house, const V3f& _dir, RenderOrchestrator& rsg ) {

//    if ( _aid.isMouseTouchedUp(TOUCH_ZERO) && bHitFurniture && isControlKeyDown ) {
//        asg.MakeHouse3d{}(asg, rsg, arc);
//        asg.pushHouseChange();
//    }

    if ( !isFlying ) {
        rsg.setMICursorCapture(false);
        if ( !bFurnitureTargetLocked ) {
            fdFurniture = HouseService::rayFeatureIntersect(_house, RayPair3{ rsg.DC()->getPosition(), _dir },
                                                            FeatureIntersectionFlags::FIF_Furnitures);
            fd = HouseService::rayFeatureIntersect( _house, RayPair3{ rsg.DC()->getPosition(), _dir },
                                                    FeatureIntersectionFlags::FIF_Walls |
                                                    FeatureIntersectionFlags::FIF_Floors);

            if ( fdFurniture.hasHit() ) {
                V3f refNormal = V3f::UP_AXIS;
                furnitureSelectionAlphaAnim.fadeIn();
                furnitureSelected = dynamic_cast<FittedFurniture *>(fdFurniture.arch);

                if ( furnitureSelected->checkIf(FittedFurnitureFlags::FF_CanBeHanged) ) {
                    float closestDist = dot(fd.normal, V3f::X_AXIS);
                    centerBottomFurnitureSelected = furnitureSelected->bbox3d.centreFront();
                    if ( auto d = dot(fd.normal, V3f::X_AXIS_NEG); d > closestDist ) {
                        centerBottomFurnitureSelected = furnitureSelected->bbox3d.centreBack();
                        closestDist = d;
                    }
                    if ( auto d = dot(fd.normal, V3f::Z_AXIS); d > closestDist ) {
                        centerBottomFurnitureSelected = furnitureSelected->bbox3d.centreLeft();
                        closestDist = d;
                    }
                    if ( auto d = dot(fd.normal, V3f::Z_AXIS_NEG); d > closestDist ) {
                        centerBottomFurnitureSelected = furnitureSelected->bbox3d.centreRight();
                    }
                    furnitureSelectionOutline = createBBoxOutline(centerBottomFurnitureSelected, V3f::UP_AXIS*furnitureSelected->bbox3d.calcHeight(), V3f::Z_AXIS*furnitureSelected->bbox3d.calcDepth());
                    refNormal = fd.normal;
                } else {
                    centerBottomFurnitureSelected = furnitureSelected->bbox3d.centreBottom();
                    furnitureSelectionOutline = furnitureSelected->bbox.points3d_xzy();
                }

                centerBottomBBox = AABB{furnitureSelectionOutline};
                bFillFullFurnitureOutline = isMouseOverFurnitureInnerSelector(rsg.DC()->getPosition(), _dir);
            } else {
                furnitureSelectionAlphaAnim.fadeOut();
                bFillFullFurnitureOutline = false;
            }
            bool isMouseOverSelection = isMouseOverFurnitureInnerSelector(rsg.DC()->getPosition(), _dir);
            if ( fd.hasHit() ) {
                float safeDist = fd.nearV > minSafeDistance ? fd.nearV - minSafeDistance : 0.0f;
                hitPosition = rsg.DC()->getPosition() + ( _dir * safeDist );
                positionalDotAlphaAnim.fade(isMouseOverSelection ? FadeInternalPhase::Out : FadeInternalPhase::In);
                updateDot(rsg, furnitureMoveDotColor());
            }
        }
        if ( furnitureSelected ) {
            updateFurnitureSelection(rsg, centerBottomFurnitureSelected, furnitureMoveDotColor() );
        }
    }
}
