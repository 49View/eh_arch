const {roleOuter, graphRel, graphWay, valueAlong, valueFlat} = require("../nameValues");
const {extrudePoly} = require("../../geometry/polygon");
const {exportGroup} = require("../nodeGraph");
const {createRoof, createComplexPolygonRoof} = require("./roof");

const HEIGHT_FOR_LEVEL = 2.97;
const DEFAULT_BUILDING_HEIGHT = HEIGHT_FOR_LEVEL * 3;
const DEFAULT_ROOF_COLOUR = "#ee2222";
const DEFAULT_BUILDING_COLOUR = "#eeeeee";

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
    if (roofShape === valueFlat) {
      roofHeight = 0;
    } else {
      roofHeight = HEIGHT_FOR_LEVEL;
    }
  }

  if (tags["roof:orientation"]) {
    roofOrientation = tags["roof:orientation"];
  } else {
    roofOrientation = valueAlong;
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
    roofShape = valueFlat;
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

const buildingFromWay = (elements, way, name) => {

  const convexHull = way.calc.convexHull;
  const orientedMinBoundingBox = way.calc.ombb;

  const buildingInfo = getBuildingInfo(way.tags);

  way.calc && way.calc.polygons.forEach(polygon => {
    //Compute lateral faces
    const lateralFaces = extrudePoly(polygon.points, buildingInfo.minHeight, buildingInfo.maxHeight);
    elements.push(exportGroup(way, name, lateralFaces, buildingInfo.colour));

    //Compute roof faces
    const roofFaces = createRoof(polygon, buildingInfo.roof, convexHull, orientedMinBoundingBox);
    elements.push(exportGroup(way,name, roofFaces, buildingInfo.roof.colour));
  });
}

const buildingFromRel = (elements, rel, name) => {

  const buildingInfo = getBuildingInfo(rel.tags);
  const polygons = rel.calc.polygons;

  let roofFaces = [];
  let lateralFaces = [];
  polygons.filter(p => p.role === roleOuter).forEach(o => {
    //Compute lateral faces
    lateralFaces = lateralFaces.concat(extrudePoly(o.points, buildingInfo.minHeight, buildingInfo.maxHeight));
    o.holes && o.holes.forEach(h => {
      lateralFaces = lateralFaces.concat(extrudePoly([...h.points].reverse(), buildingInfo.minHeight, buildingInfo.maxHeight));
    })
    elements.push(exportGroup(rel, name, lateralFaces, buildingInfo.colour));
    //Compute roof faces
    roofFaces = roofFaces.concat(createComplexPolygonRoof(o, o.holes, buildingInfo.roof));
    elements.push(exportGroup(rel, name, roofFaces, buildingInfo.roof.colour));
  })
}

const groupFromBuilding = (elements, graphNode, name) => {
  if ( graphNode.type === graphWay ) {
    buildingFromWay( elements, graphNode, name );
  } else if ( graphNode.type === graphRel ) {
    buildingFromRel( elements, graphNode, name );
  }
}

module.exports = {groupFromBuilding}
