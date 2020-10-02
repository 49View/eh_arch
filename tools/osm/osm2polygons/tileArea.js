const {groupFromRel, groupFromWay} = require("./osmHelper");
const {
    createElementsFromWays,
    createElementsFromRels,
} = require('./osmHelper.js');

const createTileAreas = (tileFilter,nodes,ways,rels) => {
    console.log("----------------------------------------------");
    console.log(tileFilter.name);
    console.log("----------------------------------------------");

    const simpleParks=createElementsFromWays(ways, tileFilter.name
        , w => tileFilter.areaFilter(w)
        , groupFromWay);

    const complexParks=createElementsFromRels(rels, tileFilter.name
        , r => tileFilter.areaFilter(r)
        , groupFromRel);

    console.log(`Found ${simpleParks.length} way ${tileFilter.name}`);
    console.log(`Found ${complexParks.length} rels ${tileFilter.name}`);
    console.log("----------------------------------------------");

    return simpleParks.concat(complexParks);
}

module.exports = { createTileAreas }
