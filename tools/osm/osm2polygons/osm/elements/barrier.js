const {getColorFromTags} = require("../nameValues");
const {serializeElement, serializeMesh} = require("../serialization");
const {extrudePoly, getTrianglesFromPolygon} = require("../../geometry/polygon");
const {graphTypeWay, tagBarrier} = require("../nameValues");

const barrierFromWay = (elements, way, name) => {

  // const buildingInfo = getBuildingInfo(way.tags);
  const barrierHeight = 1.9;

  way.calc && way.calc.polygons.forEach(polygon => {
    let lateralFaces = extrudePoly(polygon.points, 0, barrierHeight);
    const topFace = getTrianglesFromPolygon(polygon.points, polygon.holes, barrierHeight);
    lateralFaces = lateralFaces.concat(topFace);
    elements.push(serializeElement(way, name, serializeMesh(lateralFaces, getColorFromTags(way.tags), way.tags[tagBarrier])));
  });
}

const groupFromBarrier = (elements, graphNode, name) => {
  if ( graphNode.type === graphTypeWay ) {
    barrierFromWay( elements, graphNode, name );
  }
}

module.exports = {
  groupFromBarrier
}
