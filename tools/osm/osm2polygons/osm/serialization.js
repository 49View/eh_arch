
const facesToTriangles = faces => {
  // Triangulate faces as array instead of x,y,z
  let triangles = [];
  for ( let i = 0; i < faces.length; i+= 3) {
    let f = faces[i];
    triangles.push([f.x, f.y, f.z]);
    f = faces[i+2];
    triangles.push([f.x, f.y, f.z]);
    f = faces[i+1];
    triangles.push([f.x, f.y, f.z]);
  }
  return triangles;
}

const serializeMesh = (faces, color, name) => {
  const meshes = [{
    triangles: facesToTriangles(faces),
    colour: color,
    name: name
  }];

  return meshes;
}

const serializeElement = (elem, type, meshes) => {

  return {
    id: elem.type+"-"+elem.id,
    type: type,
    center: elem.spatial,
    meshes: meshes,
    tags: elem.tags
  }
}

module.exports = {
  serializeMesh,
  serializeElement
}
