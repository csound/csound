{
    "targets": [{
        "target_name": "csound",
        "sources": [
            "jscsound.cpp",
        ],
        'conditions': [
            ['OS=="linux"',
                {
                    'libraries': [
                        '-L$(CSOUND_HOME)/../cs6make -lcsound64',
                    ],
                    'include_dirs': [
                        '$(CSOUND_HOME)/include',
                    ],
                    'cflags_cc!': [
                        '-fno-exceptions',
                        '-std=c++11',
                    ],
                }
            ],
            ['OS=="win"',
                {
                    'msbuild_toolset': 'v140_xp',
                    'libraries': [#'-l$(CSOUND_HOME)/mingw64/csound64.lib',
                        '-l$(CSOUND_HOME)/msvc/csound-vs/Release/csound64.lib',
                    ],
                    'include_dirs': [
                        '$(CSOUND_HOME)/include',
                    ],
                }
            ]
        ]
    }]
}