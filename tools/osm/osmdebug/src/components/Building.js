import React, {Fragment} from "react";
import * as THREE from "three";
import Triangles from "./Triangles";


const Building = (props) => {

    const building = props.building;
    const buildingFaces = building.lateralFaces.faces.map(p => new THREE.Vector3(p.x,p.z,-p.y));
    const buildingColor = props.buildingWireframe?'#000000':building.lateralFaces.colour;
    const roofFaces = building.roofFaces.faces.map(p => new THREE.Vector3(p.x,p.z,-p.y));
    const roofColor = props.roofWireframe?'#000000':building.roofFaces.colour;

  return (
    <Fragment>
        <Triangles position={props.position} vertices={buildingFaces} wireframe={props.buildingWireframe} color={buildingColor}/>
        <Triangles position={props.position} vertices={roofFaces} wireframe={props.roofWireframe} color={roofColor}/>
    </Fragment>
  );
}

export default Building;
