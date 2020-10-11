const {groupFromGraphNode} = require("./elements/road");
const {groupFromBarrier} = require("./elements/barrier");
const {tagBarrier} = require("./nameValues");
const {groupFromBuilding} = require("./elements/building");
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
    const subwayPathFilter = w=> ( !w.tags["layer"] || w.tags["layer"] >= 0);

    return w.tags && w.tags["highway"] && subwayPathFilter(w);
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

    let countStart = elements.length;

    createElements(elements, ways, tileFilter.name
        , w => tileFilter.areaFilter(w)
        , tileFilter.elaborateCallback);

    console.log(`Found ${elements.length-countStart} way ${tileFilter.name}`);
    countStart = elements.length;

    createElements(elements, rels, tileFilter.name
        , r => tileFilter.areaFilter(r)
      , tileFilter.elaborateCallback);

    console.log(`Found ${elements.length-countStart} rel ${tileFilter.name}`);
    countStart = elements.length;

    createElements(elements, nodes, tileFilter.name
      , r => tileFilter.areaFilter(r)
      , tileFilter.elaborateCallback);

    console.log(`Found ${elements.length-countStart} node ${tileFilter.name}`);

    console.log("----------------------------------------------");
}

const createTile = (tileBoundary, nodes, ways, rels) => {

    const elements = [];
    // exportBuildings(elements, nodes, ways, rels);

    const tileAreas = [
        addTileAreaFilter("building", buildingFilter, groupFromBuilding),
        addTileAreaFilter("tree", treeFilter),
        addTileAreaFilter("unclassified", unclassifiedFilter),
        addTileAreaFilter("park", parkFilter),
        addTileAreaFilter("parking", parkingFilter),
        addTileAreaFilter("water", waterFilter),
        addTileAreaFilter("road", roadFilter),
        addTileAreaFilter(tagBarrier, barrierFilter, groupFromBarrier),
    ]

    tileAreas.forEach( tf => {
        createTileAreas(elements, tf, nodes, ways, rels);
    });

    return elements;
}

module.exports = { createTile }
