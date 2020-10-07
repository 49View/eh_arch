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

const exportBuildings = (tileBoundary, nodes, ways, rels) => {
  console.log("----------------------------------------------");
  console.log("BUILDINGS");
  console.log("----------------------------------------------");

  const simpleBuildings = createElementsFromWays(ways, tileBoundary, "building"
    , w => w.tags && (w.tags["building"] || w.tags["building:part"])
    , buildingFromWay);
  // const simpleBuildings=[];
  const complexBuildings = createElementsFromRels(rels, tileBoundary, "building"
    , r => r.tags && (r.tags["building"] || r.tags["building:part"])
    , buildingFromRel);

  console.log(`Found ${simpleBuildings.length} simple buildings`);
  console.log(`Found ${complexBuildings.length} complex buildings`);
  console.log("----------------------------------------------");

  return simpleBuildings.concat(complexBuildings);
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

const createBuildingMesh = (id, tags, type, boundingBox, lateralFaces, roofFaces, buildingInfo) => {

  const groups = [];

  groups.push({
    faces: lateralFaces,
    part: "lateral_faces",
    colour: buildingInfo.colour,
  });
  groups.push({
    faces: roofFaces,
    part: "roof_faces",
    colour: buildingInfo.roof.colour,
  });
  return createMesh(id, tags, type, boundingBox, groups);
}

const buildingFromWay = (way, tileBoundary, name) => {

  let isOutline;

  const polygon = way.calc.polygon;
  const convexHull = way.calc.convexHull;
  const orientedMinBoundingBox = way.calc.ombb;

  isOutline = false;
  if (way.tags["building"] && way.tags["building:part"] === undefined && way.children.length > 0) {

    way.children.forEach(w => {
      if (w.tags["building:part"]) {
        isOutline = true;
      }
    })
  }

  const buildingInfo = getBuildingInfo(way.tags);
  if (isOutline && way.tags["height"]) {
    buildingInfo.minHeight = 0;
    buildingInfo.maxHeight = 0;
    buildingInfo.roof.minHeight = 0;
    buildingInfo.roof.maxHeight = 0;
    buildingInfo.roof.shape = "flat";
  }

  //Compute lateral faces
  const lateralFaces = extrudePoly(polygon, buildingInfo.minHeight, buildingInfo.maxHeight);
  //Compute roof faces
  const roofFaces = createRoof(polygon, buildingInfo.roof, convexHull, orientedMinBoundingBox);

  //Create building mesh
  return createBuildingMesh("w-" + way.id, way.tags, name, way.spatial, lateralFaces, roofFaces, buildingInfo);
}

const buildingFromRel = (rel, name) => {

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
    //Compute roof faces
    roofFaces = roofFaces.concat(createComplexPolygonRoof(o, o.holes, buildingInfo.roof));
  })
  return createBuildingMesh("r-" + rel.id, rel.tags, name, rel.spatial, lateralFaces, roofFaces, buildingInfo);
}

module.exports = {exportBuildings}
