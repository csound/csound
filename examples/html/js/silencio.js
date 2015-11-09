/**
S I L E N C I O

Copyright (C) 2014 by Michael Gogins

This software is licensed under the terms of the 
GNU Lesser General Public License

Algorithmic music composition library in JavaScript for Csound.
*/

(function () {
    /**
    A Score is a matrix in which the rows are events.
    
    An Event is a homogeneous vector with the following dimensions:
    
     1 Time in seconds from start of performance.
     2 Duration in seconds.
     3 MIDI status (only the most significant nybble, e.g. 144 for 'NOTE ON').
     4 MIDI channel (any real number >= 0, fractional part ties events).
     5 MIDI key number from 0 to 127, 60 is middle C (a real number).
     6 MIDI velocity from 0 to 127, 80 is mezzo-forte (a real number).
     7 x or depth, 0 is the origin.
     8 y or pan, 0 is the origin.
     9 z or heigth, 0 is the origin.
    10 Phase, in radians.
    11 Homogeneity, normally always 1.
    
    NOTE: ECMASCRIPT 5 doesn't support inheritance from Array
    in a clean and complete way, so we don't even try. 
    */

    function eq_epsilon(a, b) {
        var epsilon_factor = 100 * Math.EPSILON;
        if (Math.abs(a - b) > epsilon_factor) {
            return false;
        }
        return true;
    }

    function lt_epsilon(a, b) {
        if (eq_epsilon(a, b)) {
            return false;
        }
        if (a < b) {
            return true;
        }
        return false;
    }

    function gt_epsilon(a, b) {
        if (eq_epsilon(a, b)) {
            return false;
        }
        if (a > b) {
            return true;
        }
        return false;
    }

    function Event() {
        this.data = [0, 0, 144, 0, 0, 0, 0, 0, 0, 0, 1];
        Object.defineProperty(this, "time", {
            get: function () { return this.data[0]; },
            set: function (value) { this.data[0] = value; }
        });
        Object.defineProperty(this, "duration", {
            get: function () { return this.data[1]; },
            set: function (value) { this.data[1] = value; }
        });
        Object.defineProperty(this, "end", {
            get: function () {
                return this.data[0] + this.data[1];
            },
            set: function (x2) {
                var x1 = this.data[0]
                var start = Math.min(x1, x2);
                var end = Math.max(x1, x2);
                this.data[0] = start;
                this.data[1] = end - start;
            }
        });
        Object.defineProperty(this, "status", {
            get: function () { return this.data[2]; },
            set: function (value) { this.data[2] = value; }
        });
        Object.defineProperty(this, "channel", {
            get: function () { return this.data[3]; },
            set: function (value) { this.data[3] = value; }
        });
        Object.defineProperty(this, "key", {
            get: function () { return this.data[4]; },
            set: function (value) { this.data[4] = value; }
        });
        Object.defineProperty(this, "velocity", {
            get: function () { return this.data[5]; },
            set: function (value) { this.data[5] = value; }
        });
        Object.defineProperty(this, "depth", {
            get: function () { return this.data[6]; },
            set: function (value) { this.data[6] = value; }
        });
        Object.defineProperty(this, "pan", {
            get: function () { return this.data[7]; },
            set: function (value) { this.data[7] = value; }
        });
        Object.defineProperty(this, "heigth", {
            get: function () { return this.data[8]; },
            set: function (value) { this.data[8] = value; }
        });
        Object.defineProperty(this, "phase", {
            get: function () { return this.data[9]; },
            set: function (value) { this.data[9] = value; }
        });
        Object.defineProperty(this, "homogeneity", {
            get: function () { return this.data[10]; Event.TIME = 0; },
            set: function (value) { this.data[10] = value; }
        });
    }
    Event.TIME = 0;
    Event.DURATION = 1;
    Event.STATUS = 2;
    Event.CHANNEL = 3;
    Event.KEY = 4;
    Event.VELOCITY = 5;
    Event.X = 6;
    Event.Y = 7;
    Event.Z = 8;
    Event.PHASE = 9;
    Event.HOMOGENEITY = 10;
    Event.COUNT = 11;
    Event.prototype.toString = function () {
        var text = '';
        for (var i = 0; i < this.data.length; i++) {
            text = text.concat(' ', this.data[i].toFixed(6));
        }
        text = text.concat('\n');
        return text;
    }
    Event.prototype.toIStatement = function () {
        var text = 'i';
        text = text.concat(' ', this.data[3].toFixed(6));
        text = text.concat(' ', this.data[0].toFixed(6));
        text = text.concat(' ', this.data[1].toFixed(6));
        text = text.concat(' ', this.data[4].toFixed(6));
        text = text.concat(' ', this.data[5].toFixed(6));
        text = text.concat(' ', this.data[6].toFixed(6));
        text = text.concat(' ', this.data[7].toFixed(6));
        text = text.concat(' ', this.data[8].toFixed(6));
        text = text.concat(' ', this.data[9].toFixed(6));
        return text;
    }
    Event.prototype.temper = function (tonesPerOctave) {
        tonesPerOctave = tonesPerOctave || 12;
        var octave = this.key / 12;
        var tone = Math.floor((octave * tonesPerOctave) + 0.5);
        octave = tone / tonesPerOctave;
        this.key = octave * 12;
    }
    Event.prototype.clone = function () {
        other = new Event();
        other.data = this.data.slice(0);
        return other;
    }

    function Score() {
        this.data = [];
        this.minima = new Event();
        this.maxima = new Event();
        this.ranges = new Event();
    }
    Score.prototype.add = function (p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11) {
        var event = new Event();
        for (var i = 0; i < event.length; i++) {
            if (arguments[i] !== undefined) {
                event[i] = arguments[i];
            }
        }
        this.data.push(event);
    }
    Score.prototype.append = function (event) {
        this.data.push(event);
    }
    Score.prototype.clear = function () {
        while (this.data.length > 0) {
            this.data.pop();
        }
    }
    Score.prototype.getDuration = function () {
        this.sort();
        this.findScale(0);
        var duration = 0;
        for (var i = 0; i < this.data.length; i++) {
            var event = this.data[i];
            if (i == 0) {
                duration = event.end; //data[0] + event.data[1];
            } else {
                var currentDuration = event.end; //data[0] + event.data[1];
                if (currentDuration > duration) {
                    duration = currentDuration;
                }
            }
        }
        return duration;
    }
    Score.prototype.log = function (what) {
        if (what === undefined) {
            what = '';
        } else {
            what = what + ': ';
        }
        for (var i = 0; i < this.data.length; i++) {
            var event = this.data[i];
            csound.message(what + event.toString());
        }
    }
    Score.prototype.setDuration = function (duration) {
        this.sort();
        var start = this.data[0].time;
        for (var i = 0; i < this.data.length; i++) {
            var event = this.data[i];
            event.data[0] = event.data[0] - start;
        }
        var currentDuration = this.data[0].end;
        for (var i = 0; i < this.data.length; i++) {
            var event = this.data[i];
            if (event.end > currentDuration) {
                currentDuration = event.end;
            }
        }
        var factor = Math.abs(duration / currentDuration);
        for (var i = 0; i < this.data.length; i++) {
            var event = this.data[i];
            event.data[0] = event.data[0] * factor;
            event.data[1] = event.data[1] * factor;
        }
    }
    Score.prototype.sendToCsound = function (csound, extra) {
        extra = extra || 5.0;
        this.sort();
        var duration = this.getDuration() + extra;
        jscore = 'f 0 ' + duration + ' 0\n';
        for (var i = 0; i < this.data.length; i++) {
            jscore += this.data[i].toIStatement() + '\n';
        }
        csound.readScore(jscore);
    }
    Score.prototype.findScales = function () {
        for (var i = 0; i < this.minima.data.length; i++) {
            this.findScale(i);
        }
    }
    Score.prototype.findScale = function (dimension) {
        var min = Number.NaN;
        var max = Number.NaN;
        for (var i = 0; i < this.data.length; i++) {
            var value = this.data[i].data[dimension];
            if (i === 0) {
                min = value;
                max = value;
            } else {
                if (value < min) {
                    min = value;
                }
                if (value > max) {
                    max = value;
                }
            }
        }
        this.minima.data[dimension] = min;
        this.maxima.data[dimension] = max;
        this.ranges.data[dimension] = max - min;
    }
    Score.prototype.setScale = function (dimension, minimum, range) {
        this.findScale(dimension);
        var toOrigin = this.minima.data[dimension];
        var currentRange = this.ranges.data[dimension];
        if (currentRange === 0) {
            currentRange = 1;
        }
        var rescale = range / currentRange;
        if (range === undefined) {
            rescale = 1;
        }
        var translate = minimum;
        for (var i = 0; i < this.data.length; i++) {
            var value = this.data[i].data[dimension];
            value -= toOrigin;
            value *= rescale;
            value += translate;
            this.data[i].data[dimension] = value;
        }
    }
    Score.prototype.temper = function (tonesPerOctave) {
        for (var i = 0; i < this.data.length; i++) {
            this.data[i].temper(tonesPerOctave);
        }
    }
    Score.prototype.sort = function () {
        this.data.sort(eventComparator);
    }
    Score.prototype.tieOverlaps = function (tieExact) {
        csound.message("Before tieing: " + this.data.length + "\n");
        tieExact = tieExact || false;
        this.sort();
        for (var laterI = this.data.length - 1; laterI >= 0; laterI--) {
            var laterEvent = this.data[laterI];
            if (laterEvent.status === 144) {
                if (laterEvent.duration <= 0 || laterEvent.velocity <= 0) {
                    this.data.pop();
                } else {
                    for (var earlierI = laterI - 1; earlierI >= 0; earlierI--) {
                        var earlierEvent = this.data[earlierI];
                        var later = false;
                        if (tieExact) {
                            later = !lt_epsilon(earlierEvent.end, laterEvent.time);
                        } else {
                            later = gt_epsilon(earlierEvent.end, laterEvent.time);
                        }
                        if (earlierEvent.status === 144 &&
                            Math.floor(earlierEvent.channel) === Math.floor(laterEvent.channel) &&
                            Math.round(earlierEvent.key) === Math.round(laterEvent.key) &&
                            later) {
                            earlierEvent.end = laterEvent.end;
                            this.data.splice(laterI, 1);
                        }
                    }
                }
            }
        }
        csound.message("After tieing: " + this.data.length + "\n");
    }
    Score.prototype.draw = function (canvas, W, H) {
        this.findScales();
        csound.message("minima:  " + this.minima + "\n");
        csound.message("ranges:  " + this.ranges + "\n");
        var xsize = this.getDuration();
        var ysize = this.ranges.key;
        var xscale = Math.abs(W / xsize);
        var yscale = Math.abs(H / ysize);
        var xmove = -this.minima.time;
        var ymove = -this.minima.key;
        var context = canvas.getContext("2d");
        context.scale(xscale, yscale);
        context.translate(xmove, ymove);
        csound.message("score:  " + xsize + ", " + ysize + "\n");
        csound.message("canvas: " + W + ", " + H + "\n");
        csound.message("scale:  " + xscale + ", " + yscale + "\n");
        csound.message("move:   " + xmove + ", " + ymove + "\n");
        var channelRange = this.ranges.channel;
        if (channelRange == 0) {
            channelRange = 1;
        }
        var velocityRange = this.ranges.velocity;
        if (velocityRange == 0) {
            velocityRange = 1;
        }
        for (var i = 0; i < this.data.length; i++) {
            var x1 = this.data[i].time;
            var x2 = this.data[i].end;
            var y = this.data[i].key;
            var hue = this.data[i].channel - this.minima.channel;
            hue = hue / channelRange;
            var value = this.data[i].velocity - this.minima.velocity;
            value = value / velocityRange;
            value = .5 + value / 2;
            var hsv = "hsv(" + hue + "," + 1 + "," + value + ")";
            context.strokeStyle = tinycolor(hsv).toHexString();
            //csound.message("color: " + context.strokeStyle + "\n");
            //context.strokeStyle = 'red';
            context.beginPath();
            context.moveTo(x1, y);
            context.lineTo(x2, y);
            context.stroke();
            //csound.message("note " + i + ": " + x1 + ", " + x2 + ", " + y + "\n");
        }
    }
    function eventComparator(a, b) {
        for (var i = 0; i < a.data.length; i++) {
            var avalue = a.data[i];
            var bvalue = b.data[i];
            var difference = avalue - bvalue;
            if (difference !== 0) {
                return difference
            }
        }
        return 0;
    }

    function Turtle(len, theta) {
        this.len = len;
        this.theta = theta;
        this.reset();
        return this;
    }
    Turtle.prototype.reset = function () {
        this.angle = Math.PI / 2;
        this.p = { 'x': 0, 'y': 0 };
        this.stack = [];
        this.instrument = 'red';
        this.tempo = 1;
    };
    Turtle.prototype.next = function () {
        return {
            'x': this.p.x + this.len * this.tempo * Math.cos(this.angle),
            'y': this.p.y - this.len * Math.sin(this.angle)
        };
    };
    Turtle.prototype.go = function (context) {
        var nextP = this.next();
        context.strokeStyle = tinycolor(this.instrument).toString();
        context.beginPath();
        context.moveTo(this.p.x, this.p.y);
        context.lineTo(nextP.x, nextP.y);
        context.stroke();
        this.p = nextP;
    };
    Turtle.prototype.move = function () {
        this.p = this.next();
    };
    Turtle.prototype.turnLeft = function () {
        this.angle += this.theta;
    };
    Turtle.prototype.turnRight = function () {
        this.angle -= this.theta;
    };
    Turtle.prototype.upInstrument = function () {
        this.instrument = tinycolor(this.instrument).spin(10);
    };
    Turtle.prototype.downInstrument = function () {
        this.instrument = tinycolor(this.instrument).spin(-10);
    };
    Turtle.prototype.upVelocity = function () {
        this.instrument = tinycolor(this.instrument).darken(-1);
    };
    Turtle.prototype.downVelocity = function () {
        this.instrument = tinycolor(this.instrument).darken(1);
    };
    Turtle.prototype.upTempo = function () {
        this.tempo = this.tempo / 1.25;
    };
    Turtle.prototype.downTempo = function () {
        this.tempo = this.tempo * 1.25;
    };
    Turtle.prototype.push = function () {
        this.stack.push({ 'p': this.p, 'angle': this.angle, 'instrument': this.instrument, 'tempo': this.tempo });
    };
    Turtle.prototype.pop = function () {
        var s = this.stack.pop();
        this.p = s.p;
        this.angle = s.angle;
        this.instrument = s.instrument;
        this.tempo = s.tempo;
    };
    function LSys() {
        this.axiom = '';
        this.rules = {};
        this.prior = '';
        this.score = new silencio.Score();
        this.event = new silencio.Event();
        return this;
    }
    LSys.prototype.addRule = function (c, replacement) {
        this.rules[c] = replacement;
    }
    LSys.prototype.generate = function (n) {
        this.sentence = this.axiom;
        for (var g = 0; g < n; g++) {
            var next = [];
            for (var i = 0; this.sentence.length > i; i++) {
                var c = this.sentence[i];
                var r = this.rules[c];
                if (r) {
                    next.push(r);
                } else {
                    next.push(c);
                }
            }
            this.sentence = next.join("");
        }
    };
    LSys.prototype.draw = function (t, context, W, H) {
        context.fillStyle = 'black';
        context.fillRect(0, 0, W, H);
        // Draw for size. 
        t.reset();
        var size = [t.p.x, t.p.y, t.p.x, t.p.y];
        for (var i = 0; this.sentence.length > i; i++) {
            var c = this.sentence[i];
            this.interpret(c, t, context, size);
        }
        // Draw to show. 
        var xsize = size[2] - size[0];
        var ysize = size[3] - size[1];
        var xscale = Math.abs(W / xsize);
        var yscale = Math.abs(H / ysize);
        var xmove = -size[0];
        var ymove = -size[1];
        context.scale(xscale, yscale);
        context.translate(xmove, ymove);
        t.reset();
        for (var i = 0; this.sentence.length > i; i++) {
            var c = this.sentence[i];
            this.interpret(c, t, context);
        }
    };
    LSys.prototype.findSize = function (t, size) {
        if (t.p.x < size[0]) {
            size[0] = t.p.x;
        }
        if (t.p.y < size[1]) {
            size[1] = t.p.y;
        }
        if (t.p.x > size[2]) {
            size[2] = t.p.x;
        }
        if (t.p.y > size[3]) {
            size[3] = t.p.y;
        }
    };
    LSys.prototype.startNote = function (t) {
        var hsv = tinycolor(t.instrument).toHsv();
        this.event.channel = hsv.h;
        this.event.time = t.p.x;
        this.event.key = -t.p.y;
        this.event.velocity = hsv.v;
        this.event.pan = Math.random();
    }
    LSys.prototype.endNote = function (t) {
        // Handle the note ending before it starts. 
        var x1 = this.event.time;
        var x2 = t.p.x;
        this.event.time = Math.min(x1, x2);
        this.event.end = Math.max(x1, x2);
        if (this.event.duration > 0) {
            var event = this.event.clone();
            this.score.data.push(event);
        }
    }
    LSys.prototype.interpret = function (c, t, context, size) {
        if (c === 'F') {
            if (size === undefined) {
                this.startNote(t);
                t.go(context);
            } else {
                t.move();
            }
        }
        else if (c === 'f') t.move();
        else if (c === '+') t.turnRight();
        else if (c === '-') t.turnLeft();
        else if (c === '[') t.push();
        else if (c === ']') t.pop();
        else if (c === 'I') t.upInstrument();
        else if (c === 'i') t.downInstrument();
        else if (c === 'V') t.upVelocity();
        else if (c === 'v') t.downVelocity();
        else if (c === 'T') t.upTempo();
        else if (c === 't') t.downTempo();
        if (size === undefined) {
            if (c === 'F') {
                this.endNote(t);
            }
            this.prior = c;
        } else {
            this.findSize(t, size);
        }

    };

    /**
    Generates scores by recursively applying a set of generating
    functions to a single initial musical event.
    This event can be considered to represent a cursor within a score.
    The generating functions may move this cursor around
    within the score, as if moving a pen, and may at any time write the
    current state of the cursor into the score, or write other events
    based, or not based, upon the cursor into the score.
    
    The generating functions may be lambdas. Generated notes must not
    be the same object as the cursor, but may be clones of the cusor,
    or entirely new objects.
    
    cursor          A Silencio.Event object that represents a position in a
                    musical score. This could be a note, a grain of sound, or
                    a control event.
    
    depth           The current depth of recursion. This must begin > 1.
                    For each recursion, the depth is decremented. Recursion
                    ends when the depth reaches 0.
    
    Returns the next position of the cursor, and optionally, a table of
    generated events.
    
    generator = function(cursor, depth);
    {cursor, events} = generator(cursor, depth);
    
    The algorithm is similar to the deterministic algorithm for computing the
    attractor of a recurrent iterated function systems. Instead of using
    affine transformation matrixes as the RIFS algorithm does, the current
    algorithm uses generating functions; but if each generating function
    applies a single affine transformation to the cursor, the current
    algorithm will in fact compute a RIFS.
    
    generators      A list of generating functions with the above signature.
                    Unlike RIFS, the functions need not be contractive.
    
    transitions     An N x N transition matrix for the N generating functions.
                    If entry [i][j] is 1, then if the current generator is the
                    ith, then the jth generator will be applied to the current
                    cursor after the ith; if the entry is 0, the jth generator
                    will not be applied to the current cursor after the ith.
                    In addition, each generator in the matrix must be reached
                    at some point during recursion.
    
    depth           The current depth of recursion. This must begin > 1.
                    For each recursion, the depth is decremented. Recursion
                    ends when the depth reaches 0.
    
    index           Indicates the current generating function, i.e. the
                    index-th row of the transition matrix.
    
    cursor          A Silencio.Event object that represents a position in a
                    musical score. This could be a note, a grain of sound, or
                    a control event.
    
    score           A Silencio.Score object that collects generated events.
    */

    function RecurrentResult(c) {
        this.cursor = c;
        this.events = [];
        return this;
    }

    function Recurrent(generators, transitions, depth, index, cursor, score) {
        depth = depth - 1;
        //print(string.format('Recurrent(depth: %d  index: %d  cursor: %s)', depth, index, cursor:__tostring()))
        if (depth == 0) {
            return;
        }
        var transitionsForThisIndex = transitions[index];
        for (var j = 0; j < transitionsForThisIndex.length; j++) {
            if (transitionsForThisIndex[j] == 1) {
                var result = generators[j](cursor.clone(), depth);
                for (var i = 0; i < result.events.length; i++) {
                    score.append(result.events[i].clone());
                }
                Recurrent(generators, transitions, depth, j, result.cursor.clone(), score);
            }
        }
    }

    var silencio = {
        eq_epsilon: eq_epsilon,
        gt_epsilon: gt_epsilon,
        lt_epsilon: lt_epsilon,
        Event: Event,
        Score: Score,
        Turtle: Turtle,
        LSys: LSys,
        RecurrentResult: RecurrentResult,
        Recurrent: Recurrent
    };
    // Node: Export function
    if (typeof module !== "undefined" && module.exports) {
        module.exports = silencio;
    }
        // AMD/requirejs: Define the module
    else if (typeof define === 'function' && define.amd) {
        define(function () { return silencio; });
    }
        // Browser: Expose to window
    else {
        window.silencio = silencio;
    }

})();
