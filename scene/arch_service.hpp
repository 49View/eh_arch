//
// Created by Dado on 22/10/2019.
//

#pragma once

#include <core/http/webclient.h>

class ArchSceneGraph;
class SceneGraph;
struct HouseBSData;

class ArchService {
public:
    ArchService( ArchSceneGraph& asg, SceneGraph& sg );

    void loadHouse( ResourceRef name );
    void loadHouse( const HouseBSData& _house );
    void publishHouse( std::shared_ptr<HouseBSData>, const std::string& );


    [[nodiscard]] std::string socketMessageHouseElaborate() const {
        return std::string(smSocketMessageHouseElaborate);
    }
private:
    void loadHouseInternal( const HouseBSData& _house );
    void loadHouseFromSocketJson( const std::string& _msg, SocketCallbackDataType&& _data );

private:
    ArchSceneGraph& asg;
    SceneGraph& sg;

private:
    const static constexpr auto smSocketMessageHouseElaborate = "HouseElaborate";
};
