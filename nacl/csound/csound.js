/*
 * Csound pnacl interactive frontend
 *
 * Copyright (C) 2013 V Lazzarini
 *
 * This file belongs to Csound.
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


var csound = (function() {

  /**
   * Loads csound pNaCl module
   */
  function createModule() {
    var model = document.createElement('embed');
    model.setAttribute('name', 'csound_module');
    model.setAttribute('id', 'csound_module');
    model.setAttribute('path', 'pnacl/Release');
    model.setAttribute('src', 'pnacl/Release/csound.nmf');
    var mimetype = 'application/x-pnacl';
    model.setAttribute('type', mimetype);
    var csoundhook = document.getElementById('engine');
    csoundhook.appendChild(model);
  }
  
  /**
   * Attaches handlers for events
   */
  function attachDefaultListeners() {
    var csoundhook= document.getElementById('engine');
    csoundhook.addEventListener('load', moduleDidLoad, true);
    csoundhook.addEventListener('message', handleMessage, true);
    csoundhook.addEventListener('crash', handleCrash, true);
    csoundhook.addEventListener('progress', handleProgress, true);
    if (typeof window.attachListeners !== 'undefined') {
      window.attachListeners();
    }
  }

  /**
   * Creates a progress count
   */
  var progressCount=0;
  function handleProgress(event) {
    var loadPercent = 0.0;
    var loadPercentString;
    if (event.lengthComputable && event.total > 0) {
      loadPercent = (event.loaded / event.total) * 100.0;
      loadPercentString = 'loading... (' + loadPercent.toFixed(2) + '%)';
    } else {
       loadPercent = -1.0;
       progressCount++;
       loadPercentString = 'loading... (count=' + progressCount + ')';
    }
    updateStatus(loadPercentString);
  }

  /**
   * Handles a crash or exit event
   */
  function handleCrash(event) {
    if (csound.module.exitStatus == -1) {
      updateStatus('Ops, something went wrong... please refresh page',1);
    } else {
	updateStatus('Csound has exited [' + csound.module.exitStatus + ']', 1);
    }
    if (typeof window.handleCrash !== 'undefined') {
      window.handleCrash(csound.module.lastError);
    }
  }

  /**
   *  After module loading, point the member var to it
   */
   function moduleDidLoad() {
    csound.module = document.getElementById('csound_module');
       updateStatus('ready', 1);
    if (typeof window.moduleDidLoad !== 'undefined') {
      window.moduleDidLoad();
    }
  }

  /**
   * Unloads/destroys module.
   */
  function destroyModule() {
    csound.module.parentNode.removeChild(csound.module);
    csound.module = null;
  }

  var fileData = null;
  function handleFileMessage(event) {
      updateStatus("fetching file data\n");
      fileData = event.data;
      updateStatus("finished fetching file data\n");
        var csoundhook= document.getElementById('engine');
        csoundhook.removeEventListener('message', handleFileMessage, true);
        csoundhook.addEventListener('message', handleMessage, true);
   }

  var tableData = null;
  function handleTableMessage(event) {
      updateStatus("fetching table data\n");
      tableData = event.data;
      updateStatus("finished fetching table data\n");
        var csoundhook= document.getElementById('engine');
        csoundhook.removeEventListener('message', handleTableMessage, true);
        csoundhook.addEventListener('message', handleMessage, true);
   }



  /**
   * handles messages by passing them to the window handler
   *
   * @param {Event} event A message event. 
   *                message_event.data contains
   *                the data sent from the csound module.
   */
  function handleMessage(event) {
    if (typeof window.handleMessage !== 'undefined') {
	if(event.data == "Reading:"){
          var csoundhook= document.getElementById('engine');
          csoundhook.removeEventListener('message', handleMessage, true);
          csoundhook.addEventListener('message', handleFileMessage, true);
	}
        else if(event.data == "ReadingTable:"){
          var csoundhook= document.getElementById('engine');
          csoundhook.removeEventListener('message', handleMessage, true);
          csoundhook.addEventListener('message', handleTableMessage, true);
	}
       else 
       window.handleMessage(event);
      return;
    }
  }

  var statusText = 'not loaded';
  /**
   * Prints current status to the console
   * @param {string} opt_message The status message.
   */
    function updateStatus(opt_message, keep) {
    if (opt_message) {
      statusText = 'Csound: ' + opt_message + '\n';
    }
    var statusField = document.getElementById('console');
    if (statusField) {
        var fieldType = statusField.tagName.toLowerCase();
        if(fieldType == 'div') {
        statusText += " <br>";
        if(!keep) statusField.innerHTML = statusText ;
            else 
             statusField.innerHTML += statusText;
	} else {
	if(!keep) statusField.value = statusText;
        else statusField.value += statusText;
	}
    }
  }

  /**
   * Starts audio playback
   */
  function Play() {
    csound.module.postMessage('playCsound');
  } 

  /**
   * Pauses audio playback
   */
  function Pause() {
   csound.module.postMessage('pauseCsound');
  }
  
  /**
   * Stops rendering and resets Csound.
   */
  function Stop() {
      destroyModule();
      createModule();
  }

  /**
   * Sends code to be compiled by Csound
   *
   * @param {string} s A string containing the code.
   */
  function CompileOrc(s) {
   csound.module.postMessage('orchestra:' + s);
  }

  /**
   * Starts audio playback with a CSD
   *
   * @param {string} s A string containing the pathname to the CSD.
   */
  function PlayCsd(s) {
   csound.module.postMessage('csd:' + s);
  }

  /**
   * Starts audio rendering with a CSD (no RT audio playback)
   *
   * @param {string} s A string containing the pathname to the CSD.
   */
  function RenderCsd(s) {
   csound.module.postMessage('render:' + s);
  }


  /**
   * Sends a score to be read by Csound
   *
   * @param {string} s A string containing the score.
   */
  function ReadScore(s) {
    csound.module.postMessage('score:' + s);
   }

  /**
   * Sends line events to Csound
   *
   * @param {string} s A string containing the line events.
   */
  function Event(s) {
    csound.module.postMessage('event:' + s);
   }

  /**
   * Sets the value of a control channel in Csound
   *
   * @param {string} name The channel to be set.
   * @param {number} value The value to set the channel.
   */
  function SetChannel(name, value){
    var channel = 'channel:' + name + ':';
    csound.module.postMessage(channel + value);
   }


  /**
   * Sends in a MIDI message to Csound MIDI system
   *
   * @param {number} byte1 first midi byte (128-255)
   * @param {number} byte2 second midi byte (0-127)
   * @param {number} byte3 third midi byte (0-127)
   */
    function MIDIin(byte1, byte2, byte3){
        if(byte1 < 128 || byte1 > 255) return;
        if(byte2 < 0 || byte2 > 127) return;
	if(byte3 < 0 || byte3 > 127) return;
	var mess1 = 'midi:' + byte1;
	var mess2 = ':' + byte2;
	var mess3 = ':' + byte3;
        csound.module.postMessage(mess1+mess2+mess3);
    }
    
   /**
   * Sends in a MIDI NOTEOFF message to Csound MIDI system
   *
   * @param {number} channel MIDI channel (1-16)
   * @param {number} number MIDI note (0-127)
   * @param {number} velocity MIDI velocity (0-127)
   */ 
    function NoteOff(channel,number,velocity){
       if(channel > 0 && channel < 17)
	csound.MIDIin(127+channel,number,velocity);
    }

  /**
   * Sends in a MIDI NOTEON message to Csound MIDI system
   *
   * @param {number} channel MIDI channel (1-16)
   * @param {number} number MIDI note (0-127)
   * @param {number} velocity MIDI velocity (0-127)
   */
    function NoteOn(channel,number,velocity){
      if(channel > 0 && channel < 17)
	csound.MIDIin(143+channel,number,velocity);
    }

   /**
   * Sends in a MIDI POLYAFT message to Csound MIDI system
   *
   * @param {number} channel MIDI channel (1-16)
   * @param {number} number MIDI note (0-127)
   * @param {number} aftertouch MIDI aftertouch (0-127)
   */ 
    function PolyAftertouch(channel,number,aftertouch){
       if(channel > 0 && channel < 17)
	csound.MIDIin(160+channel,number,aftertouch);
    }
   
   /**
   * Sends in a MIDI CONTROLCHANGE message to Csound MIDI system
   *
   * @param {number} channel MIDI channel (1-16)
   * @param {number} control MIDI cc number (0-127)
   * @param {number} amount  cc amount change (0-127)
   */ 
    function ControlChange(channel,control,amount){
       if(channel > 0 && channel < 17)
	  csound.MIDIin(176+channel,control,amount);
    }
    
   /**
   * Sends in a MIDI PROGRAMCHANGE message to Csound MIDI system
   *
   * @param {number} channel MIDI channel (1-16)
   * @param {number} number MIDI pgm number (0-127)
   */ 
    function ProgramChange(channel,control){
       if(channel > 0 && channel < 17)
	  csound.MIDIin(192+channel,control,0);
    }
    
   /**
   * Sends in a MIDI MONOAFT message to Csound MIDI system
   *
   * @param {number} channel MIDI channel (1-16)
   * @param {number} amount  aftertouch amount (0-127)
   */ 
    function Aftertouch(channel,amount){
       if(channel > 0 && channel < 17)
	  csound.MIDIin(208+channel,amount,0);
    }    

   /**
   * Sends in a MIDI PITCHBEND message to Csound MIDI system
   *
   * @param {number} channel MIDI channel (1-16)
   * @param {number} fine fine PB amount (LSB) (0-127)
   * @param {number} coarse coarse PB amount (MSB) (0-127)
   */ 
    function PitchBend(channel,fine,coarse){
       if(channel > 0 && channel < 17)
	   csound.MIDIin(224+channel,fine,coarse);
    }

    
  /**
   * Sets the value of a table position
   *
   * @param {string} num The table to be set.
   * @param {string} pos The pos to set.
   * @param {string} value The value to set.
   */
    function SetTable(num, pos, value){
    var mess = 'setTable:' + num + ':' + pos + ':';
    csound.module.postMessage(mess + value);
   }

  /**
   * Sets the value of a string channel in Csound
   *
   * @param {string} name The channel to be set.
   * @param {string} string The string to set the channel.
   */
  function SetStringChannel(name, value){
    var channel = 'schannel:' + name + ':';
    csound.module.postMessage(channel + value);
   }


  /**
   * Requests the value of a control channel in Csound
   *
   * @param {string} name The channel requested
   *
   * The channel value will be passed from csound as
   * a message with the following format:
   *
   *  ::control::channel:value
   */
  function RequestChannel(name){
    var channel = 'outchannel:' + name;
    if(csound.module != null)
      csound.module.postMessage(channel);
  }

  /**
   * Copies a server file to local/. (not persistent)
   *
   * @param {string} src The src name
   * @param {string} dest The dest name
   */
    function CopyToLocal(src, dest) {
    var ident = 'copyToLocal:' + src + '#';
     csound.module.postMessage(ident + dest);
   }
 
  /**
   * Copies a URL file to local/. (not persistent)
   *
   * NB: works with the origin URL and CORS-ready URLs
   *
   * @param {string} url  The url name
   * @param {string} name The file name
   */
    function CopyUrlToLocal(url, name) {
     var ident = 'copyUrlToLocal:' + url + '#';
     csound.module.postMessage(ident + name);
   }

  /**
   * Requests the data from a local file
   * module sends "Complete" message when done.
   *
   * @param {string} url  The file name
   */
   function RequestFileFromLocal(name) {
     csound.module.postMessage("getFile:" + name);
   }
  /**
   * Returns the most recently requested file data.
   *
   */
   function GetFileData(){
       return fileData;
   }

  /**
   * Requests the data from a table
   * module sends "Complete" message when done.
   *
   * @param {number} num  The table number
   */
   function RequestTable(num) {
     csound.module.postMessage("getTable:" + num);
   }

  /**
   * Returns the most recently requested table data.
   *
   */
   function GetTableData(){
       return tableData;
   }

   function input_ok(s) {
    csound.module.postMessage({input: s.getAudioTracks()[0]});
   }

   function input_fail(e) {
        csound.logMessage("Input audio error: " + e);
   }
   function message(text) {
       csound.logMessage(text);
   }

  /**
   * Start default audio input
   *
   */
    function StartInputAudio() {
     var constraints = {audio:  { mandatory: { echoCancellation: false }}}	
     navigator.webkitGetUserMedia(constraints,input_ok,input_fail);
   }

   return {
    module: null,
    attachDefaultListeners: attachDefaultListeners,
    createModule: createModule,
    destroyModule: destroyModule,
    updateStatus: updateStatus,
    Play: Play,
    Pause: Pause,
    PlayCsd: PlayCsd,
    RenderCsd: RenderCsd,
    CompileOrc: CompileOrc,
    ReadScore: ReadScore,
    Event: Event,
    SetTable : SetTable,
    SetChannel: SetChannel,
    CopyToLocal: CopyToLocal,
    CopyUrlToLocal: CopyUrlToLocal,
    RequestFileFromLocal: RequestFileFromLocal,
    GetFileData : GetFileData,
    RequestChannel: RequestChannel,
    GetTableData : GetTableData,
    RequestTable: RequestTable,
    SetStringChannel: SetStringChannel,
    StartInputAudio: StartInputAudio,
    MIDIin : MIDIin,
    NoteOn : NoteOn,
    NoteOff : NoteOff,
    PolyAftertouch : PolyAftertouch,
    ControlChange : ControlChange,
    ProgramChange : ProgramChange,
    Aftertouch : Aftertouch,
    PitchBend : PitchBend,
    // Common JavaScript API:
    // Should be the same signatures in csound.node, Csound for PNaCl, Csound for Android, CsoundQt.
    compileOrc: CompileOrc,
    message: message,
    perform: Play,
    readScore: ReadScore,
    setControlChannel: SetChannel,
    setStringChannel: SetStringChannel,
    stop: Stop
  };

}());

document.addEventListener('DOMContentLoaded', function() {
     csound.updateStatus('page loaded');
     if (!(navigator.mimeTypes['application/x-pnacl'] !== undefined)) {
        csound.updateStatus('No support for pNaCl (maybe disabled?)');
      } else if (csound.module == null) {
        csound.updateStatus('loading csound module');
        csound.attachDefaultListeners();
        csound.createModule();
    } else {
      csound.updateStatus('not ready');
    }

});


