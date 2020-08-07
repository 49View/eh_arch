//
//  room_name_mapping.cpp
//  sixthmaker
//
//  Created by Dado on 23/07/2017.
//
//

#include "room_name_mapping.hpp"

std::map<std::string, ASTypeT> RoomNameMapping::map {
        {"bedroom",  ASType::BedroomDouble},
        {"bedmom",  ASType::BedroomDouble},
        {"conservatory",  ASType::Conservatory},
        {"dining",  ASType::DiningRoom},
        {"recreation",  ASType::LivingRoom},
        {"reception",  ASType::LivingRoom},
        {"recepnon",  ASType::LivingRoom},
        {"living",  ASType::LivingRoom},
        {"lounge",  ASType::LivingRoom},
        {"kitchen",  ASType::Kitchen},
        {"khchen",  ASType::Kitchen},
        {"bathroom",  ASType::Bathroom},
        {"ensuite",  ASType::EnSuite},
        {"en-suite",  ASType::EnSuite},
        {"en-suile",  ASType::EnSuite},
        {"garage",  ASType::Garage},
        {"garden room",  ASType::Conservatory},
        {"studio",  ASType::Studio}
};
