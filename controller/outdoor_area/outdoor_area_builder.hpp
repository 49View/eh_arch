//
// Created by dado on 19/08/2020.
//

#pragma once

#include <memory>
#include <core/math/vector3f.h>
#include <core/memento.hpp>
#include <eh_arch/models/house_bsdata.hpp>

class SceneGraph;
class RenderOrchestrator;
class ArchOrchestrator;

class OutdoorAreaBuilder {
public:
    [[maybe_unused]] OutdoorAreaBuilder( SceneGraph& sg, RenderOrchestrator& rsg );
    void addPoint( const V2f& _p, int bIndex );
    void undo();
    void redo();
    [[nodiscard]] const std::shared_ptr<OutdoorAreaBSData>& OutdoorAreaData() const;
    void OutdoorAreaData( std::shared_ptr<OutdoorAreaBSData> outdoorAreaData );
    void clear();
    void reset();
    void cloneBoundary( std::size_t _index );
    std::vector<OutdoorBoundary>& Boundaries();
    [[nodiscard]]bool empty() const;
    void addBoundary( const OutdoorBoundary& _boundary);
    [[nodiscard]] bool isActive() const;
    void refresh();

    void makeBalcony();
    void makeGarden();
    void makeTerrace();

private:
    SceneGraph& sg;
    Memento<OutdoorAreaBSData> outdoorAreaData;
    std::vector<V3f> computedSnapPoints{};
    RenderOrchestrator& rsg;
    int rrBucket = 0;
};
