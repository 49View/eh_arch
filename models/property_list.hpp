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
	std::string name = "";
	std::string type = "";
	float distance = -1.0f;
};

JSONDATA_R ( PropertyListing, name, addressLine1, addressLine2, postCode, city, region, country, latlon, price, type, numBedroom, epcRating, longBoringDesc, floorPlanImage, mainFeatures, pictureNames, schools, transport )
	std::string name = "";
	std::string addressLine1 = "";
	std::string addressLine2 = "";
	std::string postCode = "";
	std::string city = "";
	std::string region = "";
	std::string country = "";
	Vector2f latlon = Vector2f::ZERO;
	uint64_t price = 0;
	std::string type = "";
	uint64_t numBedroom = 0;
	std::string epcRating = "";
	std::string longBoringDesc = "";

	std::string floorPlanImage = "";
	std::vector<std::string> mainFeatures;
	std::vector<std::string> pictureNames;
	std::vector<PointOfInterestListing> schools;
	std::vector<PointOfInterestListing> transport;
};

struct PropertyListingWithMapping {
	std::shared_ptr< PropertyListing > pl;
	KVFMapping kvmapping;
};

class PropertyListingService {
public:
	static std::shared_ptr<PropertyListingWithMapping> load( const std::string& _name );
};
