//
// Created by dado on 19/08/2020.
//

#include "outdoor_area_builder.hpp"

#include <graphics/renderer.h>
#include <poly/scene_graph.h>
#include <render_scene_graph/render_orchestrator.h>
#include <eh_arch/models/house_bsdata.hpp>

void OutdoorAreaBuilder::refresh() {
    Renderer& rr = rsg.RR();
    rr.clearBucket(rrBucket);

    if ( outdoorAreaData->epoints.empty() ) return;

    if ( outdoorAreaData->epoints.size() == 2 ) {
        rr.draw<DLine>(rrBucket, outdoorAreaData->epoints, "outdoorAreaBase");
    } else if ( outdoorAreaData->epoints.size() > 2 ) {
        rr.draw<DFlatPoly>(rrBucket, outdoorAreaData->epoints, "outdoorAreaBase");
    }
    int cc = 0;
    for ( const auto& p : outdoorAreaData->epoints ) {
        rr.draw<DCircleFilled>(rrBucket, p, 0.02f, C4f::STEEL_BLUE, std::to_string(cc++) + "outdoorAreaBase" );
    }

}

void OutdoorAreaBuilder::addPoint( const V2f& _p ) {
    outdoorAreaData->epoints.emplace_back(_p);
    outdoorAreaData->calcBBox();
    refresh();
}

OutdoorAreaBuilder::OutdoorAreaBuilder( SceneGraph& sg, RenderOrchestrator& rsg ) :
        sg(sg), rsg(rsg) {
    rrBucket = CommandBufferLimits::UnsortedStart + 3;
    outdoorAreaData = std::make_shared<OutdoorAreaBSData>();
}

const std::shared_ptr<OutdoorAreaBSData>& OutdoorAreaBuilder::OutdoorAreaData() const {
    return outdoorAreaData;
}

void OutdoorAreaBuilder::finalize() {
    Renderer& rr = rsg.RR();
    rr.clearBucket(rrBucket);
}
