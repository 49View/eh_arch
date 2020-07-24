//
// Created by dado on 23/07/2020.
//

#include "arch_explorer.hpp"
#include <core/camera.h>
#include <graphics/ghtypes.hpp>
#include <graphics/renderer.h>
#include <graphics/mouse_input.hpp>
#include <render_scene_graph/render_orchestrator.h>

#include <eh_arch/models/house_service.hpp>
#include <eh_arch/models/room_service.hpp>
#include <eh_arch/models/room_service_furniture.hpp>

inline V4f defaultDotColor() {
    return C4f::SKY_BLUE;
}

inline V4f furnitureMoveDotColor() {
    return C4f::PASTEL_GREEN;
}

void ArchExplorer::updateFurnitureSelection( RenderOrchestrator& rsg, const V3f& centerBottomPos,
                                                  const C4f& _dotColor ) {

    auto sm3 = DShaderMatrix{ DShaderMatrixValue3dColor };

    std::stringstream stream;
    stream << std::fixed << std::setprecision(2) << furnitureSelectionAlphaAnim.value();
    std::string nameTag = _dotColor.toString() + "furnitureBBox" + stream.str();

    rsg.RR().drawTriangleQuad(CommandBufferLimits::CameraMousePointers, furnitureSelectionOutline,
                              _dotColor.A(furnitureSelectionAlphaAnim.value() * 0.5f), nameTag + "a");

    rsg.RR().draw<DLine>(CommandBufferLimits::CameraMousePointers, furnitureSelectionOutline,
                         _dotColor.A(furnitureSelectionAlphaAnim.value()), sm3, nameTag, true, 0.015f);
}

bool ArchExplorer::isMouseOverFurnitureInnerSelector( const V3f& _origin, const V3f& _dir ) const {
    float farV = std::numeric_limits<float>::max();
    float vNear = 0.0f;
    return centerBottomBBox.intersectLine(_origin, _dir, vNear, farV);
}

void ArchExplorer::touchMoveWithModKeyCtrl( [[maybe_unused]] const HouseBSData *_house, const V3f& _dir,
                                                 RenderOrchestrator& rsg ) {
    if ( bFurnitureTargetLocked ) {
        bool inters = false;
        auto planeHit = furniturePlane.intersectRay(rsg.DC()->getPosition(), _dir, inters);
        V3f off = planeHit - prevFurnitureMovePosition;
        auto potentialBBox = furnitureSelected->bbox;
        potentialBBox.translate(XZY::C2(off));
        if ( !bRoomBboxCheck || RS::checkBBoxInsideRoom(fd.room, potentialBBox) ) {
            centerBottomFurnitureSelected += off;
            for ( auto& v : furnitureSelectionOutline ) v += off;
            RoomServiceFurniture::moveFurniture(fd.room, furnitureSelected, off, rsg.SG());
            bFurnitureDirty = true;
        }
        prevFurnitureMovePosition = planeHit;
    }
    rsg.DC()->enableInputs(!bFurnitureTargetLocked);
}

