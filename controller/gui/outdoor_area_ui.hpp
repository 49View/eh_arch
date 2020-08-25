//
// Created by dado on 20/06/2020.
//

#pragma once

class ArchOrchestrator;
class RenderOrchestrator;

class OutdoorAreaUI {
public:
    void activate( bool _flag );
    void update( ArchOrchestrator& asg, RenderOrchestrator& rsg );

private:
    bool activated = false;
};
