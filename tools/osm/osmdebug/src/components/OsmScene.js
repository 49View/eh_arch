import React from "react";
import { Canvas } from "react-three-fiber";
import * as THREE from "three";

// Components
import GridHelper from "./GridHelper";
import Controls from "./Controls";
import Triangles from "./Triangles";


const OsmScene = (props) => {

    const building = props.building;
    const buildingFaces = building.lateralFaces.faces.map(p => new THREE.Vector3(p.x,p.z,-p.y));
    const buildingColor = building.lateralFaces.colour;
    const roofFaces = building.roofFaces.faces.map(p => new THREE.Vector3(p.x,p.z,-p.y));
    const roofColor = building.roofFaces.colour;
//   const vertices = [
//     new THREE.Vector3(0, 20, 0),
//     new THREE.Vector3(0, 0, 0),
//     new THREE.Vector3(20, 0, 0),
//     new THREE.Vector3(0, 20, 0),
//     new THREE.Vector3(20, 0, 0),
//     new THREE.Vector3(20, 20, 0)  
// ];

  return (
    <div style={{height: "calc(100vh - 80px)"}}>
        <Canvas
        pixelRatio={window.devicePixelRatio}
        camera={{ position: [0, 50, 50] }}
        onCreated={({ gl }) => gl.setClearColor("#f0f0f0")}
        >
        <Triangles vertices={buildingFaces} wireframe={props.buildingWireframe} color={buildingColor}/>
        <Triangles vertices={roofFaces} wireframe={props.roofWireframe} color={roofColor}/>
        <GridHelper />
        <Controls />
        </Canvas>
    </div>
  );
}

export default OsmScene;
