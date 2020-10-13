
const serializeTriangles = 0;
const serializePoints = 1;

const graphTypeWay  = "way";
const graphTypeRel  = "relation";
const graphTypeNode = "node";

const roleOuter = "outer";
const roleInner = "inner";

// Node typology
const tagBarrier = "barrier";

const valueFence = "fence";

// Roof
const valuePyramidal = "pyramidal";
const valueGabled = "gabled";
const valueHipped = "hipped";
const valueDone = "dome";
const valueFlat = "flat";
const valueAlong = "along";

// Buildings
const valueAirShaft = "air_shaft";


const getColorFromTags = (tags) => {
  let color = "#808080";

  if (tags) {
    if (tags["natural"] === "water") {
      color = "#AAD3DF";
    } else if ((tags["landuse"] && tags["landuse"] === "grass") || (tags["leisure"] && (tags["leisure"] === "park" || tags["leisure"] === "garden"))) {
      color = "#CDF7C9";
    } else if (tags["highway"]) {
      switch (tags["highway"]) {
        case "motorway":
          color = "#DF2E6B";
          break;
        case "motorway_link":
          color = "#DF2E6B";
          break;
        case "trunk":
          color = "#FBB29A";
          break;
        case "trunk_link":
          color = "#FBB29A";
          break;
        case "primary":
          color = "#FDD7A1";
          break;
        case "primary_link":
          color = "#FDD7A1";
          break;
        case "secondary":
          color = "#F1EEE8";
          break;
        case "tertiary":
          color = "#FFDFFF";
          break;
        case "residential":
          color = "#FFDFDF";
          break;
        case "unclassified":
        case "track":
        case "cycleway":
        case "bridleway":
        case "footway":
        case "path":
        case "steps":
          color = "#8F8F8F";
          break;
        default:
          color = "#CFCFCF";
      }
    } else if (tags["railway"] && tags["railway"] === "rail") {
      color = "#303030";
    } else if (tags[tagBarrier]) {
      switch (tags[tagBarrier]) {
        case valueFence:
          color = "#22222280"
          break;
        default:
          color = "#DBB29A";
      }
    }
  } else {
    color = "#606060";
  }

  return color;
}

const getWidthFromWay = (way) => {
  let roadLane = 1;
  let roadWidth = 2;

  if (way.tags) {
    switch (way.tags["highway"]) {
      case "motorway":
        roadWidth = 2;
        roadLane = 3;
        break;
      case "motorway_link":
        roadWidth = 2;
        roadLane = 2;
        break;
      case "trunk":
      case "trunk_link":
        roadWidth = 1.8;
        roadLane = 2;
        break;
      case "primary":
      case "primary_link":
        roadWidth = 1.5;
        roadLane = 2;
        break;
      case "secondary":
        roadWidth = 0.8;
        roadLane = 2;
        break;
      case "tertiary":
        roadWidth = 1;
        roadLane = 2;
        break;
      case "unclassified":
      case "residential":
        roadWidth = 0.8;
        roadLane = 1;
        break;
      case "track":
      case "cycleway":
      case "bridleway":
      case "footway":
      case "path":
      case "steps":
        roadWidth = 0.5;
        roadLane = 1;
        break;
      default:
        roadWidth = 0.8;
        roadLane = 1;
    }
    if (way.tags["railway"] && way.tags["railway"] === "rail") {
      roadWidth = 0.5;
      roadLane = 2;
    }
    if (way.tags[tagBarrier]) {
      switch (way.tags[tagBarrier] ) {
        default:
          roadWidth = 0.3;
          roadLane = 1;
      }
    }
  }

  return {roadWidth, roadLane};
}

module.exports = {
  serializePoints, serializeTriangles,
  graphTypeWay, graphTypeRel, graphTypeNode,
  roleInner, roleOuter,
  getColorFromTags, getWidthFromWay,
  valueAirShaft,
  tagBarrier,
  valueAlong, valueDone, valueGabled, valueHipped, valuePyramidal, valueFlat, valueFence
}
