
var orchval, selection;
// Called by the common.js module.
function attachListeners() {
  document.getElementById('playButton').addEventListener('click', playSound);
  document.getElementById('stopButton').addEventListener('click', stopSound);
   document.getElementById('compileButton1').addEventListener('click', compileOrc1);
    document.getElementById('compileButton2').addEventListener('click', compileOrc2);
  document.getElementById('orchestraField').addEventListener('blur',
      changeOrchestra);
  document.getElementById('orchestraField').addEventListener('select',
     selectOrchestra);
  orchval = getOrchestraElement().value;
}

// Called by the common.js module.
function moduleDidLoad() {
  common.hideModule();
}

function getOrchestraElement() {
  return document.getElementById('orchestraField');
}

function playSound() {
    console.log("play");
  common.naclModule.postMessage('playSound');
}

function stopSound() {
  common.naclModule.postMessage('stopSound');
}
function compileOrc1() {
    common.naclModule.postMessage('orchestra:' + orchval);
    console.log(orchval);
}

function compileOrc2() {
    orchval = selection;
    common.naclModule.postMessage('orchestra:' + orchval);
    console.log(orchval);
}

function changeOrchestra() 
{
    
    orchval = getOrchestraElement().value;
}

function getText(elem) {
    if(elem.tagName === "TEXTAREA" ||
       (elem.tagName === "INPUT" && elem.type === "text")) {
        return elem.value.substring(elem.selectionStart,
                                    elem.selectionEnd);
    }
    return null;
}

//function setCsoundChannel(name, value){
  // common.naclModule.postMessage('channel:'+ name ':' + value);
//}


function selectOrchestra() 
{
    selection = getText(document.activeElement);  
}

// Called by the common.js module.
function handleMessage(e) {
  //console.log(e.data)
  var element = document.getElementById('console');
  element.value += e.data;
  element.scrollTop = 99999; // focus on bottom
}
