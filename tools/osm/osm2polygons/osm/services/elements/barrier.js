const {getColorFromTags, serializePoints} = require("../nameValues");
const {serializeElement, serializeMesh} = require("../serialization");
const {graphTypeWay, tagBarrier} = require("../nameValues");

const groupFromBarrier = (elements, graphNode, name) => {
  if ( graphNode.type === graphTypeWay && graphNode.nodes.length > 0) {
    const points = graphNode.nodes.map( n => {
      return [n.spatial.tilePos[0], 0.0, n.spatial.tilePos[1]];
    });
    elements.push(serializeElement(graphNode, name, serializeMesh(points, getColorFromTags(graphNode.tags), graphNode.tags[tagBarrier], serializePoints)));
  }
}

module.exports = {
  groupFromBarrier
}
