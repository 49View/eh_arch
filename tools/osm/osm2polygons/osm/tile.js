const {groupFromBarrier} = require("./buildings/barrier");
const {tagBarrier} = require("./nameValues");
const {groupFromBuilding} = require("./buildings/building");
const {groupFromGraphNode} = require("./nodeGraph");
const {createElements} = require('./nodeGraph.js');

const buildingFilter = w => {
    return w.tags && (w.tags["building"] || w.tags["building:part"]);
}

const parkFilter = w => {
    return w.tags && (w.tags["leisure"] || (w.tags["landuse"] && (w.tags["landuse"] !== "construction" && w.tags["landuse"] !== "governmental")) || (w.tags["area"] && w.tags["man_made"] && !w.tags["ferry"]));
}

const parkingFilter = w => {
    return w.tags && (w.tags["amenity"] === "parking");
}

const waterFilter = w => {
    return w.tags && w.tags["natural"] === "water";
}

const barrierFilter = w => {
    return w.tags && w.tags[tagBarrier];
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

const addTileAreaFilter = (name, areaFilter, elaborateCallback = groupFromGraphNode ) => {
    return {
        name,
        areaFilter,
        elaborateCallback
    }
}

const createTileAreas = (elements, tileFilter, nodes, ways, rels) => {
    console.log("----------------------------------------------");
    console.log(tileFilter.name);
    console.log("----------------------------------------------");

    createElements(elements, ways, tileFilter.name
        , w => tileFilter.areaFilter(w)
        , tileFilter.elaborateCallback);

    createElements(elements, rels, tileFilter.name
        , r => tileFilter.areaFilter(r)
      , tileFilter.elaborateCallback);

    createElements(elements, nodes, tileFilter.name
      , r => tileFilter.areaFilter(r)
      , tileFilter.elaborateCallback);

    console.log(`Found ${elements.length} ${tileFilter.name}`);
    // console.log(`Found ${wayBasedElements.length} way ${tileFilter.name}`);
    // console.log(`Found ${relBasedElements.length} rels ${tileFilter.name}`);
    // console.log(`Found ${nodeBasedElements.length} nodes ${tileFilter.name}`);
    console.log("----------------------------------------------");
}

const createTile = (tileBoundary, nodes, ways, rels) => {

    const elements = [];
    // exportBuildings(elements, nodes, ways, rels);

    const tileAreas = [
        addTileAreaFilter("building", buildingFilter, groupFromBuilding),
        addTileAreaFilter("unclassified", unclassifiedFilter),
        addTileAreaFilter("park", parkFilter),
        addTileAreaFilter("parking", parkingFilter),
        addTileAreaFilter("water", waterFilter),
        addTileAreaFilter(tagBarrier, barrierFilter, groupFromBarrier),
        addTileAreaFilter("road", roadFilter),
        addTileAreaFilter("tree", treeFilter),
    ]

    tileAreas.forEach( tf => {
        createTileAreas(elements, tf, nodes, ways, rels);
    });

    return elements;
}

module.exports = { createTile }
