//
// Created by dado on 30/05/2020.
//

#pragma once

#include <eh_arch/models/house_bsdata.hpp>
#include <graphics/renderer.h>
#include "arch_selection.hpp"

struct HouseBSData;

enum class ArchViewingMode {
    AVM_TopDown2d,
    AVM_TopDown3d,
    AVM_Walk,
    AVM_DollHouse
};

enum class FloorPlanRenderMode {
    Normal2d,
    Normal3d,
    Debug2d,
    Debug3d,
    Debug3dSelection
};

template<typename T>
bool isFloorPlanRenderModeDebug( T _flag ) {
    return _flag == T::Debug2d || _flag == T::Debug3d || _flag == T::Debug3dSelection;
}

template<typename T>
bool isFloorPlanRenderMode2d( T _flag ) {
    return _flag == T::Debug2d || _flag == T::Normal2d;
}

template<typename T>
bool isFloorPlanRenderModeSelection( T _flag ) {
    return _flag == T::Debug3dSelection;
}

DShaderMatrix floorPlanShader( FloorPlanRenderMode fpRenderMode );
Vector4f floorPlanElemColor( FloorPlanRenderMode fpRenderMode, const Vector4f& nominalColor );
Vector4f floorPlanElemColor( FloorPlanRenderMode fpRenderMode );
float floorPlanScaler( FloorPlanRenderMode fpRenderMode, float value, const Matrix4f& pm );

class ArchRenderController {
public:
    ArchRenderController() = default;

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
    bool isFloorPlanRenderModeDebugSelection() const;

    void addToSelectionList( ArchBase* _elem ) {
        selection.addToSelectionList(ArchSelectionElement{ _elem } );
    }

    void setSelectionList( ArchBase* _elem ) {
        selection.clear();
        selection.addToSelectionList(ArchSelectionElement{ _elem } );
    }

    template<typename T>
    void addToSelectionList( const T& _elem, const V2f& is, SelectionFlagsT flags = SelectionFlags::None ) {
        selection.addToSelectionList({ _elem, is, flags });
    }

    template<typename T>
    void toggleSelection( const T& _elem, const V2f& is, SelectionFlagsT flags = SelectionFlags::None ) {
        if ( selection.find(_elem) ) {
            selection.removeFromSelectionList(_elem);
        } else {
            selection.addToSelectionList({ _elem, is, flags });
        }
    }

    template<typename T>
    void singleToggleSelection( const T& _elem, const V2f& is, SelectionFlagsT flags = SelectionFlags::None ) {
        if ( selection.find(_elem) ) {
            selection.clear();
        } else {
            selection.clear();
            selection.addToSelectionList({ _elem, is, flags });
        }
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
        return getFillColor( elem, c1, c1);
    }

    C4f getFillColor( ArchStructuralFeature _asf, int64_t _index, const ArchBase* _elem, const C4f& c1 ) const {
        if ( const auto* it = selection.find(_asf, _index, _elem); it ) {
            return selectedColor;
        }

        return c1;
    }

    template<typename T>
    [[nodiscard]] bool isSelected( const T& elem ) const {
        if ( const auto* it = selection.find(elem); it ) {
            return true;
        }
        return false;
    }


    [[nodiscard]] ArchStructuralFeature singleSelectedFeature() const;
    void moveSelectionList(const V2f& _point, moveSelectionCallback ccf);
    void splitFirstEdgeOnSelectionList( splitSelectionCallback ccf );
    void createTwoShapeOnSelectedEdge( const V2f& _point, splitSelectionCallback ccf );
    void deleteElementsOnSelectionList( deleteSelectionCallback ccf );
    void toggleElementsOnSelectionList( toggleSelectionCallback ccf );
    void resetSelection();
    std::optional<ArchStructuralFeatureDescriptor> selectionFront() const;
    size_t selectionCount() const;

    [[nodiscard]] float getFloorPlanTransparencyFactor() const;
    void setFloorPlanTransparencyFactor( float _value );
    float& FloorPlanTransparencyFactor();

    ArchViewingMode getViewingMode() const;
    void setViewingMode( ArchViewingMode _viewingMode );
private:
    ArchSelection selection;
    ArchViewingMode viewingMode = ArchViewingMode::AVM_Walk;
    RDSPreMult mPm{ Matrix4f::MIDENTITY() };
    FloorPlanRenderMode mRenderMode = FloorPlanRenderMode::Normal2d;
    C4f selectedColor = C4f::ORANGE_SCHEME1_1;
    float floorPlanTransparencyFactor = 0.5f;
};
