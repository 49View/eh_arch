const {serializeElement} = require("../serialization");
const {getTrianglesFromPolygon} = require("../../geometry/polygon");
const {graphTypeWay, graphTypeRel, graphTypeNode, getColorFromTags} = require("../nameValues");

const groupFromGraphNode = (elements, graphNode, name) => {

  if ( graphNode.type === graphTypeWay || graphNode.type === graphTypeRel ) {
    graphNode.calc && graphNode.calc.polygons && graphNode.calc.polygons.forEach(o => {
      const faces = getTrianglesFromPolygon(o.points, o.holes,0);
      const tags = {...graphNode.tags, ...o.tags};
      elements.push(serializeElement(graphNode, name, faces, getColorFromTags(tags)));
    });
  } else if ( graphNode.type === graphTypeNode ) {
    elements.push(serializeElement(graphNode, name, [], getColorFromTags(graphNode.tags)));
  }
}

module.exports = {
  groupFromGraphNode
}
