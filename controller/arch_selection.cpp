//
// Created by dado on 31/05/2020.
//

#include "arch_selection.hpp"

void ArchSelection::moveSelectionList( const V2f& _point, moveSelectionCallback ccf ) {
    for ( const auto& s1 : selection ) {
        if ( s1.asf.feature == ArchStructuralFeature::ASF_Point ) {
            auto offset = s1.asf.pointOfInterests[0] + (_point - s1.initialSelectedPoint);
            ccf( s1.asf, offset );
        }
    }
}

void ArchSelection::clear() {
    selection.clear();
}
