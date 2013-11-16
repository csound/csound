/*
 * Csound pnacl interactive frontend
 * icsound messaging functions
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

function icsoundPlay() {
  common.naclModule.postMessage('playSound');
}

function icsoundPause() {
  common.naclModule.postMessage('stopSound');
}

function icsoundCompileOrc(s) {
 common.naclModule.postMessage('orchestra:' + s);
}

function icsoundReadScore(s) {
 common.naclModule.postMessage('score:' + s);
}

function icsoundEvent(s) {
 common.naclModule.postMessage('event:' + s);
}

function icsoundSetChannel(name, value){
   var channel = 'channel:' + name + ':';
   common.naclModule.postMessage(channel + value);
}
