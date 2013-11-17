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

/** 
 * @constructor
 */
var icsound = (function() {

  /**
   * Loads icsound pNaCl module
   */
  function createModule() {
    var model = document.createElement('embed');
    model.setAttribute('name', 'icsound_module');
    model.setAttribute('id', 'icsound_module');
    model.setAttribute('path', 'pnacl/Release');
    model.setAttribute('src', 'pnacl/Release/icsound.nmf');
    var mimetype = 'application/x-pnacl';
    model.setAttribute('type', mimetype);
    var icsoundhook = document.getElementById('engine');
    icsoundhook.appendChild(model);
  }
  
  /**
   * Attaches handlers for events
   */
  function attachDefaultListeners() {
    var icsoundhook= document.getElementById('engine');
    icsoundhook.addEventListener('load', moduleDidLoad, true);
    icsoundhook.addEventListener('message', handleMessage, true);
    icsoundhook.addEventListener('crash', handleCrash, true);
    icsoundhook.addEventListener('progress', handleProgress, true);
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
    if (icsound.module.exitStatus == -1) {
      updateStatus('Ops, something went wrong... please refresh page',1);
    } else {
      updateStatus('Csound has exited [' + icsound.module.exitStatus + ']');
    }
    if (typeof window.handleCrash !== 'undefined') {
      window.handleCrash(icsound.module.lastError);
    }
  }

  /**
   *  After module loading, point the member var to it
   */
   function moduleDidLoad() {
    icsound.module = document.getElementById('icsound_module');
       updateStatus('ready', 1);
    if (typeof window.moduleDidLoad !== 'undefined') {
      window.moduleDidLoad();
    }
  }

  /**
   * Unloads/destroys module.
   */
  function destroyModule() {
    icsound.module.parentNode.removeChild(icsound.module);
    icsound.module = null;
  }

  /**
   * handles messages by passing them to the window handler
   *
   * @param {Event} event A message event. 
   *                message_event.data contains
   *                the data sent from the icsound module.
   */
  function handleMessage(event) {
    if (typeof window.handleMessage !== 'undefined') {
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
	if(!keep) statusField.value = statusText;
        else statusField.value += statusText;
    }
  }

  /**
   * Starts audio playback
   */
  function Play() {
    icsound.module.postMessage('playSound');
  } 

  /**
   * Pauses audio playback
   */
  function Pause() {
   icsound.module.postMessage('stopSound');
  }

  /**
   * Sends code to be compiled by Csound
   *
   * @param {string} s A string containing the code.
   */
  function CompileOrc(s) {
   icsound.module.postMessage('orchestra:' + s);
  }

  /**
   * Sends a score to be read by Csound
   *
   * @param {string} s A string containing the score.
   */
  function ReadScore(s) {
    icsound.module.postMessage('score:' + s);
   }

  /**
   * Sends line events to Csound
   *
   * @param {string} s A string containing the line events.
   */
  function Event(s) {
    icsound.module.postMessage('event:' + s);
   }

  /**
   * Sets the value of a control channel in Csound
   *
   * @param {string} name The channel to be set.
   * @param {string} value The value to set the channel.
   */
  function SetChannel(name, value){
    var channel = 'channel:' + name + ':';
    icsound.module.postMessage(channel + value);
   }

  /**
   * Copies a server file to local/. (not persistent)
   *
   * @param {string} src The src name
   * @param {string} dest The dest name
   */
    function CopyToLocal(src, dest) {
    var ident = 'copyToLocal:' + src + '#';
     icsound.module.postMessage(ident + dest);
   }
 
  /**
   * Copies a URL file to local/. (not persistent)
   *
   * NB: not implemented yet
   *
   * @param {string} url  The url name
   * @param {string} name The file name
   */
    function CopyUrlToLocal(url, name) {
     var ident = 'copyUrlToLocal:' + url + '#';
     icsound.module.postMessage(ident + name);
   }

  return {
    module: null,
    attachDefaultListeners: attachDefaultListeners,
    createModule: createModule,
    destroyModule: destroyModule,
    updateStatus: updateStatus,
    Play: Play,
    Pause: Pause,
    CompileOrc: CompileOrc,
    ReadScore: ReadScore,
    Event: Event,
    SetChannel: SetChannel,
    CopyToLocal: CopyToLocal,
    CopyUrlToLocal: CopyUrlToLocal
  };

}());

document.addEventListener('DOMContentLoaded', function() {

     icsound.updateStatus('page loaded');
     if (!(navigator.mimeTypes['application/x-pnacl'] !== undefined)) {
        icsound.updateStatus('No support for pNaCl (maybe disabled?)');
      } else if (icsound.module == null) {
        icsound.updateStatus('loading icsound module');
        icsound.attachDefaultListeners();
         icsound.createModule();
    } else {
      icsound.updateStatus('not ready');
    }

});


