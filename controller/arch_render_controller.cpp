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
    return isFloorPlanRenderModeDebug() ? nominalColor : C4fc::BLACK;
}

Color4f ArchRenderController::floorPlanElemColor() const {
    return isFloorPlanRenderModeDebug() ? C4fc::RANDA1() : C4fc::BLACK;
}

FloorPlanRenderMode ArchRenderController::renderMode() const {
    return mRenderMode;
}

bool ArchRenderController::isFloorPlanRenderModeDebug() const {
    return ::isFloorPlanRenderModeDebug(mRenderMode);
}

bool ArchRenderController::isFloorPlanRenderModeDebugSelection() const {
    return isFloorPlanRenderModeSelection(mRenderMode);
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

void ArchRenderController::createTwoShapeOnSelectedEdge( const V2f& _point, splitSelectionCallback ccf ) {
}

void ArchRenderController::deleteElementsOnSelectionList( deleteSelectionCallback ccf ) {
    selection.deleteElementsOnSelectionList( ccf );
}

void ArchRenderController::toggleElementsOnSelectionList( toggleSelectionCallback ccf ) {
    selection.toggleElementsOnSelectionList( ccf );
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

std::optional<ArchStructuralFeatureDescriptor> ArchRenderController::selectionFront() const {
    return selection.front();
}

ArchViewingMode ArchRenderController::getViewingMode() const {
    return viewingMode;
}

void ArchRenderController::setViewingMode( ArchViewingMode _viewingMode ) {
    viewingMode = _viewingMode;
}

