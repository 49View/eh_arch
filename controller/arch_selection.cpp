//
// Created by dado on 31/05/2020.
//

#include "arch_selection.hpp"

void ArchSelection::moveSelectionList( const V2f& _point, moveSelectionCallback ccf ) {
    for ( auto& s1 : selection ) {
        if ( s1.asf.feature == ArchStructuralFeature::ASF_Point ) {
            auto offset = s1.asf.pointOfInterests[0] + ( _point - s1.initialSelectedPoint );
            ccf(s1.asf, offset);
        } else if ( s1.asf.feature == ArchStructuralFeature::ASF_Poly ) {
            auto offset = _point - s1.lastMovingPoint;
            ccf(s1.asf, offset);
        } else if ( s1.asf.feature == ArchStructuralFeature::ASF_Edge ) {
            auto dom = s1.asf.normalDirection.dominantElement();
            float offsetDistance = distance(_point[dom], s1.initialSelectedPoint[dom]);
            auto cn = rotate90(s1.asf.normalDirection);
            auto l1 = s1.initialSelectedPoint + cn;
            auto l2 = s1.initialSelectedPoint - cn;
            float sol = -sideOfLine(_point, l1, l2);
            auto offset = s1.asf.normalDirection * offsetDistance * sol;
            ccf(s1.asf, offset);
        }
        s1.lastMovingPoint = _point;
    }

}

void ArchSelection::createTwoShapeOnSelectedEdge( const V2f& _point, splitSelectionCallback ccf ) {
    auto s1 = *( selection.begin() );
    if ( s1.asf.feature == ArchStructuralFeature::ASF_Edge ) {
//        auto middle = lerp(0.5f, s1.asf.pointOfInterests[0], s1.asf.pointOfInterests[1]);
//        middle += s1.asf.normalDirection * distance(s1.asf.pointOfInterests[0], s1.asf.pointOfInterests[1]) * 0.05f;
        ccf(s1.asf, _point);
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

void ArchSelection::toggleElementsOnSelectionList( toggleSelectionCallback ccf ) {
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

std::optional<ArchStructuralFeatureDescriptor> ArchSelection::front() const {
    if ( selection.empty() ) return std::nullopt;
    return selection.begin()->asf;
}

ArchStructuralFeature ArchSelection::singleSelectedFeature() const {
    return selection.empty() ? ArchStructuralFeature::ASF_None : selection.begin()->asf.feature;
}

void ArchSelection::addToSelectionList( const ArchSelectionElement& _elem ) {
    if ( auto it = std::find( selection.begin(), selection.end(), _elem); it == selection.end() ) {
        selection.emplace_back(_elem);
    }
}

void ArchSelection::removeFromSelectionList( const ArchSelectionElement& _elem ) {
    selection.erase(std::find( selection.begin(), selection.end(), _elem));
}

