const {exportGroup} = require("../nodeGraph");
const {getTrianglesFromPolygon} = require("../../geometry/polygon");
const {graphTypeWay, graphTypeRel, graphTypeNode, getColorFromTags} = require("../nameValues");

const groupFromGraphNode = (elements, graphNode, name) => {

  if ( graphNode.type === graphTypeWay || graphNode.type === graphTypeRel ) {
    graphNode.calc && graphNode.calc.polygons && graphNode.calc.polygons.forEach(o => {
      const faces = getTrianglesFromPolygon(o.points, o.holes,0);
      const tags = {...graphNode.tags, ...o.tags};
      elements.push(exportGroup(graphNode, name, faces, getColorFromTags(tags)));
    });
  } else if ( graphNode.type === graphTypeNode ) {
    elements.push(exportGroup(graphNode, name, [], getColorFromTags(graphNode.tags)));
  }
}

module.exports = {
  groupFromGraphNode
}
