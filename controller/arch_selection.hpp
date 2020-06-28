//
// Created by dado on 31/05/2020.
//

#pragma once

#include <functional>
#include <eh_arch/models/arch_structural_service.hpp>

namespace SelectionFlags {
    constexpr uint64_t None = 0;
    constexpr uint64_t RemoveAtTouchUp = 1 << 0;
}

using SelectionFlagsT = uint64_t;

using moveSelectionCallback = std::function<void( const ArchStructuralFeatureDescriptor&, const V2f& )>;
using splitSelectionCallback = std::function<void( const ArchStructuralFeatureDescriptor&, const V2f& )>;
using deleteSelectionCallback = std::function<void( const ArchStructuralFeatureDescriptor& )>;
using toggleSelectionCallback = std::function<void( const ArchStructuralFeatureDescriptor& )>;

struct ArchSelectionElement {
    ArchSelectionElement() = default;
    explicit ArchSelectionElement( ArchBase *_elem ) {
        asf = ArchStructuralFeatureDescriptor{ _elem };
    }
    ArchSelectionElement( ArchBase *_elem, const V2f& isp ) : initialSelectedPoint(isp), lastMovingPoint(isp) {
        asf = ArchStructuralFeatureDescriptor{ _elem };
    }
    ArchSelectionElement( const ArchStructuralFeatureDescriptor& asf ) : asf(asf) {}
    ArchSelectionElement( const ArchStructuralFeatureDescriptor& asf, const V2f& isp ) : asf(asf),
                                                                                         initialSelectedPoint(isp),
                                                                                         lastMovingPoint(isp) {}
    ArchSelectionElement( const ArchStructuralFeatureDescriptor& asf, const V2f& initialSelectedPoint,
                          SelectionFlagsT flags ) : asf(asf), initialSelectedPoint(initialSelectedPoint),
                                                    lastMovingPoint(initialSelectedPoint),
                                                    flags(flags) {}

    bool operator==( const ArchSelectionElement& rhs ) const {
        return asf == rhs.asf;
    }
    bool operator!=( const ArchSelectionElement& rhs ) const {
        return !( rhs == *this );
    }

    void setLastMovingPoint( const V2f& _p ) {
        lastMovingPoint = _p;
    }

    ArchStructuralFeatureDescriptor asf;
    V2f initialSelectedPoint = V2fc::HUGE_VALUE_NEG;
    V2f lastMovingPoint = V2fc::HUGE_VALUE_NEG;
    SelectionFlagsT flags = SelectionFlags::None;
    float timeStamp = 0.0f;
};

//template <>
//struct std::hash<ArchSelectionElement> {
//    size_t operator()( const ArchSelectionElement& _elem ) const {
//        return std::hash<std::string>{}(
//                std::to_string(_elem.asf.hash) + std::to_string(_elem.asf.index) + std::to_string(
//                        static_cast<int>(_elem.asf.feature)));
//    }
//};

class ArchSelection {
public:
    void clear();
    [[nodiscard]] std::optional<ArchStructuralFeatureDescriptor> front() const;
    [[nodiscard]] size_t count() const;
    [[nodiscard]] ArchStructuralFeature singleSelectedFeature() const;

    void addToSelectionList( ArchSelectionElement _elem );

    void removeFromSelectionList( const ArchSelectionElement& _elem );

    const ArchSelectionElement *
    find( const ArchStructuralFeature& elem, int64_t _index, const ArchBase *_arch ) const {
        for ( const auto& it : selection ) {
            if ( it.asf.feature == elem && it.asf.index == _index && it.asf.elem == _arch ) {
                return &it;
            }
        }

        return nullptr;
    }

    const ArchSelectionElement *find( const ArchStructuralFeatureDescriptor& elem, float timeGap = 0.0f ) const {
        if ( auto it = std::find(selection.begin(), selection.end(), ArchSelectionElement{ elem });
                it != selection.end() &&
                ( timeGap == 0.0f || it->timeStamp == 0.0f || GameTime::getCurrTimeStamp() - it->timeStamp > timeGap ) ) {
            return &( *it );
        }
        return nullptr;
    }

    const ArchSelectionElement *find( const ArchBase *_elem, float timeGap = 0.0f ) const {
        for ( const auto& it : selection ) {
            if ( it.asf.elem == _elem && it.asf.feature == ArchStructuralFeature::ASF_Poly && ( timeGap == 0.0f || it.timeStamp == 0.0f || GameTime::getCurrTimeStamp() - it.timeStamp > timeGap ) ) {
                return &it;
            }
        }
        return nullptr;
    }

    void moveSelectionList( const V2f& _point, moveSelectionCallback ccf );
    void splitFirstEdgeOnSelectionList( splitSelectionCallback ccf );
    void createTwoShapeOnSelectedEdge( const V2f& _point, splitSelectionCallback ccf );
    void deleteElementsOnSelectionList( deleteSelectionCallback ccf );
    void toggleElementsOnSelectionList( toggleSelectionCallback ccf );

private:
    std::vector<ArchSelectionElement> selection;
};



