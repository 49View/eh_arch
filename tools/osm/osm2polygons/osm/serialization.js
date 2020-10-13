const {serializeTriangles} = require("./nameValues");

const facesToTriangles = faces => {
  // Triangulate faces as array instead of x,y,z
  let triangles = [];
  for (let i = 0; i < faces.length; i += 3) {
    let f = faces[i];
    triangles.push([f.x, f.y, f.z]);
    f = faces[i + 2];
    triangles.push([f.x, f.y, f.z]);
    f = faces[i + 1];
    triangles.push([f.x, f.y, f.z]);
  }
  return triangles;
}

const serializeMesh = (faces, color, part, vertexType = serializeTriangles) => {
  return {
    vertices: vertexType === serializeTriangles ? facesToTriangles(faces) : faces,
    colour: color,
    part: part,
    vertexType: vertexType
  };
}

const serializeTags = tags => {
  if (!tags) return [];
  let ret = [];
  for (const [key, value] of Object.entries(tags)) {
    ret.push({key, value});
  }
  return ret;
}

const serializeElement = (elem, type, meshes) => {

  const meshArray = Array.isArray(meshes) ? meshes : [meshes];

  const sTags = serializeTags(elem.tags);

  return {
    id: elem.type + "-" + elem.id,
    type: type,
    center: elem.spatial,
    tags: sTags,
    meshes: meshArray
  }
}

module.exports = {
  serializeMesh,
  serializeElement
}
