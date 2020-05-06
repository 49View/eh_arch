//
// Created by Dado on 20/10/2019.
//

#pragma once

#include "../models/house_bsdata.hpp"
#include "../models/room_service_furniture.hpp"
#include <core/resources/resource_utils.hpp>
#include <core/resources/resource_manager.hpp>

namespace ArchResourceGroup {
    const static std::string HouseBSData   = "house";
}

template <typename R>
class ArchResourceVersioning {
public:

    inline static size_t Version() {
        if ( std::is_same<R, HouseBSData>::value )          return  1000;
        return 0;
    }

    inline static bool HasDeps() {
        if constexpr ( std::is_same_v<R, HouseBSData> )                return false;
    }

    inline static SerializableContainer HashResolver( std::shared_ptr<R> _val ) {
        if constexpr ( std::is_same_v<R, HouseBSData> )                return _val->serialize();
    }

    inline static std::string Prefix() {
        if constexpr ( std::is_same_v<R, HouseBSData> ) return ArchResourceGroup::HouseBSData;
    }

    inline static std::string GenerateThumbnail( const R& _res ) {
        if ( std::is_same<R, HouseBSData>::value    )               return "HouseBSData";
        return "unknown";
    }

};

class ArchSceneGraph;
using HouseBuilder = ResourceBuilder<HouseBSData, ArchSceneGraph, ArchResourceVersioning>;
using HB = HouseBuilder;
using HouseManager    = ResourceManager<HouseBSData, ResourceManagerContainer<HouseBSData>, ArchResourceVersioning<HouseBSData>>;

class ArchSceneGraph {
public:
    explicit ArchSceneGraph( HouseManager& _hm, FurnitureMapStorage& _furns );

    HouseManager&           HM() { return hm; }
    FurnitureMapStorage&    Furns() { return furns; }

    [[nodiscard]] std::shared_ptr<HouseBSData  >  HM( const ResourceRef& _ref ) const { return hm.get(_ref); }

    template <typename T>
    std::shared_ptr<T> get( const ResourceRef& _ref ) {
        if constexpr ( std::is_same_v<T, HouseBSData>         ) { return hm.get(_ref); }
    }

    template <typename T>
    std::vector<std::string> getNames( const ResourceRef& _ref ) {
        if constexpr ( std::is_same_v<T, HouseBSData>         ) { return hm.getNames(_ref); }
    }

    template <typename T>
    [[nodiscard]] const T* get( const ResourceRef& _ref ) const {
        if constexpr ( std::is_same_v<T, HouseBSData>         ) { return hm.get(_ref).get(); }
    }

    template <typename T>
    [[nodiscard]] const T* getPtr( const ResourceRef& _ref ) const {
        if constexpr ( std::is_same_v<T, HouseBSData>         ) { return hm.get(_ref).get(); }
    }

    template <typename T>
    ResourceRef getHash( const ResourceRef& _ref ) {
        if constexpr ( std::is_same_v<T, HouseBSData>         ) { return hm.getHash(_ref); }
    }

    template <typename R>
    auto& M() {
        if constexpr ( std::is_same_v<R, HouseBSData>         ) return HM();
    }

    template <typename R>
    static void addDeferred( const ResourceRef& _key, const ResourceRef& _hash, SerializableContainer&& _res, HttpResouceCB _ccf = nullptr ) {
        if constexpr ( std::is_same_v<R, HouseBSData          > ) resourceCallbackHouse        .emplace_back( _key, _hash, std::move(_res), _ccf );
    }
    static void addDeferredComp( const ResourceRef& _key, SerializableContainer&& _data, HttpResouceCB _ccf = nullptr ) {
        static_assert("resourceCallbackComposite not yet implemented on AddDeferredComp");
//        resourceCallbackComposite.emplace_back( _key, "", std::move(_data), _ccf );
    }

    template <typename R>
    ResourceRef add( const ResourceRef& _key, const R& _res, HttpResouceCB _ccf = nullptr ) {
        if constexpr ( std::is_same_v<R, HouseBSData          > ) return addHouse        ( _key, _res, _ccf );
    }

    ResourceRef addHouse         ( const ResourceRef& _key, const HouseBSData        & _res, HttpResouceCB _ccf = nullptr );
    void addResources( CResourceRef _key, const SerializableContainer& _data, HttpResouceCB _ccf = nullptr );

    template <typename T>
    T B( const std::string& _name ) {
        return T{ *this, _name };
    }

    void loadHouse         ( std::string _names, HttpResouceCB _ccf = nullptr );
    void loadHouse         ( const HouseBSData& _res );
    template <typename R>
    void load( std::string _names, HttpResouceCB _ccf = nullptr ) {
        replaceAllStrings( _names, " ", "," );
        if constexpr ( std::is_same_v<R, HouseBSData               > ) loadHouse        ( std::move(_names), _ccf );
    }

    void update();

    static LoadedResouceCallbackContainer resourceCallbackHouse        ;

protected:
    template <typename  T>
    void loadResourceCompositeCallback( T& cba ) {
        if ( !cba.empty()) {
            auto res = cba.back();
            addResources( res.key, res.data, res.ccf );
            cba.pop_back();
        }
    }

    template <typename R, typename BB>
    void loadResourceCallback( LoadedResouceCallbackContainer& cba ) {
        if ( !cba.empty()) {
            auto res = cba.back();
            B<BB>( res.key ).addDF( R{ res.data }, res.ccf );
            cba.pop_back();
        }
    }

    template <typename R, typename BB>
    void loadResourceCallbackWithKey( LoadedResouceCallbackContainer& cba ) {
        if ( !cba.empty()) {
            auto res = cba.back();
            auto r = R{ res.data };
            r.Key( res.key );
            B<BB>( res.key ).addDF( r, res.ccf );
            cba.pop_back();
        }
    }

    template <typename R, typename BB, typename T>
    void loadResourceCallbackWithLoader( LoadedResouceCallbackContainer& cba, T loadFunc ) {
        if ( !cba.empty()) {
            auto res = cba.back();
            auto ent = loadFunc( *this, res.key, res.hash, res.data );
            B<BB>( res.key ).addIM( ent );
            if ( res.ccf ) res.ccf( res.key );
            cba.pop_back();
        }
    }

    void publishAndAddCallback();
    void realTimeCallbacks();
    void loadCallbacks();

protected:
    HouseManager& hm;
    FurnitureMapStorage& furns;
};

