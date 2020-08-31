import React, {useState} from "react";
import OsmScene from './components/OsmScene'
// Styles
import "./styles.css";
import {elements} from './elements.json'


const App = () => {

  const [wireframe, setWireframe] = useState(false);
  const [elementId, setElementId] = useState(elements[0].id);
  const [showAll, setShowAll] = useState(true);

  return (
    <div id="mainContainer">
      BUILDING: <select value={elementId} onChange={e => setElementId(e.target.value)}>
        {
          elements.map(b => {
            return (
              <option key={b.id} value={b.id}>{b.id}</option>
            )
          })
        }
      </select>
      <input type="checkbox" checked={wireframe} onChange={e => setWireframe(e.target.checked)}></input> WIREFRAME
      <input type="checkbox" checked={showAll} onChange={e => setShowAll(e.target.checked)}></input> SHOW ALL BUILDINGS
       {
         elementId &&
         <OsmScene
          elements={elements}
          element={elements.find(b => b.id.toString()===elementId.toString())}
          wireframe={wireframe}
          showAll={showAll}
          />
       }
    </div>
  );
}

export default App;
