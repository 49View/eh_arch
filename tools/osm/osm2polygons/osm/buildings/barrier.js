const {getColorFromTags} = require("../nameValues");
const {exportGroup} = require("../nodeGraph");
const {extrudePoly, getTrianglesFromPolygon} = require("../../geometry/polygon");
const {graphTypeWay} = require("../nameValues");

const barrierFromWay = (elements, way, name) => {

  // const buildingInfo = getBuildingInfo(way.tags);
  const barrierHeight = 1.9;

  way.calc && way.calc.polygons.forEach(polygon => {
    let lateralFaces = extrudePoly(polygon.points, 0, barrierHeight);
    const topFace = getTrianglesFromPolygon(polygon.points, polygon.holes, barrierHeight);
    lateralFaces = lateralFaces.concat(topFace);
    elements.push(exportGroup(way, name, lateralFaces, getColorFromTags(way.tags)));
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
