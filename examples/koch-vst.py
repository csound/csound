# Copyright (c) 2007 by Michael Gogins.
# This file is licensed under the terms of the GNU Lesser General Public License.

# This Python script is designed to be executed inside a ScoreGen plugin acting as a "MIDI insert effect" in Cubase.

# A recursive function for generating a score
# in the form of a Koch curve.
# Each note in a generating motive
# will get a smaller copy of the motive nested atop it,
# and so on.

gainsForLevels = {}
gainsForLevels[0] = 0.0
gainsForLevels[1] = 1.5
gainsForLevels[2] = 1.0
gainsForLevels[3] = 1.0
gainsForLevels[4] = 1.0
gainsForLevels[5] = 1.0
gainsForLevels[6] = 1.0
gainsForLevels[7] = 1.0

def Koch(generator, t, d, c, k, v, level):
        t1 = t
        d1 = d
        c1 = c
        k1 = k
        v1 = v
        pan = 0
        if level > 0:
                for i in range(0, generator.__len__(), 4):
                        k1 = k + generator[i]
                        v1 = generator[i + 1]
                        d1 = d  * generator[i + 2]
                        d2 = d1 * generator[i + 3]
                        # The built-in "score" object stands for a MIDI track and sends MIDI notes to the VST host.
                        score.event(float(t1), float(d2), float(144), float(level), float(k1), float((70.0 + v1) * gainsForLevels[level]))
                        Koch(generator, t1, d1, c1, k1 + 12, v1, level - 1)
                        t1 = t1 + d1

# Normalizes the times in a generator.

def normalizeGeneratorTimes(g):
        sum = 0.0
        for i in range(0, len(g), 4):
                sum = (sum + g[i + 2])
        for i in range(0, len(g), 4):
                g[i + 2] = g[i + 2] / sum

# Define two generators for the Koch function.
# Each consists of a sequence of tuples:
# {{add to MIDI key, add to MIDI velocity, relative time, normalized duration},...}

g = [  7,  0,  8,  1,
      -4,  0,  8,  .875,
       7,  1,  6,  1,
       9,  1,  6,  1,
      -4, -5,  8,  1,
      15,  4,  3,  .875,
      -9,  1,  6,  1
    ]

normalizeGeneratorTimes(g)

# The only differences between this generator and the first are the
# relative durations and dynamics of some sections.
# The relative pitches and numbers of events are identical.
# The different times will move the canon offset
# back and forth during performance.

h = [  7,  0,  8,  1,
      -4,  0,  6,  .875,
       7,  0,  3,  1,
       9,  1,  6,  1,
      -4,  1,  6,  1,
      15,  4,  6,  .875,
      -7, -5,  8,  1
#     -13,  0,  8,  1
    ]

normalizeGeneratorTimes(h)

# Generate events for 3 layers of each generator.

Koch(g, 0, 240, 0, 24, 32, 3)

# The second generator makes a canon relative to the first.

Koch(h, 0.5, 240, 0, 30, 32, 3)


