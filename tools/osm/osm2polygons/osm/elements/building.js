const {serializeVertices} = require("../nameValues");
const {convertOSMBuildingMaterialStringToColor} = require("../nameValues");
const {convertOSMColorStringToColor} = require("../nameValues");
const {valueDone} = require("../nameValues");
const {valueHipped} = require("../nameValues");
const {valueGabled} = require("../nameValues");
const {valuePyramidal} = require("../nameValues");
const {graphTypeNode, roleInner, valueAlong, valueFlat, valueAirShaft} = require("../nameValues");
const {serializeElement, serializeMesh} = require("../serialization");
const {createRoof} = require("./roof");

const HEIGHT_FOR_LEVEL = 2.97;

const getDefaultBuildingHeightWithRandomFactor = (randomFactor) => {
  return (HEIGHT_FOR_LEVEL * 3.0) *  (1.0 + Math.random()*randomFactor);
}

const getDefaultBuildingHeight = (buildingType) => {

  if (buildingType) {
    if (buildingType === valueAirShaft) {
      return 0.1;
    }
  }
  return getDefaultBuildingHeightWithRandomFactor(0.1);
}

const getRandomDefaultBuildingColor = type => {
  const defaultResidentialBuildingColors = [
    "#DBCDBC",
    "#D4B2AA",
    "#D2C5BC",
    "#BC9799"];

  const defaultCommercialBuildingColors = [
    "#7C7462",
    "#E3E2E0",
    "#E1E2E2",
    "#D2C5BC"];

  const randomResidential = defaultResidentialBuildingColors[Math.floor(Math.random() * defaultResidentialBuildingColors.length)];
  const randomCommercial = defaultCommercialBuildingColors[Math.floor(Math.random() * defaultCommercialBuildingColors.length)];
  if ( type && type !== "yes" ) {
    switch (type) {
      case "residential":
        return randomResidential;
      default:
        return randomCommercial;
    }
  }

  return Math.random() > 0.5 ? randomCommercial : randomResidential;
}

const getBuildingInfo = (tags) => {
  let minHeight, maxHeight;
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
  // Check if minHeight == maxHeight, in this case just add a small delta
  if ( maxHeight === minHeight ) {
    maxHeight = minHeight + 0.1;
  }


  let buildingColor = tags["building:colour"];
  const buildingMaterial = tags["building:material"];

  if (buildingColor) {
    buildingColor = convertOSMColorStringToColor(buildingColor);
  } else if (!buildingColor && buildingMaterial) {
    buildingColor = convertOSMBuildingMaterialStringToColor(buildingMaterial);
  } else {
    buildingColor = getRandomDefaultBuildingColor(tags["building"]);
  }

  if (!buildingColor.startsWith("#")) {
    buildingColor = "#" + buildingColor;
  }

  if (tags["roof:shape"] === "pyramidal" || tags["building:roof:shape"] === "pyramidal") {
    roofShape = valuePyramidal;
  } else if (tags["roof:shape"] === "gabled" || tags["building:roof:shape"] === "gabled") {
    roofShape = valueGabled;
  } else if (tags["roof:shape"] === "hipped" || tags["building:roof:shape"] === "hipped") {
    roofShape = valueHipped;
  } else if (tags["roof:shape"] === "dome" || tags["building:roof:shape"] === "dome") {
    roofShape = valueDone;
  } else { //FLAT
    roofShape = valueFlat;
  }

  if (tags["roof:height"]) {
    roofHeight = Number(tags["roof:height"].replace("m", ""));
  } else if (tags["building:roof:height"]) {
    roofHeight = Number(tags["building:roof:height"].replace("m", ""));
  } else if (tags["roof:levels"]) {
    roofHeight = Number(tags["roof:levels"].replace("m", "")) * HEIGHT_FOR_LEVEL;
    if ( roofShape === valueFlat ) roofShape = "gabled";
  } else if (tags["building:roof:levels"]) {
    roofHeight = Number(tags["building:roof:levels"].replace("m", "")) * HEIGHT_FOR_LEVEL;
    if ( roofShape === valueFlat ) roofShape = "gabled";
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
    roofColour = convertOSMColorStringToColor(tags["roof:colour"]);
  } else if (tags["building:colour"]) {
    roofColour = buildingColor;
  } else {
    roofColour = getRandomDefaultBuildingColor(tags["building"]);
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
    colour: buildingColor,
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
      let meshes = [];
      //Compute lateral faces
      meshes.push(serializeMesh(o.points, buildingInfo.colour, "lateral", serializeVertices, buildingInfo.minHeight, buildingInfo.maxHeight));
      o.holes && o.holes.forEach(h => {
        // const interiorFaces = extrudePoly([...h.points].reverse(), buildingInfo.minHeight, buildingInfo.maxHeight);
        // meshes.push(serializeMesh(interiorFaces, buildingInfo.colour, "lateral"));
        meshes.push(serializeMesh([...h.points].reverse(), buildingInfo.colour, "lateral", serializeVertices, buildingInfo.minHeight, buildingInfo.maxHeight));
      });

      //Compute roof faces
      const roofFaces = createRoof(o, buildingInfo.roof, convexHull, orientedMinBoundingBox);
      meshes.push(serializeMesh(roofFaces, buildingInfo.roof.colour, "roof"));
      elements.push(serializeElement(graphNode, name, meshes));
    });
  }
}

module.exports = {groupFromBuilding}
