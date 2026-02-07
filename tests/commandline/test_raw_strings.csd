<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
0dbfs = 1
instr 1
 prints R{ "type": "checkBox",
            "bounds":{"left":%d, "top":%d, "width":30, "height":30},
            "channels": [{"id": "check%d", "range": {"defaultValue": 0}}],
            "style": {
            "on": {"backgroundColor": "#ffa71e"},
            "off": {"backgroundColor": "#d5d5d5ff"}
            }
            }
            R{ %s }R \n}R, 1, 1, 1, "embedded string"
endin
</CsInstruments>
<CsScore>
i1 0 1
</CsScore>
</CsoundSynthesizer>


