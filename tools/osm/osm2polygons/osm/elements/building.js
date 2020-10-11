const {graphTypeNode, roleInner, valueAlong, valueFlat, valueAirShaft} = require("../nameValues");
const {extrudePoly} = require("../../geometry/polygon");
const {serializeElement, serializeMesh} = require("../serialization");
const {createRoof} = require("./roof");

const HEIGHT_FOR_LEVEL = 2.97;
const DEFAULT_BUILDING_HEIGHT = HEIGHT_FOR_LEVEL * 3;
const DEFAULT_ROOF_COLOUR = "#ee2222";
const DEFAULT_BUILDING_COLOUR = "#eeeeee";

const getDefaultBuildingHeight = (buildingType) => {

  if (buildingType) {
    if (buildingType === valueAirShaft) {
      return 0.1;
    }
  }
  return DEFAULT_BUILDING_HEIGHT;
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
  } else if (tags["layer"]) {
    maxHeight = HEIGHT_FOR_LEVEL * (Number(tags["layer"])+1);
  } else {
    maxHeight = getDefaultBuildingHeight(tags["building"]);
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

const groupFromBuilding = (elements, graphNode, name) => {

  if (graphNode.type !== graphTypeNode) {
    const buildingInfo = getBuildingInfo(graphNode.tags);
    const convexHull = graphNode.calc.convexHull;
    const orientedMinBoundingBox = graphNode.calc.ombb;
    const polygons = graphNode.calc.polygons;

    polygons.filter(p => !p.role || p.role !== roleInner).forEach(o => {

      //Compute lateral faces
      let lateralFaces = extrudePoly(o.points, buildingInfo.minHeight, buildingInfo.maxHeight);
      o.holes && o.holes.forEach(h => {
        lateralFaces = lateralFaces.concat(extrudePoly([...h.points].reverse(), buildingInfo.minHeight, buildingInfo.maxHeight));
      });

      //Compute roof faces
      const roofFaces = createRoof(o, buildingInfo.roof, convexHull, orientedMinBoundingBox);

      let lateralMesh = serializeMesh(lateralFaces, buildingInfo.colour, "lateral");
      let roofMesh    = serializeMesh(roofFaces, buildingInfo.roof.colour, "roof");
      elements.push(serializeElement(graphNode, name, [lateralMesh, roofMesh]));
    });
  }
}

module.exports = {groupFromBuilding}
