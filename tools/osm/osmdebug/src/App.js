import React, {useState} from "react";
import OsmScene from './components/OsmScene'
// Styles
import "./styles.css";
import {buildings} from './buildings.json'


const App = () => {

  const [bldWireframe, setBldWireframe] = useState(true);
  const [roofWireframe, setRoofWireframe] = useState(true);
  const [buildingId, setBuildingId] = useState(buildings[0].id);

  return (
    <div id="mainContainer">
      BUILDING: <select value={buildingId} onChange={e => {
        setBuildingId(e.target.value);
        console.log(buildings.find(b => b.id.toString()===e.target.value.toString()))
      }}>
        {
          buildings.map(b => {
            return (
              <option key={b.id} value={b.id}>{b.tags.name || b.id}</option>
            )
          })
        }
      </select>
      <input type="checkbox" checked={bldWireframe} onChange={e => setBldWireframe(e.target.checked)}></input> FACES WIREFRAME
      <input type="checkbox" checked={roofWireframe} onChange={e => setRoofWireframe(e.target.checked)}></input> ROOF WIREFRAME
       { 
         buildingId && 
         <OsmScene building={buildings.find(b => b.id.toString()===buildingId.toString())} buildingWireframe={bldWireframe} roofWireframe={roofWireframe}/> 
       }     
    </div>
  );
}

export default App;
