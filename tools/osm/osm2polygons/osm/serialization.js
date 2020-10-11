
const serializeElement = (elem, type, faces, color) => {

  const meshes = [];

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

  meshes.push({
    triangles: triangles,
    colour: color,
  });

  return {
    id: elem.type+"-"+elem.id,
    type: type,
    center: elem.spatial,
    meshes: meshes,
    tags: elem.tags
  }
}

module.exports = {
  serializeElement
}