void ArchExplorer::firstTimeTouchDownCtrlKey( const V3f& _dir, RenderOrchestrator& rsg ) {
    if ( furnitureSelected ) {
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
    rsg.setMICursorCapture( !bFurnitureTargetLocked, bFurnitureTargetLocked||bFillFullFurnitureOutline ? MouseCursorType::HAND : MouseCursorType::ARROW );
}

void ArchExplorer::spaceToggle( RenderOrchestrator& rsg ) {
    if ( furnitureSelected && (bFurnitureTargetLocked||bFillFullFurnitureOutline) && fd.room ) {
        Quaternion quat = furnitureSelected->checkIf(FittedFurnitureFlags::FF_CanBeHanged)
                          ? QuaternionC::QuarterRollRotation : QuaternionC::QuarterYawRotation;
        RoomServiceFurniture::rotateFurniture(fd.room, furnitureSelected, quat, rsg.SG());
    }
}

void ArchExplorer::replaceFurnitureWithOneOfItsKind( RenderOrchestrator& rsg ) {

}

void ArchExplorer::deleteSelected( RenderOrchestrator& rsg ) {
    if ( furnitureSelected && (bFurnitureTargetLocked||bFillFullFurnitureOutline) && fd.room ) {
        RoomServiceFurniture::removeFurniture(fd.room, furnitureSelected, rsg.SG());
    }
}

void ArchExplorer::cloneSelected( HouseBSData *_house, RenderOrchestrator& rsg ) {
    auto ffs = EntityFactory::cloneHashed(*furnitureSelected);
    V2f pos = XZY::C2(centerBottomFurnitureSelected);
    auto f = HouseService::findFloorOf(_house, fd.room->hash);
    RS::placeManually(
            FurnitureRuleParams{ f, fd.room, ffs, pos, furnitureSelected->heightOffset, furnitureSelected->rotation, FRPFurnitureRuleFlags{ forceManualFurnitureFlags } });
}

bool ArchExplorer::touchUpWithModKeyCtrl( RenderOrchestrator& rsg ) {
    rsg.DC()->enableInputs(true);
    rsg.setMICursorCapture( true, bFillFullFurnitureOutline ? MouseCursorType::HAND : MouseCursorType::ARROW );
    furnitureSelected = nullptr;
    bFurnitureTargetLocked = false;
    bool ret = bFurnitureDirty;
    bFurnitureDirty = false;
    return ret;
}

std::vector<V3f> createBBoxOutline( const V3f& input, const V3f& axis1, const V3f& axis2 ) {
    std::vector<V3f> ret{};
    ret.push_back(input + axis1 * 0.5f - axis2 * 0.5f);
    ret.push_back(input + axis1 * 0.5f + axis2 * 0.5f);
    ret.push_back(input - axis1 * 0.5f + axis2 * 0.5f);
    ret.push_back(input - axis1 * 0.5f - axis2 * 0.5f);
    return ret;
}

void ArchExplorer::tickControlKey( const HouseBSData *_house, const V3f& _dir, RenderOrchestrator& rsg ) {

    if ( !bFurnitureTargetLocked ) {
        fdFurniture = HouseService::rayFeatureIntersect(_house, RayPair3{ rsg.DC()->getPosition(), _dir },
                                                        FeatureIntersectionFlags::FIF_Furnitures);
        fd = HouseService::rayFeatureIntersect(_house, RayPair3{ rsg.DC()->getPosition(), _dir },
                                               FeatureIntersectionFlags::FIF_Walls |
                                               FeatureIntersectionFlags::FIF_Floors);

        if ( fdFurniture.hasHit() && fdFurniture.nearV < fd.nearV) {
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
                furnitureSelectionOutline = createBBoxOutline(centerBottomFurnitureSelected,
                                                              V3f::UP_AXIS * furnitureSelected->bbox3d.calcHeight(),
                                                              V3f::Z_AXIS * furnitureSelected->bbox3d.calcDepth());
                refNormal = fd.normal;
            } else {
                centerBottomFurnitureSelected = furnitureSelected->bbox3d.centreBottom();
                furnitureSelectionOutline = furnitureSelected->bbox3d.bottomFace();
            }

            centerBottomBBox = AABB{ furnitureSelectionOutline };
            bFillFullFurnitureOutline = isMouseOverFurnitureInnerSelector(rsg.DC()->getPosition(), _dir);
        } else {
            furnitureSelectionAlphaAnim.fadeOut();
            bFillFullFurnitureOutline = false;
        }
        bool isMouseOverSelection = isMouseOverFurnitureInnerSelector(rsg.DC()->getPosition(), _dir);
        rsg.setMICursorCapture( true, isMouseOverSelection ? MouseCursorType::HAND : MouseCursorType::ARROW);
    }
    if ( furnitureSelected ) {
        updateFurnitureSelection(rsg, centerBottomFurnitureSelected, furnitureMoveDotColor());
    }
}
