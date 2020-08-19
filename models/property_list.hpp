//
//  property_list.hpp
//  bob
//
//  Created by Dado on 05/10/2015.
//
//

#pragma once

#include "core/kvfmapping.h"
#include "core/serialization.hpp"

JSONDATA ( PointOfInterestListing, name, type, distance )
	std::string name;
	std::string type;
	float distance = -1.0f;
};

JSONDATA ( HouseLocation, coordinates, type )
    V2f coordinates{V2fc::ZERO};
    std::string type{"point"};
};

JSONDATA ( PropertyListing, _id, origin, addressLine1, addressLine2, addressLine3, buyOrLet, description, estateAgentId, keyFeatures, location, name, price, priceReadable, priceUnity, status, userId, floorplanUrl, images, thumbs )
	std::string _id;
	std::string origin;
	std::string addressLine1;
	std::string addressLine2;
	std::string addressLine3;
	std::string buyOrLet{};
	std::string description{};
	std::string estateAgentId{};
	std::vector<std::string> keyFeatures{};
	HouseLocation location{};
	std::string name;
	std::vector<int> price;
	std::string priceReadable;
	std::string priceUnity;
	std::string status;
	std::string userId;
	std::string floorplanUrl;
    std::vector<std::string> images{};
    std::vector<std::string> thumbs{};
};
