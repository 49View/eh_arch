const {
    getPolygonsFromMultipolygonRelation,
    getPolygonFromWay,
    checkPolygonInsidePolygon
} = require("./osmHelper.js");

RAD2DEG = 180 / Math.PI;
PI_4 = Math.PI / 4;

const lat2y = lat => { 
    return Math.log(Math.tan((lat / 90 + 1) * PI_4 )) * RAD2DEG; 
}
// lon2x is basically lon because the mercator is a cilyndrical projection so longitude doesn't change it's ratio
//const lon2x = lon => { return lon; }
const calcCoordinate = (nodes) => {
    // const latCenter = bbox[0]+(bbox[2]-bbox[0])/2;
    // const lonCenter = bbox[1]+(bbox[3]-bbox[1])/2;

    nodes.forEach(n => {
        // n.x = calcDistance(bbox[0],bbox[1],bbox[0],n.lon);
        // n.y = calcDistance(bbox[0],bbox[1],n.lat,bbox[1]);
        // n.x = new Decimal(calcDistance(0,0,0,n.lon)*Math.sign(n.lon));
        // n.y = new Decimal(calcDistance(0,0,n.lat,0)*Math.sign(n.lat));
        // n.x=new Decimal(Math.sign(n.lon)).mul(calcDistance(0,0,0,n.lon));
        // n.y=new Decimal(Math.sign(n.lat)).mul(calcDistance(0,0,lat2y(n.lat),0));
        n.x=Math.sign(n.lon)*calcDistance(0,0,0,n.lon);
        n.y=Math.sign(n.lat)*calcDistance(0,0,lat2y(n.lat),0);
    })
}

const calcDistance = (latitude1,longitude1,latitude2,longitude2) => {

    // const toRadians = new Decimal(Math.PI).div(new Decimal(180));
    // const lat1 = new Decimal(latitude1);
    // const lon1 = new Decimal(longitude1);
    // const lat2 = new Decimal(latitude2);
    // const lon2 = new Decimal(longitude2);
    // const R = new Decimal(6372.797e3); // metres

    // const phi1 = lat1.mul(toRadians); // φ, λ in radians
    // const phi2 = lat2.mul(toRadians);
    // const deltaPhi = (lat2.sub(lat1)).mul(toRadians);
    // const deltaLambda = (lon2.sub(lon1)).mul(toRadians);

    // let a = Decimal.sin(deltaPhi.div(2)).mul(Decimal.sin(deltaPhi.div(2)))
    //     .add(Decimal.cos(phi1).mul(Decimal.cos(phi2)).mul(Decimal.sin(deltaLambda.div(2))).mul(Decimal.sin(deltaLambda.div(2))))

    // const c = Decimal.atan2(Decimal.sqrt(a),Decimal.sqrt(new Decimal(1).sub(a))).mul(2);

    // const d = R.mul(c); // in metres

    const lat1 = latitude1;
    const lon1 = longitude1;
    const lat2 = latitude2;
    const lon2 = longitude2;
    const R = 6371e3; // metres
    const phi1 = lat1 * Math.PI/180; // φ, λ in radians
    const phi2 = lat2 * Math.PI/180;
    const deltaPhi = (lat2-lat1) * Math.PI/180;
    const deltaLambda = (lon2-lon1) * Math.PI/180;

    const a = Math.sin(deltaPhi/2) * Math.sin(deltaPhi/2) +
              Math.cos(phi1) * Math.cos(phi2) *
              Math.sin(deltaLambda/2) * Math.sin(deltaLambda/2);
    const c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1-a));

    const d = (R * c); // in metres


    return d;
}

const findElement = (list,id) => {
    return list.find(e => e.id===id);
}

const extendData = (nodes,ways,relations) => {

    nodes.forEach(n => {
        n.inWay=false;
        n.inRelation=false;
    })
    ways.forEach(w => {
        w.inRelation=false;
        const wayNodes=[];
        w.nodes.forEach(n => {
            const node=findElement(nodes,n);
            if (node) {
                node.inWay=true;
                wayNodes.push(node);
            }
        })
        w.nodes=wayNodes;
    });

    relations.forEach(r => {
        const relationMembers=[];
        r.members.forEach(m => {
            const member = {
                type: m.type,
                role: m.role
            }
            if (m.type==="way") {
                const way=findElement(ways,m.ref);
                if (way) {
                    way.inRelation=true;
                    member.ref=way;
                }
            } else if (m.type==="node") {
                const node=findElement(nodes,m.ref);
                if (node) {
                    node.inRelation=true;
                    member.ref=node;
                }
            }
            if (member.ref) {
                relationMembers.push(member);
            }
        });
        r.members=relationMembers;
    })
}

const elaborateData = (nodes,ways,rels) => {
    //Calculate coordinate as distance from bbox center
    calcCoordinate(nodes);
    //Transform reference in ways and relations to data
    extendData(nodes,ways,rels);    
    //Compute ways and rels
    computePolygons(ways,rels);

    createPolygonsHierarchy(ways, rels);
}

const computePolygons = (ways, rels) => {

    ways
        .filter(w => !w.inRelation && w.nodes.length>2 && w.nodes[0].id===w.nodes[w.nodes.length-1].id)
        .forEach(w => {
            try {
                const {polygon,localBoundingBox,convexHull,orientedMinBoundingBox} = getPolygonFromWay(w);
                const absolutePolygon=[];
                polygon.forEach(p => {
                    absolutePolygon.push({x: p.x+localBoundingBox.centerX, y:p.y+localBoundingBox.centerY});
                })
                w.calc = {
                    polygon: polygon,
                    absolutePolygon: absolutePolygon,
                    lbb: localBoundingBox,
                    convexHull: convexHull,
                    ombb:orientedMinBoundingBox
                };
            } catch (ex) {
                console.log(`Error in ${w.id} way`, ex);
            }
        }
    );

    rels
        .forEach(r => {
            try {
                const {polygons,localBoundingBox} = getPolygonsFromMultipolygonRelation(r);
                r.calc = {
                    polygons: polygons,
                    lbb: localBoundingBox,
                };
            } catch (ex) {
                console.log(`Error in ${r.id} relation`, ex);
            }
        }
    );

}

const createPolygonsHierarchy = (ways, rels) => {

    ways
        .filter(w => w.calc!==undefined)
        .forEach(wo => {
            wo.children=[];
            ways
                .filter(w => w.calc!==undefined)
                .filter(w => w.id !== wo.id)
                .filter(w => !w.owner || (w.containers && w.containers.findIndex(c => c.id==="w-"+wo.id)))
                .forEach(wi => {
                    
                    if (checkPolygonInsidePolygon(wo.calc.absolutePolygon, wi.calc.absolutePolygon)) {
                        if (wi.containers===undefined) {
                            wi.containers=[];
                        };
                        wi.containers.push(wo);
                        wo.children.push(wi);
                    }
                })
        }
    );

    ways
        .filter(w => w.calc!==undefined)
        .forEach(w => {
            console.log(`Way ${w.id} has ${w.children.length} children`);
        }
    );

}

module.exports = {
    elaborateData
}