const {exportGroup} = require("../osmHelper");
const {createComplexPolygonRoof} = require("./roof");
const {createRoof} = require("./roof");
const {
  extrudePoly,
  createMesh,
  createElementsFromWays,
  createElementsFromRels,
} = require('../osmHelper.js');

const HEIGHT_FOR_LEVEL = 2.97;
const DEFAULT_BUILDING_HEIGHT = HEIGHT_FOR_LEVEL * 3;
const DEFAULT_ROOF_COLOUR = "#ee2222";
const DEFAULT_BUILDING_COLOUR = "#eeeeee";

const exportBuildings = (elements, tileBoundary, nodes, ways, rels) => {
  console.log("----------------------------------------------");
  console.log("BUILDINGS");
  console.log("----------------------------------------------");

  createElementsFromWays(elements, ways, tileBoundary, "building"
    , w => w.tags && (w.tags["building"] || w.tags["building:part"])
    , buildingFromWay);

  createElementsFromRels(elements, rels, tileBoundary, "building"
    , r => r.tags && (r.tags["building"] || r.tags["building:part"])
    , buildingFromRel);

  console.log(`Found ${elements.length} buildings`);
  console.log("----------------------------------------------");
}

const getBuildingInfo = (tags) => {
  let minHeight, maxHeight, colour;
  let roofOrientation, roofHeight, roofShape, roofColour;

  if (tags["min_height"]) {
    minHeight = Number(tags["min_height"].replace("m", ""));
  } else if (tags["building:min_height"]) {
    minHeight = Number(tags["building:min_height"].replace("m", ""));
  } else if (tags["building:min_level"]) {
    minHeight = Number(tags["building:min_level"].replace("m", "")) * HEIGHT_FOR_LEVEL;
  } else {
    minHeight = 0;
  }
  if (tags["height"]) {
    maxHeight = Number(tags["height"].replace("m", ""));
  } else if (tags["building:height"]) {
    maxHeight = Number(tags["building:height"].replace("m", ""));
  } else if (tags["building:levels"]) {
    if (tags["building:levels"].indexOf(",") !== -1) {
      const levels = tags["building:levels"].split(",");
      maxHeight = levels.length * HEIGHT_FOR_LEVEL;
    } else {
      maxHeight = Number(tags["building:levels"].replace("m", "")) * HEIGHT_FOR_LEVEL;
    }
  } else {
    maxHeight = DEFAULT_BUILDING_HEIGHT;
  }

  if (tags["building:colour"]) {
    colour = tags["building:colour"];
  } else {
    colour = DEFAULT_BUILDING_COLOUR;
  }

  if (!colour.startsWith("#")) {
    colour = "#" + colour;
  }

  if (tags["roof:shape"] === "pyramidal" || tags["building:roof:shape"] === "pyramidal") {
    roofShape = "pyramidal";
  } else if (tags["roof:shape"] === "gabled" || tags["building:roof:shape"] === "gabled") {
    roofShape = "gabled";
  } else if (tags["roof:shape"] === "hipped" || tags["building:roof:shape"] === "hipped") {
    roofShape = "hipped";
  } else if (tags["roof:shape"] === "dome" || tags["building:roof:shape"] === "dome") {
    roofShape = "dome";
  } else { //FLAT
    roofShape = "flat";
  }

  if (tags["roof:height"]) {
    roofHeight = Number(tags["roof:height"].replace("m", ""));
  } else if (tags["building:roof:height"]) {
    roofHeight = Number(tags["building:roof:height"].replace("m", ""));
  } else if (tags["roof:levels"]) {
    roofHeight = Number(tags["roof:levels"].replace("m", "")) * HEIGHT_FOR_LEVEL;
  } else if (tags["building:roof:levels"]) {
    roofHeight = Number(tags["building:roof:levels"].replace("m", "")) * HEIGHT_FOR_LEVEL;
  } else {
    if (roofShape === "flat") {
      roofHeight = 0;
    } else {
      roofHeight = HEIGHT_FOR_LEVEL;
    }
  }

  if (tags["roof:orientation"]) {
    roofOrientation = tags["roof:orientation"];
  } else {
    roofOrientation = "along";
  }

  if (tags["roof:colour"]) {
    roofColour = tags["roof:colour"];
  } else if (tags["building:colour"]) {
    roofColour = tags["building:colour"];
  } else {
    roofColour = DEFAULT_ROOF_COLOUR;
  }
  if (!roofColour.startsWith("#")) {
    roofColour = "#" + roofColour;
  }

  if (maxHeight - roofHeight < minHeight) {
    roofHeight = 0;
    roofShape = "flat";
  } else {
    maxHeight = maxHeight - roofHeight;
  }
  const roofMinHeight = maxHeight;
  const roofMaxHeight = maxHeight + roofHeight;

  return {
    minHeight: minHeight,
    maxHeight: maxHeight,
    colour: colour,
    roof: {
      minHeight: roofMinHeight,
      maxHeight: roofMaxHeight,
      shape: roofShape,
      orientation: roofOrientation,
      colour: roofColour
    }
  }
}

const buildingFromWay = (elements, way, tileBoundary, name) => {

  const polygon = way.calc.polygon;
  const convexHull = way.calc.convexHull;
  const orientedMinBoundingBox = way.calc.ombb;

  const buildingInfo = getBuildingInfo(way.tags);

  //Compute lateral faces
  const lateralFaces = extrudePoly(polygon, buildingInfo.minHeight, buildingInfo.maxHeight);
  elements.push(exportGroup(way, "wbl-", name, lateralFaces, buildingInfo.colour));

  //Compute roof faces
  const roofFaces = createRoof(polygon, buildingInfo.roof, convexHull, orientedMinBoundingBox);
  elements.push(exportGroup(way,"wbr-", name, roofFaces, buildingInfo.roof.colour));
}

const buildingFromRel = (elements, rel, tileBoundary, name) => {

  const buildingInfo = getBuildingInfo(rel.tags);

  const polygons = rel.calc.polygons;

  let roofFaces = [];
  let lateralFaces = [];
  polygons.filter(p => p.role === "outer").forEach(o => {
    //Compute lateral faces
    lateralFaces = lateralFaces.concat(extrudePoly(o.points, buildingInfo.minHeight, buildingInfo.maxHeight));
    o.holes.forEach(h => {
      lateralFaces = lateralFaces.concat(extrudePoly([...h.points].reverse(), buildingInfo.minHeight, buildingInfo.maxHeight));
    })
    elements.push(exportGroup(rel, "rbl-", name, lateralFaces, buildingInfo.colour));
    //Compute roof faces
    roofFaces = roofFaces.concat(createComplexPolygonRoof(o, o.holes, buildingInfo.roof));
    elements.push(exportGroup(rel, "rbr-", name, roofFaces, buildingInfo.roof.colour));
  })
}

module.exports = {exportBuildings}
