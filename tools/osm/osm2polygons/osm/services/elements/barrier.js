const {getColorFromTags, serializePoints} = require("../nameValues");
const {serializeElement, serializeMesh} = require("../serialization");
const {graphTypeWay, tagBarrier} = require("../nameValues");

const groupFromBarrier = (elements, graphNode, name) => {
  if ( graphNode.type === graphTypeWay && graphNode.nodes.length > 0) {
    const points = graphNode.nodes.map( n => {
      return [n.spatial.deltaPosInTile[0], 0.0, n.spatial.deltaPosInTile[1]];
    });
    elements.push(serializeElement(graphNode, name, serializeMesh(points, getColorFromTags(graphNode.tags), graphNode.tags[tagBarrier], serializePoints)));
  }
}

module.exports = {
  groupFromBarrier
}
