const {exportBuildings} = require("./buildings");
const {groupFromNode} = require("./osmHelper");
const {createElementsFromNodes} = require("./osmHelper");
const {groupFromRel, groupFromWay} = require("./osmHelper");
const {
    createElementsFromWays,
    createElementsFromRels,
} = require('./osmHelper.js');

const parkFilter = w => {
    return w.tags && (w.tags["leisure"] || (w.tags["landuse"] && (w.tags["landuse"] !== "construction" && w.tags["landuse"] !== "governmental")) || (w.tags["area"] && w.tags["man_made"] && !w.tags["ferry"]));
}

const parkingFilter = w => {
    return w.tags && (w.tags["amenity"] === "parking");
}

const waterFilter = w => {
    return w.tags && w.tags["natural"] === "water";
}

const fenceFilter = w => {
    return w.tags && w.tags["barrier"];
}

const roadFilter = w => {
    return w.tags && (w.tags["highway"]);
}

const treeFilter = w => {
    return w.tags && (w.tags["natural"] === "tree");
}

const unclassifiedFilter = w => {
    return w.tags === undefined && w.nodes && w.nodes.length > 0;
}

const addTileAreaFilter = (name, areaFilter) => {
    return {
        name,
        areaFilter
    }
}

const createTileAreas = (tileFilter, tileBoundary, nodes,ways,rels) => {
    console.log("----------------------------------------------");
    console.log(tileFilter.name);
    console.log("----------------------------------------------");

    const wayBasedElements=createElementsFromWays(ways, tileBoundary, tileFilter.name
        , w => tileFilter.areaFilter(w)
        , groupFromWay);

    const relBasedElements=createElementsFromRels(rels, tileBoundary, tileFilter.name
        , r => tileFilter.areaFilter(r)
        , groupFromRel);

    const nodeBasedElements=createElementsFromNodes(nodes, tileBoundary, tileFilter.name
      , r => tileFilter.areaFilter(r)
      , groupFromNode);

    console.log(`Found ${wayBasedElements.length} way ${tileFilter.name}`);
    console.log(`Found ${relBasedElements.length} rels ${tileFilter.name}`);
    console.log(`Found ${nodeBasedElements.length} nodes ${tileFilter.name}`);
    console.log("----------------------------------------------");

    return wayBasedElements.concat(relBasedElements).concat(nodeBasedElements);
}

const createTile = (tileBoundary, nodes, ways, rels) => {
    let elements = [];

    elements = elements.concat(exportBuildings(tileBoundary, nodes, ways, rels));

    const tileAreas = [
        addTileAreaFilter("unclassified", unclassifiedFilter),
        addTileAreaFilter("park", parkFilter),
        addTileAreaFilter("parking", parkingFilter),
        addTileAreaFilter("water", waterFilter),
        addTileAreaFilter("barrier", fenceFilter),
        addTileAreaFilter("road", roadFilter),
        addTileAreaFilter("tree", treeFilter),
    ]

    tileAreas.forEach( tf => {
        elements = elements.concat(createTileAreas(tf, tileBoundary, nodes, ways, rels));
    });

    return elements;
}

module.exports = { createTile }
