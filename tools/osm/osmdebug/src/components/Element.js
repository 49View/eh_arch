import React, {Fragment} from "react";
import * as THREE from "three";
import Triangles from "./Triangles";


const Element = (props) => {

  return (
    <Fragment>
        {
          props.element.groups.map(g => (
            <Triangles
              position={props.position}
              vertices={g.triangles.map(p => new THREE.Vector3(p[0],p[2],-p[1]))}
              wireframe={props.wireframe} color={props.wireframe?'#000000':g.colour}/>
          ))
        }
    </Fragment>
  );
}

export default Element;
