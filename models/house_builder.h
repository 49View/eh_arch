//
// Created by Dado on 04/02/2018.
//

#pragma once

#include "core/builders.hpp"
#include "core/observable.h"
#include "core/file_manager.h"

namespace HBOS { // House Builder Observer Strings
    const static std::string HMB = "HMB";
    const static std::string HM  = "HM";
    const static std::string HC  = "HC";
};

struct HouseBSData;

//class HouseBuilder : public ResourceBuilderObservable, public Observable<HouseBuilder> {
//public:
//    virtual ~HouseBuilder();
//    using ResourceBuilderObservable::ResourceBuilderObservable;
//    template<typename T>
//    bool build(std::shared_ptr<T> _observer) {
//        subscribe(_observer);
//        readRemote<HouseBuilder, HttpQuery::JSON, HouseBSData>( Name(), *this );
//        return true;
//    }
//    bool make( uint8_p&& _data, const DependencyStatus _status ) override;
//    const std::shared_ptr<HouseBSData>& getHouse() const;
//protected:
//    std::shared_ptr<HouseBSData> house;
//};
