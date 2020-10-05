import React from "react";
import { Canvas } from "react-three-fiber";
import Controls from "./Controls";
import Element from "./Element";


const OsmScene = (props) => {

  let Content;
  if (props.showAll) {

    Content = props.elements.map(b =>
      <Element
        position={[b.center.x-props.element.center.x,0,b.center.y-props.element.center.y]}
        element={b}
        wireframe={props.wireframe}
      />
    );
  } else {
    Content = (
      <Element
        position={[0,0,0]}
        element={props.element}
        wireframe={props.wireframe}
      />
    );
  }

  return (
    <div style={{height: "calc(100vh - 80px)"}}>
        <Canvas
        pixelRatio={window.devicePixelRatio}
        camera={{ position: [0, 50, 50], fov: 45.0, far:10000.0}}
        onCreated={({ gl }) => gl.setClearColor("#f0f0f0")}
        >
          {/* <ambientLight />*/}
          <pointLight position={[1000, 1000, 1000]} castShadow={true} />
          <pointLight position={[-1000, -100, -1000]} castShadow={true} />
          {Content}
          {/* <GridHelper /> */}
          <Controls />
        </Canvas>
    </div>
  );
}

export default OsmScene;
