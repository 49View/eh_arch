#include "property_list.hpp"

template <typename T>
void mapkvArray( std::map< std::string, std::string >& kvmapping, const std::string _key, const std::vector<T>& _array, std::function<std::string(const int)> mf ) {
	for ( auto t = 0u; t < _array.size(); t++ ) {
		kvmapping[_key + std::to_string( t )] = mf(t);
	}
}

void mapkv( std::shared_ptr< PropertyListing >& pl, std::map< std::string, std::string >& kvmapping ) {
	kvmapping["$price"] = priceMaker( pl->price );
	mapkvArray( kvmapping, "$cover_image", pl->pictureNames, [pl]( const int index ) -> std::string { return pl->pictureNames[index]; } );
	kvmapping["$line1Info"] = std::to_string( pl->numBedroom ) + " Bedroom " + pl->type;
	kvmapping["$line2Info"] = pl->addressLine1;

	mapkvArray( kvmapping, "$line1Schools", pl->schools, [pl]( const int index ) -> std::string { return pl->schools[index].name; } );
	mapkvArray( kvmapping, "$line2Schools", pl->schools, [pl]( const int index ) -> std::string { return distaneToString( pl->schools[index].distance ); } );

	mapkvArray( kvmapping, "$line1Transport", pl->transport, [pl]( const int index ) -> std::string { return pl->transport[index].name; } );
	mapkvArray( kvmapping, "$line2Transport", pl->transport, [pl]( const int index ) -> std::string { return distaneToString( pl->transport[index].distance ); } );
}

std::shared_ptr<PropertyListingWithMapping> PropertyListingService::load( const std::string& _name ) {
	std::shared_ptr<PropertyListingWithMapping> ret = std::make_shared<PropertyListingWithMapping>();
	
	ret->pl = std::make_shared<PropertyListing>(_name);

	ret->kvmapping["$price"] = priceMaker( ret->pl->price );
//	mapkv( ret->pl, ret->kvmapping );
	
	return ret;
}

//private:
//	std::map< std::string, std::string > kvmapping;
//void PropertyListing::deserialize( const std::string _name ) {
//	mapkv();
//}
//
