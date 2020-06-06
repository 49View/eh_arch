//
// Created by dado on 31/05/2020.
//

#pragma once

#include <functional>
#include <eh_arch/models/arch_structural_service.hpp>

using moveSelectionCallback = std::function<void( const ArchStructuralFeatureDescriptor&, const V2f& )>;
using splitSelectionCallback = std::function<void( const ArchStructuralFeatureDescriptor&, const V2f& )>;
using deleteSelectionCallback = std::function<void( const ArchStructuralFeatureDescriptor& )>;
using toggleSelectionCallback = std::function<void( const ArchStructuralFeatureDescriptor& )>;

struct ArchSelectionElement {
    explicit ArchSelectionElement( HashEH elemHash ) {
        asf = ArchStructuralFeatureDescriptor{ elemHash };
    }
    ArchSelectionElement( HashEH elemHash, const V2f& isp ) : initialSelectedPoint(isp) {
        asf = ArchStructuralFeatureDescriptor{ elemHash };
    }
    ArchSelectionElement( const ArchStructuralFeatureDescriptor& asf ) : asf(asf) {}
    ArchSelectionElement( const ArchStructuralFeatureDescriptor& asf, const V2f& isp ) : asf(asf),
                                                                                         initialSelectedPoint(isp) {}

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
    void clear();
    [[nodiscard]] size_t count() const;
    [[nodiscard]] ArchStructuralFeature singleSelectedFeature() const;

    void addToSelectionList( const ArchSelectionElement& _elem ) {
        selection.emplace(_elem);
    }

    template<typename T>
    const ArchSelectionElement *find( const T& elem ) const {

        if constexpr ( std::is_same_v<T, HashEH> ) {
            if ( auto it = selection.find(ArchStructuralFeatureDescriptor{ ArchStructuralFeature::ASF_Poly, -1, elem });
                    it != selection.end() ) {
                return &( *it );
            }
        }

        if constexpr ( std::is_same_v<T, ArchStructuralFeatureDescriptor> ) {
            if ( auto it = selection.find(ArchSelectionElement{ elem }); it != selection.end() ) {
                return &( *it );
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
    std::unordered_set<ArchSelectionElement, ArchStructuralFeatureDescriptorHashFunctor> selection;
};



