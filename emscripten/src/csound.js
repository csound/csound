/*
 * Csound JS frontend, adapted from PNaCl Csound
 *
 * Copyright (C) 2017 V Lazzarini
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

    var Csound = null;
    
    function load_dep(file,elm,callback) {
	var jsl = document.createElement(elm);
	jsl.type = "text/javascript";
	jsl.src = file;
	document.getElementsByTagName("head")[0].appendChild(jsl)
	jsl.onload =  callback;
	console.log("loading: " + file);
    }

    function absolute_path() {
	var scriptElements = document.getElementsByTagName('script');
	for (var i = 0; i < scriptElements.length; i++) {
            var source = scriptElements[i].src;
            if (source.indexOf("csound.js") > -1) {
		var location = source.substring(0, source.indexOf("csound.js"));
		return location;
	    }
	}
	return "";
    }
    
    function createModule() {
	var path = absolute_path();
        load_dep(path + "libcsound.js", "script", function() {
	    load_dep(path + "CsoundObj.js", "script", function() {
	     console.log("loaded scripts");
             Module["onRuntimeInitialized"] = function() {
		console.log("loaded WASM runtime");    
		csound.Csound = new CsoundObj();
		csound.Csound.setMidiCallbacks(); 
		csound.module = true;
		if(typeof window.moduleDidLoad !== 'undefined') 
                    window.moduleDidLoad();
		if(typeof window.attachListeners !== 'undefined') 
                    window.attachListeners();
	    };
	  });});
    }
    
    var fileData = null;
    var tableData = null;
    var statusText = 'Not loaded.';
    
    /**
     * Prints current status to the console.
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

    var started = false;
    /**
     * Starts audio playback.
     */
    function Play() {
	if(csound.started == false) {
        csound.Csound.compileOrc("nchnls=2\n 0dbfs=1\n");
        csound.Csound.start();
        csound.started = true;   
	} else { 
        csound.Csound.start();    
    }
    } 

    /**
     * Get Score time in seconds
     */
    function GetScoreTime() {
	return csound.Csound.getScoreTime();  
    }

    /**
     * Pauses audio playback.
     */
    function Pause() {
	csound.Csound.stop();
    }
    
    /**
     * Stops rendering and resets csound.Csound.
     */
    function Stop() {
	csound.Csound.stop();
	csound.Csound.reset();
    }

    /**
     * Sends orchestra code to be compiled by csound.Csound.
     *
     * @param {string} s A string containing the code.
     */
    function CompileOrc(s) {
	csound.Csound.evaluateCode(s);
    }

    function loadCSD(url, callback) {
	var xmlHttpRequest = new XMLHttpRequest();
	xmlHttpRequest.onload = function () {
	    var data = new Uint8Array(xmlHttpRequest.response);
	    var stream = FS.open(url, 'w+');
            FS.write(stream, data, 0, data.length, 0);
	    FS.close(stream);
	    callback();   
	};
	xmlHttpRequest.open("get", url, true);
	xmlHttpRequest.responseType = "arraybuffer";
	xmlHttpRequest.send(null);
    }
    
    /**
     * Starts real-time audio playback with a CSD. The variable can contain 
     * a filepath or the literal text of a CSD.
     *
     * @param {string} s A string containing the pathname to the CSD.
     */
    function PlayCsd(s) {
        loadCSD(s, function () {
            csound.Csound.compileCSD(s);
            csound.Csound.start();
            started = true; });
    }

    /**
     * Compiles a CSD passed as a string of text.
     *
     * @param {string} s A string containing the complete text of the CSD.
     */
    function CompileCsdText(s) {
        // This function internally discriminates between CSD text 
        // and CSD pathnames.
        csound.Csound.compileCSD(s);
        started = true;
    }

    /**
     * Starts file rendering with a CSD (no real-time audio). The variable can contain 
     * a filepath or the literal text of a CSD.
     *
     * @param {string} s A string containing the pathname to the CSD.
     * @param {function} callback completion callback
     */
    function RenderCsd(s, callback = null) {
	loadCSD(s, function () {
	    csound.Csound.render(s);
	    callback();
	});
    }

    /**
     * Sends a score to be read by csound.Csound.
     *
     * @param {string} s A string containing the score.
     */
    function ReadScore(s) {
	csound.Csound.readScore(s);
    }

    /**
     * Sends line events to csound.Csound.
     *
     * @param {string} s A string containing the line events.
     */
    function Event(s) {
	csound.Csound.readScore(s);
    }

    /**
     * Sets the value of a control channel in csound.Csound.
     *
     * @param {string} name The channel to be set.
     * @param {number} value The value to set the channel.
     */
    function SetChannel(name, value){
	csound.Csound.setControlChannel(name, value);
    }

    /**
     * Sends a MIDI channel message to Csound's MIDI input.
     *
     * @param {number} byte1 first midi byte (128-255)
     * @param {number} byte2 second midi byte (0-127)
     * @param {number} byte3 third midi byte (0-127)
     */
    function MIDIin(byte1, byte2, byte3){
        if(byte1 < 128 || byte1 > 255) return;
        if(byte2 < 0 || byte2 > 127) return;
	if(byte3 < 0 || byte3 > 127) return;
	csound.Csound.midiMessage(byte1,byte2,byte3);
    }
    
    /**
     * Sends a MIDI NOTEOFF message to Csound's MIDI input.
     *
     * @param {number} channel MIDI channel (1-16)
     * @param {number} number MIDI note (0-127)
     * @param {number} velocity MIDI velocity (0-127)
     */ 
    function NoteOff(channel,number,velocity){
	if(channel > 0 && channel < 17)
	    csound.Csound.midiMessage(127+channel,number,velocity);
    }

    /**
     * Sends in a MIDI NOTEON message to Csound's MIDI input.
     *
     * @param {number} channel MIDI channel (1-16)
     * @param {number} number MIDI note (0-127)
     * @param {number} velocity MIDI velocity (0-127)
     */
    function NoteOn(channel,number,velocity){
	if(channel > 0 && channel < 17)
	    csound.Csound.midiMessage(143+channel, number, velocity);
    }

    /**
     * Sends in a MIDI POLYAFT message to Csound's MIDI input.
     *
     * @param {number} channel MIDI channel (1-16)
     * @param {number} number MIDI note (0-127)
     * @param {number} aftertouch MIDI aftertouch (0-127)
     */ 
    function PolyAftertouch(channel,number,aftertouch){
	if(channel > 0 && channel < 17)
	    csound.Csound.midiMessage(159+channel, number, aftertouch);
    }
    
    /**
     * Sends in a MIDI CONTROLCHANGE message to Csound's MIDI input.
     *
     * @param {number} channel MIDI channel (1-16)
     * @param {number} control MIDI cc number (0-127)
     * @param {number} amount  cc amount change (0-127)
     */ 
    function ControlChange(channel,control,amount){
	if(channel > 0 && channel < 17)
	    csound.Csound.midiMessage(175+channel, control, amount);
    }
    
    /**
     * Sends in a MIDI PROGRAMCHANGE message to Csound's MIDI input.
     *
     * @param {number} channel MIDI channel (1-16)
     * @param {number} number MIDI pgm number (0-127)
     */ 
    function ProgramChange(channel,control){
	if(channel > 0 && channel < 17)
	    csound.Csound.midiMessage(191+channel, control, 0);
    }
    
    /**
     * Sends in a MIDI MONOAFT message to Csound's MIDI input.
     *
     * @param {number} channel MIDI channel (1-16)
     * @param {number} amount  aftertouch amount (0-127)
     */ 
    function Aftertouch(channel,amount){
	if(channel > 0 && channel < 17)
	    csound.Csound.midiMessage(207+channel,amount,0);
    }    

    /**
     * Sends in a MIDI PITCHBEND message to Csound's MIDI input.
     *
     * @param {number} channel MIDI channel (1-16)
     * @param {number} fine fine PB amount (LSB) (0-127)
     * @param {number} coarse coarse PB amount (MSB) (0-127)
     */ 
    function PitchBend(channel,fine,coarse){
	if(channel > 0 && channel < 17)
	    csound.Csound.midiMessage(223+channel, fine, coarse);
    }

    /**
     * Sets the value of a table element.
     *
     * @param {string} num The table to be set.
     * @param {string} pos The pos to set.
     * @param {string} value The value to set.
     */
    function SetTable(num, pos, value){
        csound.Csound.setTable(num, pos, value);
    }

    /**
     * Sets the value of a string channel in csound.Csound.
     *
     * @param {string} name The channel to be set.
     * @param {string} string The string to set the channel.
     */
    function SetStringChannel(name, string){
	csound.Csound.setStringChannel(name, string);
    }

    /**
     * Requests the value of a control channel in csound.Csound.
     *
     * @param {string} name The channel requested
     *
     */
    function RequestChannel(name){
	return csound.Csound.getControlChannel(name);
    }

    /**
     * Copies a server file to local/. (not persistent).
     *
     * @param {string} src The src name
     * @param {string} dest The dest name
     */
    function CopyToLocal(src, dest) {
	csound.CopyUrlToLocal(src, dest);
    }
    
    /**
     * Copies a URL file to local/. (not persistent).
     *
     * NB: works with the origin URL and CORS-ready URLs
     *
     * @param {string} url  The url name
     * @param {string} name The file name
     * @param {function} callback completion callback
     */
    function CopyUrlToLocal(url, name, callback=null) {
	var xmlHttpRequest = new XMLHttpRequest();
	xmlHttpRequest.onload = function () {
	    var data = new Uint8Array(xmlHttpRequest.response);
	    var stream = FS.open(name, 'w+');
            FS.write(stream, data, 0, data.length, 0);
	    FS.close(stream);
	    if(callback != null) callback();  
	};
	xmlHttpRequest.open("get", url, true);
	xmlHttpRequest.responseType = "arraybuffer";
	xmlHttpRequest.send(null);
    }

    /**
     * Requests the data from a local file;
     * module sends "Complete" message when done.
     *
     * @param {string} url  The file name
     */
    function RequestFileFromLocal(name) {
	fileData = FS.readFile(name, {encoding: 'binary'});
    }
    
    /**
     * Returns the most recently requested file data.
     *
     */
    function GetFileData(){
	return fileData;
    }

    /**
     * Requests the data from a table;
     * module sends "Complete" message when done.
     *
     * @param {number} num  The table number
     */
    function RequestTable(num) {
	tableData = csound.Csound.getTable(num);
    }

    /**
     * Returns the most recently requested table data.
     *
     */
    function GetTableData(){
	return tableData;
    }

    function message(text) {
	csound.updateStatus(text);
    }

    function start() {
        csound.Csound.start();
    }
    /**
     * Start default audio input.
     *
     */
    function StartInputAudio() {
	csound.Csound.enableInput(function(status) {
            if(status) csound.updateStatus("enabled audio input\n");
	    else csound.updateStatus("failed to enable audio input\n");
	});
    }

    return {
        module: false,
	/* Keep these in alphabetical order: */
	Csound : Csound,
        Aftertouch : Aftertouch,
        CompileOrc: CompileOrc,
        compileOrc: CompileOrc,
        CompileCsdText: CompileCsdText,
        compileCsdText: CompileCsdText,
        ControlChange : ControlChange,
        CopyToLocal: CopyToLocal,
        CopyUrlToLocal: CopyUrlToLocal,
        createModule: createModule,
        Event: Event,
        GetFileData : GetFileData,
        GetScoreTime: GetScoreTime,
        getScoreTime: GetScoreTime,
        GetTableData: GetTableData,
        message: message,
        MIDIin : MIDIin,
        NoteOff : NoteOff,
        NoteOn : NoteOn,
        Pause: Pause,
        perform: Play,
        PitchBend: PitchBend,
        Play: Play,
        PlayCsd: PlayCsd,
        PolyAftertouch : PolyAftertouch,
        ProgramChange : ProgramChange,
        ReadScore: ReadScore,
        readScore: ReadScore,
        RenderCsd: RenderCsd,
        RequestChannel: RequestChannel,
        RequestFileFromLocal: RequestFileFromLocal,
        RequestTable: RequestTable,
        SetChannel: SetChannel,
        setControlChannel: SetChannel,
        SetStringChannel: SetStringChannel,
        setStringChannel: SetStringChannel,
        SetTable : SetTable,
        StartInputAudio: StartInputAudio,
        start: start,
        Start: Play,
        stop: Stop,
        updateStatus: updateStatus
    };
}());

document.addEventListener('DOMContentLoaded', function() {
    csound.updateStatus('page loaded');
    if (csound.module == false) {
        csound.updateStatus('Loading WASM Csound module.\nThis might take a little while.');
        csound.createModule();
    } else {
	csound.updateStatus('Not ready.');
    }
    window.addEventListener("unload", function(e){
	if(csound != null && csound.Csound != null)
            csound.Csound.destroy();
    }, false);

});


