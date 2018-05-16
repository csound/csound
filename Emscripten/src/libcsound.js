import CsoundObj from "./CsoundObj";
const libcsound = () => {
    return new Promise(function(resolve, reject) {
        CsoundObj.importScripts()
            .then(() => {
                const cs = new CsoundObj();
                resolve(cs);
            })
            .catch(e => {
                console.log(e);
                reject(e);
            });
    });
};

export default libcsound;
