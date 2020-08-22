//
// Created by dado on 19/08/2020.
//

#include "balcony_builder.hpp"

#include <graphics/renderer.h>
#include <poly/scene_graph.h>
#include <render_scene_graph/render_orchestrator.h>
#include <eh_arch/models/house_bsdata.hpp>
#include <eh_arch/models/floor_service.hpp>

void BalconyBuilder::refresh() {
    Renderer& rr = rsg.RR();
    rr.clearBucket(rrBucket);

    if ( balconyData->epoints.empty() ) return;

    if ( balconyData->epoints.size() == 2 ) {
        rr.draw<DLine>(rrBucket, balconyData->epoints, "balconyBase");
    } else if ( balconyData->epoints.size() > 2 ) {
        rr.draw<DFlatPoly>(rrBucket, balconyData->epoints, "balconyBase");
    }
    int cc = 0;
    for ( const auto& p : balconyData->epoints ) {
        rr.draw<DCircleFilled>(rrBucket, p, 0.02f, C4f::STEEL_BLUE, std::to_string(cc++) + "balconyBase" );
    }

}

void BalconyBuilder::addPoint( const V2f& _p ) {
    balconyData->epoints.emplace_back(_p);
    balconyData->calcBBox();
    refresh();
}

BalconyBuilder::BalconyBuilder( SceneGraph& sg, RenderOrchestrator& rsg ) :
        sg(sg), rsg(rsg) {
    rrBucket = CommandBufferLimits::UnsortedStart + 3;
    balconyData = std::make_shared<BalconyBSData>();
}

const std::shared_ptr<BalconyBSData>& BalconyBuilder::BalconyData() const {
    return balconyData;
}

void BalconyBuilder::finalize() {
    Renderer& rr = rsg.RR();
    rr.clearBucket(rrBucket);
}
