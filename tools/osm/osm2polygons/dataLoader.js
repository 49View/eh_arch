const axios = require('axios')
const qs = require('qs');
const fs = require('fs');

const OVERPASSAPI_URL="http://overpass-api.de/api/interpreter";

const getData = async (bbox) => {
    
    const query=`
    [out:json][timeout:25];
    (
        rel["building"](${bbox.join(",")});
        way["building"](${bbox.join(",")});
        way["building:part"](${bbox.join(",")});
    );
    (
        ._;
        >;
    );
    out;
    `;

    const options = {
        method: "POST",
        headers: { 'content-type': 'application/x-www-form-urlencoded' },
        data: qs.stringify({data:query})
    };
    //console.log(qs.stringify({data: query}));
    const response=await axios(OVERPASSAPI_URL, options);
    osmData=response.data;
    return prepareData(bbox,osmData);
}

const getDataLocal = (bbox) => {
    const osmData = JSON.parse(fs.readFileSync("testData.json", { encoding: "utf8"}));
    return prepareData(bbox,osmData);
}

const prepareData = (bbox,osmData) => {
    //Group loaded element for type
    const {nodes,ways,rels} = parseData(osmData);
    //Calculate coordinate as distance from bbox center
    calcCoordinate(bbox,nodes);
    //Transform reference in ways and relations to data
    extendData(nodes,ways,rels);

    return {nodes,ways,rels}
}

const calcCoordinate = (bbox, nodes) => {
    const latCenter = bbox[0]+(bbox[2]-bbox[0])/2;
    const lonCenter = bbox[1]+(bbox[3]-bbox[1])/2;

    nodes.forEach(n => {
        // n.x = calcDistance(bbox[0],bbox[1],bbox[0],n.lon);
        // n.y = calcDistance(bbox[0],bbox[1],n.lat,bbox[1]);
        n.x = calcDistance(0,0,0,n.lon);
        n.y = calcDistance(0,0,n.lat,0);
    })
}

const calcDistance = (lat1,lon1,lat2,lon2) => {
    const R = 6371e3; // metres
    const phi1 = lat1 * Math.PI/180; // φ, λ in radians
    const phi2 = lat2 * Math.PI/180;
    const deltaPhi = (lat2-lat1) * Math.PI/180;
    const deltaLambda = (lon2-lon1) * Math.PI/180;
    
    const a = Math.sin(deltaPhi/2) * Math.sin(deltaPhi/2) +
              Math.cos(phi1) * Math.cos(phi2) *
              Math.sin(deltaLambda/2) * Math.sin(deltaLambda/2);
    const c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1-a));
    
    const d = R * c; // in metres   
    
    return d;
}

const parseData = (osmData) => {
    const elements = osmData.elements;
    const nodes = [];
    const ways = [];
    const rels = [];

    elements.forEach((e,i) => {
        if (e.type==="node") nodes.push(e)
        else if (e.type==="way") ways.push(e)
        else if (e.type==="relation") rels.push(e)
        else console.log(`Element ${i} unknown`);
    });

    return {nodes,ways,rels}
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


module.exports = {getData,getDataLocal}

