import React from "react";
import { Canvas } from "react-three-fiber";
import GridHelper from "./GridHelper";
import Controls from "./Controls";
import Building from "./Building";


const OsmScene = (props) => {

  let Content;
  if (props.showAll) {

    Content = props.buildings.map(b => <Building
      position={[b.center.x-props.building.center.x,0,props.building.center.y-b.center.y]}
      building={b}
      buildingWireframe={props.buildingWireframe}
      roofWireframe={props.roofWireframe}
    />
    );
  } else {
    Content = (
      <Building 
        position={[0,0,0]}
        building={props.building} 
        buildingWireframe={props.buildingWireframe}
        roofWireframe={props.roofWireframe}
      />
    );
  }

  return (
    <div style={{height: "calc(100vh - 80px)"}}>
        <Canvas
        pixelRatio={window.devicePixelRatio}
        camera={{ position: [0, 50, 50] }}
        onCreated={({ gl }) => gl.setClearColor("#f0f0f0")}
        >
        
          {Content}
          <GridHelper />
          <Controls />
        </Canvas>
    </div>
  );
}

export default OsmScene;
