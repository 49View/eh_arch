import React, { useMemo } from "react";
import * as THREE from "three";

const Triangles = (props) => {
  const f32array = useMemo(
    () =>
      Float32Array.from(
        new Array(props.vertices.length)
          .fill()
          .flatMap((item, index) => props.vertices[index].toArray())
      ),
    [props.vertices]
  );

  return (
    <mesh position={props.position} castShadow>
      <bufferGeometry attach="geometry">
        <bufferAttribute
          attachObject={["attributes", "position"]}
          args={[f32array, 3]}
        />
      </bufferGeometry>
      {
        !props.wireframe ? <meshStandardMaterial 
          attach="material"
          color={props.color}
          shading={THREE.FlatShading}
          side={THREE.DoubleSide}
          castShadow
        />
        : <meshBasicMaterial 
        attach="material"
        color={props.color}
        wireframe={props.wireframe}
        side={THREE.DoubleSide}
        />
      }
    </mesh>
  );
};

export default Triangles;
