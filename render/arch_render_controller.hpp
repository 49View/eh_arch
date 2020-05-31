//
// Created by dado on 30/05/2020.
//

#pragma once

#include <eh_arch/models/house_bsdata.hpp>
#include <graphics/renderer.h>

struct HouseBSData;

namespace ShowHouseMatrixFlags {
    static constexpr uint64_t None = 0;
    static constexpr uint64_t Show2dFloorPlan = 1u << 1u;
    static constexpr uint64_t Show3dFloorPlan = 1u << 2u;
    static constexpr uint64_t Show3dHouse = 1u << 3u;
    static constexpr uint64_t UseDebugMode = 1u << 4u;
};

using ShowHouseMatrixFlagsT = uint64_t;
using moveSelectionCallback = std::function<void(const ArchStructuralFeatureDescriptor&, const V2f& )>;

struct ArchSelectionElement {
    explicit ArchSelectionElement( HashEH elemHash ) {
        asf = ArchStructuralFeatureDescriptor{ elemHash };
    }
    ArchSelectionElement( HashEH elemHash, const V2f& initialSelectedPoint ) : initialSelectedPoint(
            initialSelectedPoint) {
        asf = ArchStructuralFeatureDescriptor{ elemHash };
    }
    ArchSelectionElement( const ArchStructuralFeatureDescriptor& asf ) : asf(asf) {}
    ArchSelectionElement( const ArchStructuralFeatureDescriptor& asf, const V2f& initialSelectedPoint ) : asf(asf),
                                                                                                          initialSelectedPoint(
                                                                                                                  initialSelectedPoint) {}

    bool operator==( const ArchSelectionElement& rhs ) const {
        return asf == rhs.asf;
    }
    bool operator!=( const ArchSelectionElement& rhs ) const {
        return !( rhs == *this );
    }

    ArchStructuralFeatureDescriptor asf;
    V2f initialSelectedPoint = V2fc::HUGE_VALUE_NEG;
};

class ArchStructuralFeatureDescriptorHashFunctor {
public:
    size_t operator()( const ArchSelectionElement& _elem ) const {
        return std::hash<std::string>{}(
                std::to_string(_elem.asf.hash) + std::to_string(_elem.asf.index) + std::to_string(
                        static_cast<int>(_elem.asf.feature)));
    }
};

class ArchSelection {
public:
    void addToSelectionList( const ArchSelectionElement& _elem ) {
        selection.emplace(_elem);
    }

    template<typename T>
    const ArchSelectionElement *find( const T& elem ) const {

        if constexpr ( std::is_same_v<T, HashEH> ) {
            if ( auto it = selection.find( ArchStructuralFeatureDescriptor{ ArchStructuralFeature::ASF_Poly, -1, elem}); it != selection.end() ) {
                return &(*it);
            }
        }

        if constexpr ( std::is_same_v<T, ArchStructuralFeatureDescriptor> ) {
            if ( auto it = selection.find(ArchSelectionElement{elem}); it != selection.end() ) {
                return &(*it);
            }
        }

        return nullptr;
    }

    void moveSelectionList(const V2f& _point, moveSelectionCallback ccf);

private:
    std::unordered_set<ArchSelectionElement, ArchStructuralFeatureDescriptorHashFunctor> selection;
};

class IMHouseRenderSettings {
public:
    IMHouseRenderSettings( const RDSPreMult& pm, FloorPlanRenderMode renderMode );
    IMHouseRenderSettings( FloorPlanRenderMode renderMode );

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
    void addToSelectionList( const T& _elem, const V3f& is ) {
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

private:
    ArchSelection selection;
    RDSPreMult mPm{ Matrix4f::MIDENTITY() };
    FloorPlanRenderMode mRenderMode = FloorPlanRenderMode::Normal2d;
    C4f selectedColor = C4f::ORANGE_SCHEME1_1;
};
