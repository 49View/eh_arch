//
// Created by dado on 20/06/2020.
//

#pragma once

#include <memory>

class ArchOrchestrator;
class RenderOrchestrator;
class OutdoorAreaBuilder;
class Vector2f;

struct OutdoorAreaBSData;

class OutdoorAreaUI {
public:
    [[maybe_unused]] explicit OutdoorAreaUI( OutdoorAreaBuilder& bb );
    void activate( bool _flag );
    void update( ArchOrchestrator& asg, RenderOrchestrator& rsg );
    void addPoint( const Vector2f& _p);
    [[nodiscard]] const std::shared_ptr<OutdoorAreaBSData>& OutdoorAreaData() const;
private:
    bool activated = false;
    int boundaryIndex = 0;
    OutdoorAreaBuilder& bb;
};
