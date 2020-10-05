const {groupFromNode} = require("./osmHelper");
const {createElementsFromNodes} = require("./osmHelper");
const {groupFromRel, groupFromWay} = require("./osmHelper");
const {
    createElementsFromWays,
    createElementsFromRels,
} = require('./osmHelper.js');

const createTileAreas = (tileFilter,nodes,ways,rels) => {
    console.log("----------------------------------------------");
    console.log(tileFilter.name);
    console.log("----------------------------------------------");

    const wayBasedElements=createElementsFromWays(ways, tileFilter.name
        , w => tileFilter.areaFilter(w)
        , groupFromWay);

    const relBasedElements=createElementsFromRels(rels, tileFilter.name
        , r => tileFilter.areaFilter(r)
        , groupFromRel);

    const nodeBasedElements=createElementsFromNodes(nodes, tileFilter.name
      , r => tileFilter.areaFilter(r)
      , groupFromNode);

    console.log(`Found ${wayBasedElements.length} way ${tileFilter.name}`);
    console.log(`Found ${relBasedElements.length} rels ${tileFilter.name}`);
    console.log(`Found ${nodeBasedElements.length} nodes ${tileFilter.name}`);
    console.log("----------------------------------------------");

    return wayBasedElements.concat(relBasedElements).concat(nodeBasedElements);
    // return wayBasedElements.concat(relBasedElements);
}

module.exports = { createTileAreas }
