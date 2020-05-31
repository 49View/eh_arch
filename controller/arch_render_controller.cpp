//
// Created by dado on 30/05/2020.
//

#include "arch_render_controller.hpp"

DShaderMatrix IMHouseRenderSettings::floorPlanShader() const {
    return isFloorPlanRenderMode2d() ? DShaderMatrix{ DShaderMatrixValue2dColor } : DShaderMatrix{
            DShaderMatrixValue3dColor };
}

float IMHouseRenderSettings::floorPlanScaler( float value ) const {
    if ( isFloorPlanRenderMode2d() ) {
        return max(mPm()[0] * value, 1.0f / getScreenSizef.y());
    }
    return value;
}

Color4f IMHouseRenderSettings::floorPlanElemColor( const C4f& nominalColor ) const {
    return isFloorPlanRenderModeDebug() ? nominalColor : C4f::BLACK;
}

Color4f IMHouseRenderSettings::floorPlanElemColor() const {
    return isFloorPlanRenderModeDebug() ? Color4f::RANDA1() : C4f::BLACK;
}

IMHouseRenderSettings::IMHouseRenderSettings( const RDSPreMult& pm, FloorPlanRenderMode renderMode ) : mPm(pm),
                                                                                                       mRenderMode(
                                                                                                               renderMode) {}
IMHouseRenderSettings::IMHouseRenderSettings( FloorPlanRenderMode renderMode ) : mRenderMode(renderMode) {}

FloorPlanRenderMode IMHouseRenderSettings::renderMode() const {
    return mRenderMode;
}

bool IMHouseRenderSettings::isFloorPlanRenderModeDebug() const {
    return ::isFloorPlanRenderModeDebug(mRenderMode);
}

bool IMHouseRenderSettings::isFloorPlanRenderMode2d() const {
    return ::isFloorPlanRenderMode2d(mRenderMode);
}

void IMHouseRenderSettings::renderMode( FloorPlanRenderMode rm ) {
    mRenderMode = rm;
}
const RDSPreMult& IMHouseRenderSettings::pm() const {
    return mPm;
}

void IMHouseRenderSettings::pm( const RDSPreMult& pm ) {
    mPm = pm;
}

void IMHouseRenderSettings::moveSelectionList( const V2f& _point, moveSelectionCallback ccf ) {
    selection.moveSelectionList( _point, ccf );
}
