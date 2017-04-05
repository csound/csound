sr=44100
kr=44100
ksmps=1
nchnls=1

instr 1

        k1      line            0, p3, 10

                printks         {{
Hello there!
This should be in a second line.
Anyway I didn't mean to write so much useless things.
I just want to say that k1 is %f at the moment.
Ok now let's not waste more time and resume the performance, ok?
Hey!?

Oh, one last thing. This

#include "dontincludeme.orc"

is not a real inclusion directive.
This

#define M1 #hahahahah#

is not a real macro definition. And this

$M1

is not a real macro expansion directive.

Notice this is also a quite long text.
}}, 1, k1

endin

instr 2

        ; This is not {{ heredoc }}

        ; The is not a {{
        ; multiline
        ; heredoc }}

        k1      line            0, p3, 10

                printks         "This is not {{heredoc}}: %f\\n", 1, k1
endin
