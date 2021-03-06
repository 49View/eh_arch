//
// Created by dado on 19/08/2020.
//

#include "outdoor_area_builder.hpp"

#include <graphics/renderer.h>
#include <poly/scene_graph.h>
#include <render_scene_graph/render_orchestrator.h>
#include <eh_arch/models/house_bsdata.hpp>

static constexpr float snapThresholdOutdoorArea = 0.2f;

void OutdoorAreaBuilder::refresh() {

    Renderer& rr = rsg.RR();
    rr.clearBucket(rrBucket);

    if ( empty() ) return;

    std::size_t cc = 0;

    std::size_t bc = 0;
    for ( const auto& oa : outdoorAreaData()->Boundaries() ) {
        if ( oa.extrusionType == 0 ) {
            if ( oa.bPoints.size() > 2 ) {
                rr.draw<DFlatPoly>(rrBucket, oa.bPoints, "outdoorAreaBaseP1" + std::to_string(cc++));
            }
        } else if ( oa.extrusionType == 1 ) {
            if ( oa.bPoints.size() > 1 ) {
                rr.draw<DLine>(rrBucket, oa.bPoints, C4fc::SKY_BLUE, "outdoorAreaBaseL1" + std::to_string(cc++), oa.followerWidth);
            }
        }
        auto dotColor = bc == outdoorAreaData()->Boundaries().size() - 1 ? C4fc::PASTEL_ORANGE : C4fc::STEEL_BLUE;
        for ( const auto& p : oa.bPoints ) {
            rr.draw<DCircleFilled>(rrBucket, p, 0.04f, dotColor, std::to_string(cc++) + "outdoorAreaBase");
        }
        ++bc;
    }
}

void OutdoorAreaBuilder::addPoint( const V2f& _p, int _bIndex ) {
    auto snapped = XZY::C2(snapTo(XZY::C(_p, outdoorAreaData()->Boundary(_bIndex).elevation), computedSnapPoints,
                                  snapThresholdOutdoorArea));
    outdoorAreaData()->Boundary(_bIndex).bPoints.emplace_back(snapped);
    outdoorAreaData.push();
    computedSnapPoints.emplace_back(XZY::C(snapped, outdoorAreaData()->Boundary(_bIndex).elevation));
    refresh();
}

[[maybe_unused]] OutdoorAreaBuilder::OutdoorAreaBuilder( RenderOrchestrator& rsg ) : rsg(rsg) {
    rrBucket = CommandBufferLimits::UnsortedStart + 3;
    clear();
}

const std::shared_ptr<OutdoorAreaBSData>& OutdoorAreaBuilder::OutdoorAreaData() const {
    return outdoorAreaData();
}

void OutdoorAreaBuilder::OutdoorAreaData( std::shared_ptr<OutdoorAreaBSData> _outdoorAreaData ) {
    outdoorAreaData.push(_outdoorAreaData);
}

void OutdoorAreaBuilder::clear() {
    Renderer& rr = rsg.RR();
    rr.clearBucket(rrBucket);
    computedSnapPoints.clear();
}

void OutdoorAreaBuilder::cloneBoundary( std::size_t _index ) {
    auto newOutdoor = outdoorAreaData()->Boundary(_index);
    newOutdoor.elevation += newOutdoor.zPull;
    newOutdoor.outdoorBoundaryMaterial = MaterialAndColorProperty{"fence"};
    newOutdoor.extrusionType = 1;
    outdoorAreaData()->Boundaries().emplace_back(newOutdoor);
    refresh();
}

bool OutdoorAreaBuilder::isActive() const {
    return outdoorAreaData() != nullptr;
}

std::vector<OutdoorBoundary>& OutdoorAreaBuilder::Boundaries() {
    return outdoorAreaData()->Boundaries();
}

bool OutdoorAreaBuilder::empty() const {
    return !outdoorAreaData() || outdoorAreaData()->Boundaries().empty();
}

void OutdoorAreaBuilder::addBoundary( const OutdoorBoundary& _boundary ) {
    if ( !outdoorAreaData() ) {
        outdoorAreaData.push();
    }
    outdoorAreaData()->Boundaries().emplace_back(_boundary);
}

void OutdoorAreaBuilder::undo() {
    if ( outdoorAreaData.undo() ) {
        refresh();
    }
}

void OutdoorAreaBuilder::redo() {
    if ( outdoorAreaData.redo() ) {
        refresh();
    }
}

void OutdoorAreaBuilder::reset() {
    outdoorAreaData.reset();
    clear();
}

void OutdoorAreaBuilder::makeBalcony() {
    outdoorAreaData()->Boundary(0).outdoorBoundaryMaterial = MaterialAndColorProperty{"rustic"};
    auto newOutdoor = outdoorAreaData()->Boundary(0);
    newOutdoor.elevation += newOutdoor.zPull;
    newOutdoor.zPull = 0.8f;
    newOutdoor.followerWidth = 0.05f;
    newOutdoor.outdoorBoundaryMaterial = MaterialAndColorProperty{"plastic"};
    newOutdoor.extrusionType = 1;
    outdoorAreaData()->Boundaries().emplace_back(newOutdoor);
    refresh();
}

void OutdoorAreaBuilder::makeGarden() {
    auto newOutdoor = outdoorAreaData()->Boundary(0);
    newOutdoor.elevation += newOutdoor.zPull;
    newOutdoor.zPull = 2.0f;
    newOutdoor.followerWidth = 0.02f;
    newOutdoor.outdoorBoundaryMaterial = MaterialAndColorProperty{"fence"};
    newOutdoor.extrusionType = 1;
    outdoorAreaData()->Boundaries().emplace_back(newOutdoor);
    refresh();
}

void OutdoorAreaBuilder::makeTerrace() {

}
