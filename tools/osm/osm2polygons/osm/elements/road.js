const {serializeElement, serializeMesh} = require("../serialization");
const {getTrianglesFromPolygon} = require("../../geometry/polygon");
const {graphTypeWay, graphTypeRel, graphTypeNode, getColorFromTags} = require("../nameValues");

const groupFromGraphNode = (elements, graphNode, type) => {

  if ( graphNode.type === graphTypeWay || graphNode.type === graphTypeRel ) {
    graphNode.calc && graphNode.calc.polygons && graphNode.calc.polygons.forEach(o => {
      const faces = getTrianglesFromPolygon(o.points, o.holes,0);
      const tags = {...graphNode.tags, ...o.tags};
      elements.push(serializeElement(graphNode, type, serializeMesh(faces, getColorFromTags(tags))));
    });
  } else if ( graphNode.type === graphTypeNode ) {
    elements.push(serializeElement(graphNode, type, serializeMesh([], getColorFromTags(graphNode.tags))));
  }
}

module.exports = {
  groupFromGraphNode
}
