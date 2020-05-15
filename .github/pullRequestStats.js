const fs = require('fs');
const moment = require('moment');
const https = require('https');
const path = require('path');

// https://api.github.com/repos/microsoft/react-native-windows/pulls?state=closed

const lastPage = 5;

let csv = 'ID, Author, Duration (h)\n';


function handleData(pageNumber, data) {
    // fs.readFileSync('pulls.json')
    const pulls = JSON.parse(data);

    map = pulls
        .filter(x => x.user.login != 'dependabot-preview[bot]' && x.merged_at != null)
        .map( (x) => ({
            id: x.number,
            author: x.user.login,
            duration: (new Date(x.merged_at) - new Date(x.created_at)) / (1000 * 3600)
        }))
        .sort((x,y) => x.duration - y.duration);

    for (const entry of map) {
        csv += `${entry.id},   ${entry.author},    ${entry.duration}\n`;
    }

    if (pageNumber == lastPage) {
        const csvFile = path.join(process.env.temp, 'pullDuration.csv');
        fs.writeFileSync(csvFile, csv);
        console.log(`Wrote ${csv.split('\n').length} entries to ${csvFile}`);
    } else {
        getData(++pageNumber);
    }
}



function getData(pageNumber) {
    let data = '';
    https.get(`https://api.github.com/repos/microsoft/react-native-windows/pulls?state=closed&per_page=100&page=${pageNumber}`,
    { headers: {'User-Agent': 'PR_Duration' } }, 
    (r) => { 
        r.on('data', (c)=> {data += c;}); 
        r.on('end', () => { handleData(pageNumber, data); }) 
        }
    );
}

getData(1);