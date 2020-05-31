//
// Created by dado on 30/05/2020.
//

#pragma once

#include <eh_arch/models/house_bsdata.hpp>
#include <graphics/renderer.h>
#include "arch_selection.hpp"

struct HouseBSData;

class ArchRenderController {
public:
    ArchRenderController( const RDSPreMult& pm, FloorPlanRenderMode renderMode );
    ArchRenderController( FloorPlanRenderMode renderMode );

    [[nodiscard]] FloorPlanRenderMode renderMode() const;
    void renderMode( FloorPlanRenderMode rm );
    [[nodiscard]]const RDSPreMult& pm() const;
    void pm( const RDSPreMult& pm );

    DShaderMatrix floorPlanShader() const;
    float floorPlanScaler( float value ) const;
    C4f floorPlanElemColor( const C4f& nominalColor ) const;
    C4f floorPlanElemColor() const;
    bool isFloorPlanRenderModeDebug() const;
    bool isFloorPlanRenderMode2d() const;

    template<typename T>
    void addToSelectionList( const T& _elem, const V2f& is ) {
        selection.addToSelectionList({ _elem, is });
    }

    template<typename T>
    C4f getFillColor( const T& elem, const C4f& c1, const C4f& c2 ) const {

        if ( const auto* it = selection.find(elem); it ) {
            return selectedColor;
        }

        if ( isFloorPlanRenderModeDebug() ) {
            return c1;
        }

        return c2;
    }

    template<typename T>
    C4f getFillColor( const T& elem, const C4f& c1 ) const {
        return getFillColor(elem, c1, c1);
    }

    void moveSelectionList(const V2f& _point, moveSelectionCallback ccf);
    void resetSelection();

private:
    ArchSelection selection;
    RDSPreMult mPm{ Matrix4f::MIDENTITY() };
    FloorPlanRenderMode mRenderMode = FloorPlanRenderMode::Normal2d;
    C4f selectedColor = C4f::ORANGE_SCHEME1_1;
};
