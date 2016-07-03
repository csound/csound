
//Copyright (c) 2012 Hugh Fisher

//Permission is hereby granted, free of charge, to any person obtaining a
//copy of this software and associated documentation files (the "Software"),
//to deal in the Software without restriction, including without limitation
//the rights to use, copy, modify, merge, publish, distribute, sublicense,
//and/or sell copies of the Software, and to permit persons to whom the
//Software is furnished to do so, subject to the following conditions:

//The above copyright notice and this permission notice shall be included
//in all copies or substantial portions of the Software.

//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
//OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
//THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//DEALINGS IN THE SOFTWARE.

//  My GPU utility code translated into JavaScript


var gpu = {};

gpu.getShaderCode = function(id)
    // Extract shader script from page
{
    var script, result, node;
    script = document.getElementById(id);
    if (! script) {
        return null;
    }
    result = "";
    node = script.firstChild;
    while (node) {
        if (node.nodeType === 3) {
            result += node.textContent;
        }
        node = node.nextSibling;
    }
    return result;
};

gpu.compileShader = function(kind, source)
    // Kind must be gl.VERTEX_SHADER, gl.FRAGMENT_SHADER.
    // source is just a string
{
    var shader, msg;

    // Shader itself. Assume only one per program
    shader = gl.createShader(kind);
    // And only one source string per shader
    gl.shaderSource(shader, source);

    // Now compile and report any errors
    gl.compileShader(shader);
    if (! gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
        if (kind === gl.VERTEX_SHADER) {
            msg = "Error in vertex shader: ";
        } else if (kind === gl.FRAGMENT_SHADER) {
            msg = "Error in fragment shader: ";
        }
        throw { name : 'OpenGL Error',
                message : msg + gl.getShaderInfoLog(shader)};
    }
    // Compiled OK doesn't mean perfect: might still fail link
    // with fragment shader when final program is created
    return shader;
};

gpu.loadShader = function(kind, id)
    // Kind VERTEX or FRAGMENT
    // id is document element id
{
    var src = gpu.getShaderCode(id);
    if (src === undefined) {
        throw { name : 'OpenGL Error',
                message : "Cannot load shader script " + id};
    }
    return gpu.compileShader(kind, src);
};

gpu.newProgram = function(vertex, fragment)
{
    var prog;

    // Sanity check
    if (! vertex && ! fragment) {
        throw { name : 'OpenGL Error',
                message : "gpu.newProgram without any shaders!"};
    }

    prog = gl.createProgram();
    // Actually no difference between shader types when attaching
    if (vertex) {
        gl.attachShader(prog, vertex);
    }
    if (fragment) {
        gl.attachShader(prog, fragment);
    }

    // And make sure they play nice with each other
    gl.linkProgram(prog);
    if (! gl.getProgramParameter(prog, gl.LINK_STATUS)) {
        throw {name : 'OpenGL Error',
            message : "Error linking GPU program: " + gl.getProgramInfoLog(prog)};
    }
    gl.validateProgram(prog);
    if (! gl.getProgramParameter(prog, gl.VALIDATE_STATUS)) {
        throw { name : 'OpenGL Error',
                message : "Validation error for GPU program: "  + gl.getProgramInfoLog(prog)};
    }
    
    return prog;
};

gpu.getUniform = function(program, name)
{
    var h;
    
    h = gl.getUniformLocation(program, name);
    if (h < 0) {
        throw { name : 'OpenGL Error',
                message : "Cannot get location of uniform " + name + " in shader"};
    }
    return h;
}

gpu.getAttribute = function(program, name)
{
    var h;
    
    h = gl.getAttribLocation(program, name);
    if (h < 0) {
        throw { name : 'OpenGL Error',
                message : "Cannot get location of attribute " + name + " in shader"};
    }
    return h;
}


