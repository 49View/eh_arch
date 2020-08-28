import React from 'react'

const Light = (props) => {
    return (
        <rectAreaLight
          width={3}
          height={3}
          color={props.color}
          intensity={props.brightness}
          position={props.position}
          lookAt={[0, 0, 0]}
          penumbra={1}
          castShadow
        />
      );
}

export default Light;