const {serializeElement, serializeMesh} = require("../serialization");
const {getTrianglesFromPolygon} = require("../../../geometry/polygon");
const {graphTypeWay, graphTypeRel, graphTypeNode, getColorFromTags} = require("../nameValues");

const groupFromGraphNode = (elements, graphNode, type) => {

  const addNode = (node, faces = [], tags = null) => {
    return {
      node: node,
      faces: faces,
      tags: tags === null ? node.tags : tags
    }
  }

  let elemsToAdd = [];
  if (graphNode.type === graphTypeWay || graphNode.type === graphTypeRel) {
    graphNode.calc && graphNode.calc.polygons && graphNode.calc.polygons.forEach(o => {
      elemsToAdd.push(addNode(graphNode, getTrianglesFromPolygon(o.points, o.holes, 0), {...graphNode.tags, ...o.tags}));
    });
  } else if (graphNode.type === graphTypeNode) {
    elemsToAdd.push(addNode(graphNode));
  }

  elemsToAdd.forEach(e => elements.push(serializeElement(e.node, type, serializeMesh(e.faces, getColorFromTags(e.tags)), e.tags["highway"])));
}

module.exports = {
  groupFromGraphNode
}
