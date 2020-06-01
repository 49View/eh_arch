//
// Created by dado on 30/05/2020.
//

#include "arch_render_controller.hpp"

DShaderMatrix ArchRenderController::floorPlanShader() const {
    return isFloorPlanRenderMode2d() ? DShaderMatrix{ DShaderMatrixValue2dColor } : DShaderMatrix{
            DShaderMatrixValue3dColor };
}

float ArchRenderController::floorPlanScaler( float value ) const {
    if ( isFloorPlanRenderMode2d() ) {
        return max(mPm()[0] * value, 1.0f / getScreenSizef.y());
    }
    return value;
}

Color4f ArchRenderController::floorPlanElemColor( const C4f& nominalColor ) const {
    return isFloorPlanRenderModeDebug() ? nominalColor : C4f::BLACK;
}

Color4f ArchRenderController::floorPlanElemColor() const {
    return isFloorPlanRenderModeDebug() ? Color4f::RANDA1() : C4f::BLACK;
}

ArchRenderController::ArchRenderController( const RDSPreMult& pm, FloorPlanRenderMode renderMode ) : mPm(pm),
                                                                                                     mRenderMode(
                                                                                                               renderMode) {}
ArchRenderController::ArchRenderController( FloorPlanRenderMode renderMode ) : mRenderMode(renderMode) {}

FloorPlanRenderMode ArchRenderController::renderMode() const {
    return mRenderMode;
}

bool ArchRenderController::isFloorPlanRenderModeDebug() const {
    return ::isFloorPlanRenderModeDebug(mRenderMode);
}

bool ArchRenderController::isFloorPlanRenderMode2d() const {
    return ::isFloorPlanRenderMode2d(mRenderMode);
}

void ArchRenderController::renderMode( FloorPlanRenderMode rm ) {
    mRenderMode = rm;
}
const RDSPreMult& ArchRenderController::pm() const {
    return mPm;
}

void ArchRenderController::pm( const RDSPreMult& pm ) {
    mPm = pm;
}

void ArchRenderController::moveSelectionList( const V2f& _point, moveSelectionCallback ccf ) {
    selection.moveSelectionList( _point, ccf );
}

void ArchRenderController::splitFirstEdgeOnSelectionList( splitSelectionCallback ccf ) {
    selection.splitFirstEdgeOnSelectionList( ccf );
}

void ArchRenderController::resetSelection() {
    selection.clear();
}

size_t ArchRenderController::selectionCount() const {
    return selection.count();
}

ArchStructuralFeature ArchRenderController::singleSelectedFeature() const {
    return selection.singleSelectedFeature();
}
