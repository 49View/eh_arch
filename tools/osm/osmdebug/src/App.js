import React, {useState} from "react";
import OsmScene from './components/OsmScene'
// Styles
import "./styles.css";
import {buildings} from './buildings.json'


const App = () => {

  const [bldWireframe, setBldWireframe] = useState(true);
  const [roofWireframe, setRoofWireframe] = useState(true);
  const [buildingId, setBuildingId] = useState(buildings[0].id);
  const [showAll, setShowAll] = useState(false);

  return (
    <div id="mainContainer">
      BUILDING: <select value={buildingId} onChange={e => setBuildingId(e.target.value)}>
        {
          buildings.map(b => {
            return (
              <option key={b.id} value={b.id}>{b.id}</option>
            )
          })
        }
      </select>
      <input type="checkbox" checked={bldWireframe} onChange={e => setBldWireframe(e.target.checked)}></input> FACES WIREFRAME
      <input type="checkbox" checked={roofWireframe} onChange={e => setRoofWireframe(e.target.checked)}></input> ROOF WIREFRAME
      <input type="checkbox" checked={showAll} onChange={e => setShowAll(e.target.checked)}></input> SHOW ALL BUILDINGS
       { 
         buildingId && 
         <OsmScene
          buildings={buildings} 
          building={buildings.find(b => b.id.toString()===buildingId.toString())} 
          buildingWireframe={bldWireframe} 
          roofWireframe={roofWireframe}
          showAll={showAll}
          /> 
       }     
    </div>
  );
}

export default App;
