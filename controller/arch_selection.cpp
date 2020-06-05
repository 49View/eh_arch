//
// Created by dado on 31/05/2020.
//

#include "arch_selection.hpp"

void ArchSelection::moveSelectionList( const V2f& _point, moveSelectionCallback ccf ) {
    for ( const auto& s1 : selection ) {
        if ( s1.asf.feature == ArchStructuralFeature::ASF_Point ) {
            auto offset = s1.asf.pointOfInterests[0] + ( _point - s1.initialSelectedPoint );
            ccf(s1.asf, offset);
        } else if ( s1.asf.feature == ArchStructuralFeature::ASF_Edge ) {
            float offsetDistance = distance(_point, s1.initialSelectedPoint);
            auto cn = rotate90(s1.asf.normalDirection);
            auto l1 = s1.initialSelectedPoint + cn;
            auto l2 = s1.initialSelectedPoint - cn;
            float sol = -sideOfLine(_point, l1, l2);
            auto offset = s1.asf.normalDirection * offsetDistance * sol;
            ccf(s1.asf, offset);
        }
    }
}

void ArchSelection::splitFirstEdgeOnSelectionList( splitSelectionCallback ccf ) {
    auto s1 = *( selection.begin() );
    if ( s1.asf.feature == ArchStructuralFeature::ASF_Edge ) {
        auto middle = lerp(0.5f, s1.asf.pointOfInterests[0], s1.asf.pointOfInterests[1]);
        middle += s1.asf.normalDirection * distance(s1.asf.pointOfInterests[0], s1.asf.pointOfInterests[1]) * 0.05f;
        ccf(s1.asf, middle);
    }
}

void ArchSelection::deleteElementsOnSelectionList( deleteSelectionCallback ccf ) {
    for ( auto s1 : selection ) {
        ccf(s1.asf);
    }
}

void ArchSelection::clear() {
    selection.clear();
}

size_t ArchSelection::count() const {
    return selection.size();
}

ArchStructuralFeature ArchSelection::singleSelectedFeature() const {
    return selection.empty() ? ArchStructuralFeature::ASF_None : selection.begin()->asf.feature;
}


