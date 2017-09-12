{
	"patcher" : 	{
		"fileversion" : 1,
		"appversion" : 		{
			"major" : 7,
			"minor" : 0,
			"revision" : 3,
			"architecture" : "x64",
			"modernui" : 1
		}
,
		"rect" : [ 154.0, 112.0, 741.0, 429.0 ],
		"bglocked" : 0,
		"openinpresentation" : 0,
		"default_fontsize" : 12.0,
		"default_fontface" : 0,
		"default_fontname" : "Arial",
		"gridonopen" : 1,
		"gridsize" : [ 15.0, 15.0 ],
		"gridsnaponopen" : 1,
		"objectsnaponopen" : 1,
		"statusbarvisible" : 2,
		"toolbarvisible" : 1,
		"lefttoolbarpinned" : 0,
		"toptoolbarpinned" : 0,
		"righttoolbarpinned" : 0,
		"bottomtoolbarpinned" : 0,
		"toolbars_unpinned_last_save" : 0,
		"tallnewobj" : 0,
		"boxanimatetime" : 200,
		"enablehscroll" : 1,
		"enablevscroll" : 1,
		"devicewidth" : 0.0,
		"description" : "",
		"digest" : "",
		"tags" : "",
		"style" : "",
		"subpatcher_template" : "",
		"boxes" : [ 			{
				"box" : 				{
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"id" : "obj-80",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 450.0, 75.0, 117.0, 20.0 ],
					"style" : "",
					"text" : "csound~ controls"
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"hidden" : 1,
					"id" : "obj-74",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 585.0, 182.0, 67.0, 22.0 ],
					"style" : "",
					"text" : "pvar csIN"
				}

			}
, 			{
				"box" : 				{
					"bgcolor" : [ 0.0, 0.0, 0.0, 1.0 ],
					"id" : "obj-73",
					"maxclass" : "ezdac~",
					"numinlets" : 2,
					"numoutlets" : 0,
					"patching_rect" : [ 465.0, 285.0, 56.0, 56.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 450.0, 105.0, 56.0, 56.0 ],
					"style" : ""
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"id" : "obj-63",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 525.0, 105.0, 38.0, 22.0 ],
					"style" : "",
					"text" : "open"
				}

			}
, 			{
				"box" : 				{
					"fontface" : 3,
					"fontname" : "Arial",
					"fontsize" : 30.0,
					"id" : "obj-3",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 20.0, 3.0, 172.0, 40.0 ],
					"presentation" : 1,
					"presentation_linecount" : 2,
					"presentation_rect" : [ 30.0, 15.0, 135.0, 74.0 ],
					"style" : "",
					"text" : "Bass Drum",
					"textcolor" : [ 0.93, 0.93, 0.97, 1.0 ],
					"varname" : "autohelp_top_title"
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"id" : "obj-79",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 699.0, 105.0, 34.0, 20.0 ],
					"style" : "",
					"text" : "bpm"
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"id" : "obj-1",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 236.0, 305.0, 70.0, 20.0 ],
					"style" : "",
					"text" : "waveform"
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"id" : "obj-2",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 362.0, 394.0, 113.0, 20.0 ],
					"style" : "",
					"text" : "2: tanh"
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"id" : "obj-4",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 161.0, 380.0, 79.0, 20.0 ],
					"style" : "",
					"text" : "1: moog filter"
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"id" : "obj-5",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 161.0, 365.0, 78.0, 20.0 ],
					"style" : "",
					"text" : "0: no filter"
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"id" : "obj-6",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 362.0, 379.0, 114.0, 20.0 ],
					"style" : "",
					"text" : "1: sine"
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"id" : "obj-7",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 362.0, 364.0, 113.0, 20.0 ],
					"style" : "",
					"text" : "0: Bram de Jong"
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"id" : "obj-8",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 241.0, 409.0, 62.0, 20.0 ],
					"style" : "",
					"text" : "4: saw9"
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"id" : "obj-9",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 241.0, 394.0, 76.0, 20.0 ],
					"style" : "",
					"text" : "3: square9"
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"id" : "obj-10",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 241.0, 379.0, 72.0, 20.0 ],
					"style" : "",
					"text" : "2: triangle"
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"hidden" : 1,
					"id" : "obj-11",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 660.0, 60.0, 28.0, 22.0 ],
					"style" : "",
					"text" : "90."
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"hidden" : 1,
					"id" : "obj-12",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 361.0, 131.0, 37.0, 22.0 ],
					"style" : "",
					"text" : "-12."
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"hidden" : 1,
					"id" : "obj-14",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "bang" ],
					"patching_rect" : [ 360.0, 53.0, 61.0, 22.0 ],
					"style" : "",
					"text" : "loadbang"
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"hidden" : 1,
					"id" : "obj-15",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 435.0, 135.0, 67.0, 22.0 ],
					"style" : "",
					"text" : "pvar csIN"
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"hidden" : 1,
					"id" : "obj-16",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 525.0, 195.0, 21.0, 22.0 ],
					"style" : "",
					"text" : "t l",
					"varname" : "csIN"
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"hidden" : 1,
					"id" : "obj-17",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 543.0, 196.0, 34.0, 20.0 ],
					"style" : "",
					"text" : "csIN",
					"textcolor" : [ 0.972549, 0.0, 0.0, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"hidden" : 1,
					"id" : "obj-18",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 465.0, 216.0, 68.0, 22.0 ],
					"style" : "",
					"text" : "prepend c"
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"id" : "obj-19",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 361.0, 173.0, 43.0, 22.0 ],
					"style" : "",
					"text" : "dbtoa"
				}

			}
, 			{
				"box" : 				{
					"bgcolor" : [ 1.0, 1.0, 1.0, 1.0 ],
					"fontface" : 1,
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"format" : 6,
					"htricolor" : [ 0.87, 0.82, 0.24, 1.0 ],
					"id" : "obj-20",
					"maxclass" : "flonum",
					"maximum" : 12.0,
					"minimum" : -90.0,
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "bang" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 361.0, 151.0, 52.0, 22.0 ],
					"style" : "",
					"textcolor" : [ 0.815686, 0.0, 0.0, 1.0 ],
					"tricolor" : [ 0.75, 0.75, 0.75, 1.0 ],
					"triscale" : 0.9
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"id" : "obj-21",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 361.0, 194.0, 55.0, 22.0 ],
					"style" : "",
					"text" : "amp $1"
				}

			}
, 			{
				"box" : 				{
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"format" : 6,
					"htricolor" : [ 0.87, 0.82, 0.24, 1.0 ],
					"id" : "obj-22",
					"maxclass" : "flonum",
					"minimum" : 0.0,
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "bang" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 660.0, 105.0, 40.0, 22.0 ],
					"style" : "",
					"textcolor" : [ 0.0, 0.0, 0.0, 1.0 ],
					"tricolor" : [ 0.75, 0.75, 0.75, 1.0 ],
					"triscale" : 0.9
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-23",
					"maxclass" : "button",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "bang" ],
					"patching_rect" : [ 135.0, 75.0, 58.0, 58.0 ],
					"style" : ""
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"id" : "obj-24",
					"maxclass" : "newobj",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "float" ],
					"patching_rect" : [ 15.0, 172.0, 49.0, 22.0 ],
					"style" : "",
					"text" : "* 0.5"
				}

			}
, 			{
				"box" : 				{
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"htricolor" : [ 0.87, 0.82, 0.24, 1.0 ],
					"id" : "obj-25",
					"maxclass" : "number",
					"maximum" : 2,
					"minimum" : 0,
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "bang" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 362.0, 321.0, 37.0, 22.0 ],
					"style" : "",
					"textcolor" : [ 0.0, 0.0, 0.0, 1.0 ],
					"tricolor" : [ 0.75, 0.75, 0.75, 1.0 ],
					"triscale" : 0.9
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"id" : "obj-26",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 362.0, 343.0, 81.0, 22.0 ],
					"style" : "",
					"text" : "clipType $1"
				}

			}
, 			{
				"box" : 				{
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"format" : 6,
					"htricolor" : [ 0.87, 0.82, 0.24, 1.0 ],
					"id" : "obj-27",
					"maxclass" : "flonum",
					"maximum" : 64.0,
					"minimum" : 0.125,
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "bang" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 307.0, 321.0, 50.0, 22.0 ],
					"style" : "",
					"textcolor" : [ 0.0, 0.0, 0.0, 1.0 ],
					"tricolor" : [ 0.75, 0.75, 0.75, 1.0 ],
					"triscale" : 0.9
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"id" : "obj-28",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 307.0, 343.0, 53.0, 22.0 ],
					"style" : "",
					"text" : "drive $1"
				}

			}
, 			{
				"box" : 				{
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"htricolor" : [ 0.87, 0.82, 0.24, 1.0 ],
					"id" : "obj-29",
					"maxclass" : "number",
					"maximum" : 1,
					"minimum" : 0,
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "bang" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 161.0, 322.0, 37.0, 22.0 ],
					"style" : "",
					"textcolor" : [ 0.0, 0.0, 0.0, 1.0 ],
					"tricolor" : [ 0.75, 0.75, 0.75, 1.0 ],
					"triscale" : 0.9
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"id" : "obj-30",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 161.0, 344.0, 49.0, 22.0 ],
					"style" : "",
					"text" : "filt $1"
				}

			}
, 			{
				"box" : 				{
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"htricolor" : [ 0.87, 0.82, 0.24, 1.0 ],
					"id" : "obj-31",
					"maxclass" : "number",
					"maximum" : 4,
					"minimum" : 1,
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "bang" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 241.0, 322.0, 37.0, 22.0 ],
					"style" : "",
					"textcolor" : [ 0.0, 0.0, 0.0, 1.0 ],
					"tricolor" : [ 0.75, 0.75, 0.75, 1.0 ],
					"triscale" : 0.9
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"id" : "obj-32",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 241.0, 344.0, 62.0, 22.0 ],
					"style" : "",
					"text" : "wave $1"
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"hidden" : 1,
					"id" : "obj-33",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 17.0, 368.0, 61.0, 22.0 ],
					"style" : "",
					"text" : "pvar cIN"
				}

			}
, 			{
				"box" : 				{
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"format" : 6,
					"htricolor" : [ 0.87, 0.82, 0.24, 1.0 ],
					"id" : "obj-34",
					"maxclass" : "flonum",
					"maximum" : 1.0,
					"minimum" : 0.0,
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "bang" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 98.0, 322.0, 52.0, 22.0 ],
					"style" : "",
					"textcolor" : [ 0.0, 0.0, 0.0, 1.0 ],
					"tricolor" : [ 0.75, 0.75, 0.75, 1.0 ],
					"triscale" : 0.9
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"id" : "obj-35",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 17.0, 344.0, 80.0, 22.0 ],
					"style" : "",
					"text" : "filtFrqMult $1"
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"id" : "obj-36",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 98.0, 344.0, 62.0, 22.0 ],
					"style" : "",
					"text" : "filtRes $1"
				}

			}
, 			{
				"box" : 				{
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"format" : 6,
					"htricolor" : [ 0.87, 0.82, 0.24, 1.0 ],
					"id" : "obj-37",
					"maxclass" : "flonum",
					"minimum" : 0.0,
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "bang" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 17.0, 322.0, 52.0, 22.0 ],
					"style" : "",
					"textcolor" : [ 0.0, 0.0, 0.0, 1.0 ],
					"tricolor" : [ 0.75, 0.75, 0.75, 1.0 ],
					"triscale" : 0.9
				}

			}
, 			{
				"box" : 				{
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"htricolor" : [ 0.87, 0.82, 0.24, 1.0 ],
					"id" : "obj-38",
					"maxclass" : "number",
					"minimum" : 1,
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "bang" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 17.0, 243.0, 37.0, 22.0 ],
					"style" : "",
					"textcolor" : [ 0.0, 0.0, 0.0, 1.0 ],
					"tricolor" : [ 0.75, 0.75, 0.75, 1.0 ],
					"triscale" : 0.9
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"id" : "obj-39",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patcher" : 					{
						"fileversion" : 1,
						"appversion" : 						{
							"major" : 7,
							"minor" : 0,
							"revision" : 3,
							"architecture" : "x64",
							"modernui" : 1
						}
,
						"rect" : [ 10.0, 59.0, 311.0, 252.0 ],
						"bglocked" : 0,
						"openinpresentation" : 0,
						"default_fontsize" : 12.0,
						"default_fontface" : 0,
						"default_fontname" : "Arial",
						"gridonopen" : 1,
						"gridsize" : [ 15.0, 15.0 ],
						"gridsnaponopen" : 1,
						"objectsnaponopen" : 1,
						"statusbarvisible" : 2,
						"toolbarvisible" : 1,
						"lefttoolbarpinned" : 0,
						"toptoolbarpinned" : 0,
						"righttoolbarpinned" : 0,
						"bottomtoolbarpinned" : 0,
						"toolbars_unpinned_last_save" : 0,
						"tallnewobj" : 0,
						"boxanimatetime" : 200,
						"enablehscroll" : 1,
						"enablevscroll" : 1,
						"devicewidth" : 0.0,
						"description" : "",
						"digest" : "",
						"tags" : "",
						"style" : "",
						"subpatcher_template" : "",
						"boxes" : [ 							{
								"box" : 								{
									"fontname" : "Arial",
									"fontsize" : 9.0,
									"id" : "obj-1",
									"maxclass" : "newobj",
									"numinlets" : 2,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 14.0, 92.0, 74.0, 17.0 ],
									"style" : "",
									"text" : "pak ampAtk 0."
								}

							}
, 							{
								"box" : 								{
									"fontname" : "Arial",
									"fontsize" : 9.0,
									"id" : "obj-2",
									"maxclass" : "newobj",
									"numinlets" : 2,
									"numoutlets" : 1,
									"outlettype" : [ "float" ],
									"patching_rect" : [ 19.0, 65.0, 75.0, 17.0 ],
									"style" : "",
									"text" : "* 0.0001"
								}

							}
, 							{
								"box" : 								{
									"comment" : "",
									"id" : "obj-3",
									"maxclass" : "outlet",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 19.0, 116.0, 15.0, 15.0 ],
									"style" : ""
								}

							}
, 							{
								"box" : 								{
									"comment" : "",
									"id" : "obj-4",
									"maxclass" : "inlet",
									"numinlets" : 0,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 21.0, 35.0, 15.0, 15.0 ],
									"style" : ""
								}

							}
 ],
						"lines" : [ 							{
								"patchline" : 								{
									"destination" : [ "obj-3", 0 ],
									"disabled" : 0,
									"hidden" : 0,
									"source" : [ "obj-1", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-1", 1 ],
									"disabled" : 0,
									"hidden" : 0,
									"source" : [ "obj-2", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-2", 0 ],
									"disabled" : 0,
									"hidden" : 0,
									"source" : [ "obj-4", 0 ]
								}

							}
 ]
					}
,
					"patching_rect" : [ 17.0, 264.0, 40.0, 22.0 ],
					"saved_object_attributes" : 					{
						"description" : "",
						"digest" : "",
						"globalpatchername" : "",
						"style" : "",
						"tags" : ""
					}
,
					"style" : "",
					"text" : "p atk"
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"id" : "obj-41",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 200.0, 265.0, 72.0, 22.0 ],
					"style" : "",
					"text" : "ampSus $1"
				}

			}
, 			{
				"box" : 				{
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"format" : 6,
					"htricolor" : [ 0.87, 0.82, 0.24, 1.0 ],
					"id" : "obj-42",
					"maxclass" : "flonum",
					"maximum" : 1.0,
					"minimum" : 0.001,
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "bang" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 200.0, 243.0, 51.0, 22.0 ],
					"style" : "",
					"textcolor" : [ 0.0, 0.0, 0.0, 1.0 ],
					"tricolor" : [ 0.75, 0.75, 0.75, 1.0 ],
					"triscale" : 0.9
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"id" : "obj-43",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 146.0, 194.0, 65.0, 22.0 ],
					"style" : "",
					"text" : "frqSus $1"
				}

			}
, 			{
				"box" : 				{
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"format" : 6,
					"htricolor" : [ 0.87, 0.82, 0.24, 1.0 ],
					"id" : "obj-44",
					"maxclass" : "flonum",
					"maximum" : 1.0,
					"minimum" : 0.001,
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "bang" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 146.0, 172.0, 52.0, 22.0 ],
					"style" : "",
					"textcolor" : [ 0.0, 0.0, 0.0, 1.0 ],
					"tricolor" : [ 0.75, 0.75, 0.75, 1.0 ],
					"triscale" : 0.9
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"hidden" : 1,
					"id" : "obj-45",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 15.0, 218.0, 61.0, 22.0 ],
					"style" : "",
					"text" : "pvar cIN"
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"hidden" : 1,
					"id" : "obj-46",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 17.0, 290.0, 61.0, 22.0 ],
					"style" : "",
					"text" : "pvar cIN"
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"hidden" : 1,
					"id" : "obj-47",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 465.0, 195.0, 21.0, 22.0 ],
					"style" : "",
					"text" : "t l",
					"varname" : "cIN"
				}

			}
, 			{
				"box" : 				{
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"format" : 6,
					"htricolor" : [ 0.87, 0.82, 0.24, 1.0 ],
					"id" : "obj-48",
					"maxclass" : "flonum",
					"minimum" : 0.02,
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "bang" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 130.0, 243.0, 52.0, 22.0 ],
					"style" : "",
					"textcolor" : [ 0.0, 0.0, 0.0, 1.0 ],
					"tricolor" : [ 0.75, 0.75, 0.75, 1.0 ],
					"triscale" : 0.9
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"id" : "obj-49",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 58.0, 265.0, 72.0, 22.0 ],
					"style" : "",
					"text" : "ampDec $1"
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"id" : "obj-50",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 130.0, 265.0, 69.0, 22.0 ],
					"style" : "",
					"text" : "ampRel $1"
				}

			}
, 			{
				"box" : 				{
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"format" : 6,
					"htricolor" : [ 0.87, 0.82, 0.24, 1.0 ],
					"id" : "obj-51",
					"maxclass" : "flonum",
					"minimum" : 0.0,
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "bang" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 58.0, 243.0, 52.0, 22.0 ],
					"style" : "",
					"textcolor" : [ 0.0, 0.0, 0.0, 1.0 ],
					"tricolor" : [ 0.75, 0.75, 0.75, 1.0 ],
					"triscale" : 0.9
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"id" : "obj-52",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 283.0, 194.0, 77.0, 22.0 ],
					"style" : "",
					"text" : "frqRange $1"
				}

			}
, 			{
				"box" : 				{
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"format" : 6,
					"htricolor" : [ 0.87, 0.82, 0.24, 1.0 ],
					"id" : "obj-53",
					"maxclass" : "flonum",
					"maximum" : 1.0,
					"minimum" : 0.0,
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "bang" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 283.0, 172.0, 52.0, 22.0 ],
					"style" : "",
					"textcolor" : [ 0.0, 0.0, 0.0, 1.0 ],
					"tricolor" : [ 0.75, 0.75, 0.75, 1.0 ],
					"triscale" : 0.9
				}

			}
, 			{
				"box" : 				{
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"format" : 6,
					"htricolor" : [ 0.87, 0.82, 0.24, 1.0 ],
					"id" : "obj-54",
					"maxclass" : "flonum",
					"minimum" : 0.0,
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "bang" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 81.0, 172.0, 52.0, 22.0 ],
					"style" : "",
					"textcolor" : [ 0.0, 0.0, 0.0, 1.0 ],
					"tricolor" : [ 0.75, 0.75, 0.75, 1.0 ],
					"triscale" : 0.9
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"id" : "obj-55",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 585.0, 75.0, 131.0, 20.0 ],
					"style" : "",
					"text" : "metronome controls"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-56",
					"maxclass" : "button",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "bang" ],
					"patching_rect" : [ 585.0, 105.0, 21.0, 21.0 ],
					"style" : ""
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"id" : "obj-58",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 615.0, 105.0, 34.0, 22.0 ],
					"style" : "",
					"text" : "stop"
				}

			}
, 			{
				"box" : 				{
					"color" : [ 1.0, 0.741176, 0.611765, 1.0 ],
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"hidden" : 1,
					"id" : "obj-59",
					"maxclass" : "newobj",
					"numinlets" : 3,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patcher" : 					{
						"fileversion" : 1,
						"appversion" : 						{
							"major" : 7,
							"minor" : 0,
							"revision" : 3,
							"architecture" : "x64",
							"modernui" : 1
						}
,
						"rect" : [ 416.0, 590.0, 508.0, 357.0 ],
						"bglocked" : 0,
						"openinpresentation" : 0,
						"default_fontsize" : 12.0,
						"default_fontface" : 0,
						"default_fontname" : "Arial",
						"gridonopen" : 1,
						"gridsize" : [ 15.0, 15.0 ],
						"gridsnaponopen" : 1,
						"objectsnaponopen" : 1,
						"statusbarvisible" : 2,
						"toolbarvisible" : 1,
						"lefttoolbarpinned" : 0,
						"toptoolbarpinned" : 0,
						"righttoolbarpinned" : 0,
						"bottomtoolbarpinned" : 0,
						"toolbars_unpinned_last_save" : 0,
						"tallnewobj" : 0,
						"boxanimatetime" : 200,
						"enablehscroll" : 1,
						"enablevscroll" : 1,
						"devicewidth" : 0.0,
						"description" : "",
						"digest" : "",
						"tags" : "",
						"style" : "",
						"subpatcher_template" : "",
						"boxes" : [ 							{
								"box" : 								{
									"comment" : "",
									"id" : "obj-1",
									"maxclass" : "inlet",
									"numinlets" : 0,
									"numoutlets" : 1,
									"outlettype" : [ "bang" ],
									"patching_rect" : [ 173.0, 23.0, 15.0, 15.0 ],
									"style" : ""
								}

							}
, 							{
								"box" : 								{
									"comment" : "",
									"id" : "obj-2",
									"maxclass" : "inlet",
									"numinlets" : 0,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 153.0, 21.0, 15.0, 15.0 ],
									"style" : ""
								}

							}
, 							{
								"box" : 								{
									"comment" : "BPM",
									"id" : "obj-3",
									"maxclass" : "inlet",
									"numinlets" : 0,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 199.0, 23.0, 15.0, 15.0 ],
									"style" : ""
								}

							}
, 							{
								"box" : 								{
									"fontname" : "Arial",
									"fontsize" : 9.0,
									"id" : "obj-4",
									"maxclass" : "newobj",
									"numinlets" : 3,
									"numoutlets" : 2,
									"outlettype" : [ "float", "float" ],
									"patching_rect" : [ 153.0, 188.0, 88.0, 17.0 ],
									"style" : "",
									"text" : "makenote 64 500"
								}

							}
, 							{
								"box" : 								{
									"fontname" : "Arial",
									"fontsize" : 9.0,
									"id" : "obj-5",
									"maxclass" : "newobj",
									"numinlets" : 2,
									"numoutlets" : 1,
									"outlettype" : [ "int" ],
									"patching_rect" : [ 153.0, 166.0, 31.0, 17.0 ],
									"style" : "",
									"text" : "+ 30"
								}

							}
, 							{
								"box" : 								{
									"fontname" : "Arial",
									"fontsize" : 9.0,
									"id" : "obj-6",
									"maxclass" : "newobj",
									"numinlets" : 2,
									"numoutlets" : 1,
									"outlettype" : [ "int" ],
									"patching_rect" : [ 153.0, 146.0, 55.0, 17.0 ],
									"style" : "",
									"text" : "random 60"
								}

							}
, 							{
								"box" : 								{
									"fontname" : "Arial",
									"fontsize" : 9.0,
									"id" : "obj-7",
									"maxclass" : "newobj",
									"numinlets" : 2,
									"numoutlets" : 1,
									"outlettype" : [ "float" ],
									"patching_rect" : [ 199.0, 64.0, 27.0, 17.0 ],
									"style" : "",
									"text" : "* 1."
								}

							}
, 							{
								"box" : 								{
									"fontname" : "Arial",
									"fontsize" : 9.0,
									"id" : "obj-8",
									"maxclass" : "newobj",
									"numinlets" : 2,
									"numoutlets" : 1,
									"outlettype" : [ "float" ],
									"patching_rect" : [ 199.0, 84.0, 52.0, 17.0 ],
									"style" : "",
									"text" : "* 60000."
								}

							}
, 							{
								"box" : 								{
									"fontname" : "Arial",
									"fontsize" : 9.0,
									"id" : "obj-9",
									"maxclass" : "newobj",
									"numinlets" : 2,
									"numoutlets" : 1,
									"outlettype" : [ "float" ],
									"patching_rect" : [ 199.0, 42.0, 44.0, 17.0 ],
									"style" : "",
									"text" : "pow -1."
								}

							}
, 							{
								"box" : 								{
									"color" : [ 1.0, 0.890196, 0.090196, 1.0 ],
									"fontname" : "Arial",
									"fontsize" : 9.0,
									"id" : "obj-10",
									"maxclass" : "newobj",
									"numinlets" : 2,
									"numoutlets" : 1,
									"outlettype" : [ "bang" ],
									"patching_rect" : [ 153.0, 125.0, 56.0, 17.0 ],
									"style" : "",
									"text" : "metro 500"
								}

							}
, 							{
								"box" : 								{
									"comment" : "",
									"id" : "obj-11",
									"maxclass" : "outlet",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 153.0, 267.0, 15.0, 15.0 ],
									"style" : ""
								}

							}
, 							{
								"box" : 								{
									"fontname" : "Arial",
									"fontsize" : 9.0,
									"id" : "obj-12",
									"maxclass" : "newobj",
									"numinlets" : 2,
									"numoutlets" : 1,
									"outlettype" : [ "" ],
									"patching_rect" : [ 153.0, 211.0, 47.0, 17.0 ],
									"style" : "",
									"text" : "pack 0 0"
								}

							}
, 							{
								"box" : 								{
									"fontname" : "Arial",
									"fontsize" : 9.0,
									"id" : "obj-13",
									"maxclass" : "comment",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 215.0, 23.0, 32.0, 17.0 ],
									"style" : "",
									"text" : "BPM"
								}

							}
, 							{
								"box" : 								{
									"fontname" : "Arial",
									"fontsize" : 9.0,
									"id" : "obj-14",
									"maxclass" : "comment",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 78.0, 22.0, 71.0, 17.0 ],
									"style" : "",
									"text" : "metro control"
								}

							}
 ],
						"lines" : [ 							{
								"patchline" : 								{
									"destination" : [ "obj-6", 0 ],
									"disabled" : 0,
									"hidden" : 0,
									"source" : [ "obj-1", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-6", 0 ],
									"disabled" : 0,
									"hidden" : 0,
									"source" : [ "obj-10", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-11", 0 ],
									"disabled" : 0,
									"hidden" : 0,
									"source" : [ "obj-12", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-10", 0 ],
									"disabled" : 0,
									"hidden" : 0,
									"source" : [ "obj-2", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-9", 0 ],
									"disabled" : 0,
									"hidden" : 0,
									"source" : [ "obj-3", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-12", 1 ],
									"disabled" : 0,
									"hidden" : 0,
									"midpoints" : [ 231.5, 208.0, 190.5, 208.0 ],
									"source" : [ "obj-4", 1 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-12", 0 ],
									"disabled" : 0,
									"hidden" : 0,
									"source" : [ "obj-4", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-4", 0 ],
									"disabled" : 0,
									"hidden" : 0,
									"source" : [ "obj-5", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-5", 0 ],
									"disabled" : 0,
									"hidden" : 0,
									"source" : [ "obj-6", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-8", 0 ],
									"disabled" : 0,
									"hidden" : 0,
									"source" : [ "obj-7", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-10", 1 ],
									"disabled" : 0,
									"hidden" : 0,
									"source" : [ "obj-8", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-4", 2 ],
									"disabled" : 0,
									"hidden" : 0,
									"midpoints" : [ 208.5, 109.0, 231.5, 109.0 ],
									"source" : [ "obj-8", 0 ]
								}

							}
, 							{
								"patchline" : 								{
									"destination" : [ "obj-7", 0 ],
									"disabled" : 0,
									"hidden" : 0,
									"source" : [ "obj-9", 0 ]
								}

							}
 ]
					}
,
					"patching_rect" : [ 585.0, 135.0, 76.0, 22.0 ],
					"saved_object_attributes" : 					{
						"description" : "",
						"digest" : "",
						"globalpatchername" : "",
						"style" : "",
						"tags" : ""
					}
,
					"style" : "",
					"text" : "p MIDI Input"
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"hidden" : 1,
					"id" : "obj-60",
					"maxclass" : "newobj",
					"numinlets" : 7,
					"numoutlets" : 1,
					"outlettype" : [ "int" ],
					"patching_rect" : [ 585.0, 158.0, 119.0, 22.0 ],
					"style" : "",
					"text" : "midiformat 1"
				}

			}
, 			{
				"box" : 				{
					"bgcolor" : [ 0.227451, 0.227451, 0.227451, 1.0 ],
					"calccount" : 16,
					"fgcolor" : [ 0.403, 1.0, 0.2, 1.0 ],
					"gridcolor" : [ 0.33, 0.33, 0.33, 1.0 ],
					"id" : "obj-61",
					"maxclass" : "scope~",
					"numinlets" : 2,
					"numoutlets" : 0,
					"patching_rect" : [ 525.0, 285.0, 125.0, 113.0 ],
					"style" : ""
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"hidden" : 1,
					"id" : "obj-62",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "bang", "bang" ],
					"patching_rect" : [ 360.0, 75.0, 33.0, 22.0 ],
					"style" : "",
					"text" : "t b b"
				}

			}
, 			{
				"box" : 				{
					"id" : "obj-64",
					"maxclass" : "button",
					"numinlets" : 1,
					"numoutlets" : 1,
					"outlettype" : [ "bang" ],
					"patching_rect" : [ 450.0, 105.0, 21.0, 21.0 ],
					"style" : ""
				}

			}
, 			{
				"box" : 				{
					"active1" : [ 0.0, 0.203922, 1.0, 1.0 ],
					"bgcolor" : [ 0.94, 0.94, 0.94, 1.0 ],
					"bubblesize" : 10,
					"emptycolor" : [ 0.83, 0.83, 0.83, 1.0 ],
					"id" : "obj-65",
					"maxclass" : "preset",
					"numinlets" : 1,
					"numoutlets" : 4,
					"outlettype" : [ "preset", "int", "preset", "int" ],
					"patching_rect" : [ 15.0, 75.0, 104.0, 57.0 ],
					"preset_data" : [ 						{
							"number" : 1,
							"data" : [ 5, "obj-72", "flonum", "float", 0.02, 5, "obj-70", "flonum", "float", 0.12, 5, "obj-54", "flonum", "float", 0.06, 5, "obj-53", "flonum", "float", 0.3, 5, "obj-51", "flonum", "float", 0.06, 5, "obj-48", "flonum", "float", 0.26, 5, "obj-44", "flonum", "float", 0.32, 5, "obj-42", "flonum", "float", 0.1, 5, "obj-38", "number", "int", 1, 5, "obj-37", "flonum", "float", 3.22, 5, "obj-34", "flonum", "float", 0.76, 5, "obj-31", "number", "int", 2, 5, "obj-29", "number", "int", 1, 5, "obj-27", "flonum", "float", 19.0, 5, "obj-25", "number", "int", 2 ]
						}
, 						{
							"number" : 2,
							"data" : [ 5, "obj-72", "flonum", "float", 0.02, 5, "obj-70", "flonum", "float", 0.01, 5, "obj-54", "flonum", "float", 0.43, 5, "obj-53", "flonum", "float", 0.69, 5, "obj-51", "flonum", "float", 0.06, 5, "obj-48", "flonum", "float", 0.28, 5, "obj-44", "flonum", "float", 0.32, 5, "obj-42", "flonum", "float", 0.1, 5, "obj-38", "number", "int", 1, 5, "obj-37", "flonum", "float", 3.86, 5, "obj-34", "flonum", "float", 0.12, 5, "obj-31", "number", "int", 2, 5, "obj-29", "number", "int", 1, 5, "obj-27", "flonum", "float", 4.0, 5, "obj-25", "number", "int", 1 ]
						}
, 						{
							"number" : 3,
							"data" : [ 5, "obj-72", "flonum", "float", 0.02, 5, "obj-70", "flonum", "float", 0.1, 5, "obj-54", "flonum", "float", 0.21, 5, "obj-53", "flonum", "float", 0.69, 5, "obj-51", "flonum", "float", 0.06, 5, "obj-48", "flonum", "float", 0.33, 5, "obj-44", "flonum", "float", 0.32, 5, "obj-42", "flonum", "float", 0.1, 5, "obj-38", "number", "int", 1, 5, "obj-37", "flonum", "float", 1.31, 5, "obj-34", "flonum", "float", 0.12, 5, "obj-31", "number", "int", 2, 5, "obj-29", "number", "int", 1, 5, "obj-27", "flonum", "float", 3.0, 5, "obj-25", "number", "int", 1 ]
						}
, 						{
							"number" : 4,
							"data" : [ 5, "obj-72", "flonum", "float", 0.03, 5, "obj-70", "flonum", "float", 0.15, 5, "obj-54", "flonum", "float", 0.6, 5, "obj-53", "flonum", "float", 0.57, 5, "obj-51", "flonum", "float", 0.12, 5, "obj-48", "flonum", "float", 1.77, 5, "obj-44", "flonum", "float", 0.3, 5, "obj-42", "flonum", "float", 0.27, 5, "obj-38", "number", "int", 1, 5, "obj-37", "flonum", "float", 1.74, 5, "obj-34", "flonum", "float", 0.97, 5, "obj-31", "number", "int", 1, 5, "obj-29", "number", "int", 1, 5, "obj-27", "flonum", "float", 6.08, 5, "obj-25", "number", "int", 1 ]
						}
, 						{
							"number" : 5,
							"data" : [ 5, "obj-72", "flonum", "float", 0.01, 5, "obj-70", "flonum", "float", 0.01, 5, "obj-54", "flonum", "float", 0.43, 5, "obj-53", "flonum", "float", 0.68, 5, "obj-51", "flonum", "float", 0.06, 5, "obj-48", "flonum", "float", 4.36, 5, "obj-44", "flonum", "float", 0.32, 5, "obj-42", "flonum", "float", 0.35, 5, "obj-38", "number", "int", 1, 5, "obj-37", "flonum", "float", 3.33, 5, "obj-34", "flonum", "float", 0.05, 5, "obj-31", "number", "int", 3, 5, "obj-29", "number", "int", 1, 5, "obj-27", "flonum", "float", 1.0, 5, "obj-25", "number", "int", 1 ]
						}
, 						{
							"number" : 7,
							"data" : [ 5, "obj-72", "flonum", "float", 0.07, 5, "obj-70", "flonum", "float", 0.06, 5, "obj-54", "flonum", "float", 2.31, 5, "obj-53", "flonum", "float", 0.71, 5, "obj-51", "flonum", "float", 0.05, 5, "obj-48", "flonum", "float", 2.47, 5, "obj-44", "flonum", "float", 0.05, 5, "obj-42", "flonum", "float", 0.02, 5, "obj-38", "number", "int", 1, 5, "obj-37", "flonum", "float", 0.79, 5, "obj-34", "flonum", "float", 0.69, 5, "obj-31", "number", "int", 1, 5, "obj-29", "number", "int", 1, 5, "obj-27", "flonum", "float", 20.0, 5, "obj-25", "number", "int", 1 ]
						}
, 						{
							"number" : 8,
							"data" : [ 5, "obj-72", "flonum", "float", 0.09, 5, "obj-70", "flonum", "float", 0.06, 5, "obj-54", "flonum", "float", 0.54, 5, "obj-53", "flonum", "float", 0.52, 5, "obj-51", "flonum", "float", 0.05, 5, "obj-48", "flonum", "float", 0.21, 5, "obj-44", "flonum", "float", 0.02, 5, "obj-42", "flonum", "float", 0.27, 5, "obj-38", "number", "int", 1, 5, "obj-37", "flonum", "float", 2.82, 5, "obj-34", "flonum", "float", 0.4, 5, "obj-31", "number", "int", 2, 5, "obj-29", "number", "int", 1, 5, "obj-27", "flonum", "float", 20.0, 5, "obj-25", "number", "int", 1 ]
						}
, 						{
							"number" : 9,
							"data" : [ 5, "obj-72", "flonum", "float", 0.02, 5, "obj-70", "flonum", "float", 0.01, 5, "obj-54", "flonum", "float", 0.43, 5, "obj-53", "flonum", "float", 0.69, 5, "obj-51", "flonum", "float", 0.06, 5, "obj-48", "flonum", "float", 1.11, 5, "obj-44", "flonum", "float", 0.32, 5, "obj-42", "flonum", "float", 0.1, 5, "obj-38", "number", "int", 1, 5, "obj-37", "flonum", "float", 4.97, 5, "obj-34", "flonum", "float", 0.89, 5, "obj-31", "number", "int", 2, 5, "obj-29", "number", "int", 1, 5, "obj-27", "flonum", "float", 64.0, 5, "obj-25", "number", "int", 1 ]
						}
, 						{
							"number" : 11,
							"data" : [ 5, "obj-72", "flonum", "float", 0.02, 5, "obj-70", "flonum", "float", 0.1, 5, "obj-54", "flonum", "float", 0.21, 5, "obj-53", "flonum", "float", 0.82, 5, "obj-51", "flonum", "float", 0.06, 5, "obj-48", "flonum", "float", 0.5, 5, "obj-44", "flonum", "float", 0.23, 5, "obj-42", "flonum", "float", 0.12, 5, "obj-38", "number", "int", 1, 5, "obj-37", "flonum", "float", 1.39, 5, "obj-34", "flonum", "float", 0.16, 5, "obj-31", "number", "int", 2, 5, "obj-29", "number", "int", 1, 5, "obj-27", "flonum", "float", 3.0, 5, "obj-25", "number", "int", 1 ]
						}
, 						{
							"number" : 12,
							"data" : [ 5, "obj-72", "flonum", "float", 0.03, 5, "obj-70", "flonum", "float", 0.1, 5, "obj-54", "flonum", "float", 0.6, 5, "obj-53", "flonum", "float", 0.57, 5, "obj-51", "flonum", "float", 0.12, 5, "obj-48", "flonum", "float", 1.77, 5, "obj-44", "flonum", "float", 0.3, 5, "obj-42", "flonum", "float", 0.27, 5, "obj-38", "number", "int", 1, 5, "obj-37", "flonum", "float", 1.74, 5, "obj-34", "flonum", "float", 0.97, 5, "obj-31", "number", "int", 1, 5, "obj-29", "number", "int", 1, 5, "obj-27", "flonum", "float", 6.08, 5, "obj-25", "number", "int", 1 ]
						}
, 						{
							"number" : 15,
							"data" : [ 5, "obj-72", "flonum", "float", 0.02, 5, "obj-70", "flonum", "float", 0.01, 5, "obj-54", "flonum", "float", 0.43, 5, "obj-53", "flonum", "float", 0.69, 5, "obj-51", "flonum", "float", 0.06, 5, "obj-48", "flonum", "float", 1.11, 5, "obj-44", "flonum", "float", 0.32, 5, "obj-42", "flonum", "float", 0.1, 5, "obj-38", "number", "int", 1, 5, "obj-37", "flonum", "float", 6.57, 5, "obj-34", "flonum", "float", 0.89, 5, "obj-31", "number", "int", 2, 5, "obj-29", "number", "int", 1, 5, "obj-27", "flonum", "float", 64.0, 5, "obj-25", "number", "int", 1 ]
						}
, 						{
							"number" : 16,
							"data" : [ 5, "obj-72", "flonum", "float", 0.09, 5, "obj-70", "flonum", "float", 0.06, 5, "obj-54", "flonum", "float", 0.54, 5, "obj-53", "flonum", "float", 0.52, 5, "obj-51", "flonum", "float", 0.05, 5, "obj-48", "flonum", "float", 0.21, 5, "obj-44", "flonum", "float", 0.02, 5, "obj-42", "flonum", "float", 0.13, 5, "obj-38", "number", "int", 1, 5, "obj-37", "flonum", "float", 1.57, 5, "obj-34", "flonum", "float", 0.71, 5, "obj-31", "number", "int", 2, 5, "obj-29", "number", "int", 1, 5, "obj-27", "flonum", "float", 20.0, 5, "obj-25", "number", "int", 1 ]
						}
, 						{
							"number" : 18,
							"data" : [ 5, "obj-72", "flonum", "float", 0.02, 5, "obj-70", "flonum", "float", 0.01, 5, "obj-54", "flonum", "float", 0.43, 5, "obj-53", "flonum", "float", 0.69, 5, "obj-51", "flonum", "float", 0.06, 5, "obj-48", "flonum", "float", 1.11, 5, "obj-44", "flonum", "float", 0.32, 5, "obj-42", "flonum", "float", 0.1, 5, "obj-38", "number", "int", 1, 5, "obj-37", "flonum", "float", 3.86, 5, "obj-34", "flonum", "float", 0.12, 5, "obj-31", "number", "int", 2, 5, "obj-29", "number", "int", 1, 5, "obj-27", "flonum", "float", 4.0, 5, "obj-25", "number", "int", 1 ]
						}
, 						{
							"number" : 19,
							"data" : [ 5, "obj-72", "flonum", "float", 0.02, 5, "obj-70", "flonum", "float", 0.1, 5, "obj-54", "flonum", "float", 0.21, 5, "obj-53", "flonum", "float", 0.82, 5, "obj-51", "flonum", "float", 0.06, 5, "obj-48", "flonum", "float", 0.8, 5, "obj-44", "flonum", "float", 0.23, 5, "obj-42", "flonum", "float", 0.14, 5, "obj-38", "number", "int", 1, 5, "obj-37", "flonum", "float", 1.68, 5, "obj-34", "flonum", "float", 0.17, 5, "obj-31", "number", "int", 2, 5, "obj-29", "number", "int", 1, 5, "obj-27", "flonum", "float", 5.15, 5, "obj-25", "number", "int", 1 ]
						}
, 						{
							"number" : 20,
							"data" : [ 5, "obj-72", "flonum", "float", 0.03, 5, "obj-70", "flonum", "float", 0.04, 5, "obj-54", "flonum", "float", 0.6, 5, "obj-53", "flonum", "float", 0.57, 5, "obj-51", "flonum", "float", 0.12, 5, "obj-48", "flonum", "float", 1.77, 5, "obj-44", "flonum", "float", 0.3, 5, "obj-42", "flonum", "float", 0.27, 5, "obj-38", "number", "int", 1, 5, "obj-37", "flonum", "float", 1.74, 5, "obj-34", "flonum", "float", 0.97, 5, "obj-31", "number", "int", 1, 5, "obj-29", "number", "int", 1, 5, "obj-27", "flonum", "float", 6.08, 5, "obj-25", "number", "int", 1 ]
						}
, 						{
							"number" : 24,
							"data" : [ 5, "obj-72", "flonum", "float", 0.07, 5, "obj-70", "flonum", "float", 0.06, 5, "obj-54", "flonum", "float", 0.54, 5, "obj-53", "flonum", "float", 0.71, 5, "obj-51", "flonum", "float", 0.05, 5, "obj-48", "flonum", "float", 0.21, 5, "obj-44", "flonum", "float", 0.02, 5, "obj-42", "flonum", "float", 0.13, 5, "obj-38", "number", "int", 1, 5, "obj-37", "flonum", "float", 1.57, 5, "obj-34", "flonum", "float", 0.71, 5, "obj-31", "number", "int", 2, 5, "obj-29", "number", "int", 1, 5, "obj-27", "flonum", "float", 20.0, 5, "obj-25", "number", "int", 1 ]
						}
, 						{
							"number" : 25,
							"data" : [ 5, "obj-72", "flonum", "float", 0.02, 5, "obj-70", "flonum", "float", 0.01, 5, "obj-54", "flonum", "float", 1.76, 5, "obj-53", "flonum", "float", 0.69, 5, "obj-51", "flonum", "float", 0.06, 5, "obj-48", "flonum", "float", 1.11, 5, "obj-44", "flonum", "float", 0.32, 5, "obj-42", "flonum", "float", 0.1, 5, "obj-38", "number", "int", 1, 5, "obj-37", "flonum", "float", 3.86, 5, "obj-34", "flonum", "float", 0.89, 5, "obj-31", "number", "int", 2, 5, "obj-29", "number", "int", 1, 5, "obj-27", "flonum", "float", 64.0, 5, "obj-25", "number", "int", 1 ]
						}
, 						{
							"number" : 26,
							"data" : [ 5, "obj-72", "flonum", "float", 0.02, 5, "obj-70", "flonum", "float", 0.01, 5, "obj-54", "flonum", "float", 1.76, 5, "obj-53", "flonum", "float", 0.69, 5, "obj-51", "flonum", "float", 0.06, 5, "obj-48", "flonum", "float", 1.11, 5, "obj-44", "flonum", "float", 0.04, 5, "obj-42", "flonum", "float", 0.1, 5, "obj-38", "number", "int", 1, 5, "obj-37", "flonum", "float", 3.86, 5, "obj-34", "flonum", "float", 0.89, 5, "obj-31", "number", "int", 2, 5, "obj-29", "number", "int", 1, 5, "obj-27", "flonum", "float", 64.0, 5, "obj-25", "number", "int", 1 ]
						}
, 						{
							"number" : 27,
							"data" : [ 5, "obj-72", "flonum", "float", 0.02, 5, "obj-70", "flonum", "float", 0.05, 5, "obj-54", "flonum", "float", 1.65, 5, "obj-53", "flonum", "float", 0.69, 5, "obj-51", "flonum", "float", 0.06, 5, "obj-48", "flonum", "float", 1.11, 5, "obj-44", "flonum", "float", 0.04, 5, "obj-42", "flonum", "float", 0.1, 5, "obj-38", "number", "int", 1, 5, "obj-37", "flonum", "float", 3.86, 5, "obj-34", "flonum", "float", 0.89, 5, "obj-31", "number", "int", 2, 5, "obj-29", "number", "int", 1, 5, "obj-27", "flonum", "float", 64.0, 5, "obj-25", "number", "int", 1 ]
						}
, 						{
							"number" : 29,
							"data" : [ 5, "obj-72", "flonum", "float", 0.02, 5, "obj-70", "flonum", "float", 0.01, 5, "obj-54", "flonum", "float", 0.63, 5, "obj-53", "flonum", "float", 0.69, 5, "obj-51", "flonum", "float", 0.06, 5, "obj-48", "flonum", "float", 0.63, 5, "obj-44", "flonum", "float", 0.37, 5, "obj-42", "flonum", "float", 0.1, 5, "obj-38", "number", "int", 1, 5, "obj-37", "flonum", "float", 3.86, 5, "obj-34", "flonum", "float", 0.89, 5, "obj-31", "number", "int", 2, 5, "obj-29", "number", "int", 1, 5, "obj-27", "flonum", "float", 64.0, 5, "obj-25", "number", "int", 1 ]
						}
, 						{
							"number" : 30,
							"data" : [ 5, "obj-72", "flonum", "float", 0.02, 5, "obj-70", "flonum", "float", 0.01, 5, "obj-54", "flonum", "float", 0.43, 5, "obj-53", "flonum", "float", 0.69, 5, "obj-51", "flonum", "float", 0.06, 5, "obj-48", "flonum", "float", 1.11, 5, "obj-44", "flonum", "float", 0.32, 5, "obj-42", "flonum", "float", 0.1, 5, "obj-38", "number", "int", 1, 5, "obj-37", "flonum", "float", 6.57, 5, "obj-34", "flonum", "float", 0.89, 5, "obj-31", "number", "int", 2, 5, "obj-29", "number", "int", 1, 5, "obj-27", "flonum", "float", 64.0, 5, "obj-25", "number", "int", 1 ]
						}
, 						{
							"number" : 31,
							"data" : [ 5, "obj-72", "flonum", "float", 0.02, 5, "obj-70", "flonum", "float", 0.01, 5, "obj-54", "flonum", "float", 0.74, 5, "obj-53", "flonum", "float", 0.69, 5, "obj-51", "flonum", "float", 0.06, 5, "obj-48", "flonum", "float", 0.63, 5, "obj-44", "flonum", "float", 0.33, 5, "obj-42", "flonum", "float", 0.1, 5, "obj-38", "number", "int", 1, 5, "obj-37", "flonum", "float", 3.86, 5, "obj-34", "flonum", "float", 0.99, 5, "obj-31", "number", "int", 2, 5, "obj-29", "number", "int", 1, 5, "obj-27", "flonum", "float", 64.0, 5, "obj-25", "number", "int", 1 ]
						}
, 						{
							"number" : 32,
							"data" : [ 5, "obj-72", "flonum", "float", 0.03, 5, "obj-70", "flonum", "float", 0.23, 5, "obj-54", "flonum", "float", 0.6, 5, "obj-53", "flonum", "float", 0.57, 5, "obj-51", "flonum", "float", 0.13, 5, "obj-48", "flonum", "float", 4.01, 5, "obj-44", "flonum", "float", 0.001, 5, "obj-42", "flonum", "float", 0.001, 5, "obj-38", "number", "int", 1, 5, "obj-37", "flonum", "float", 12.0, 5, "obj-34", "flonum", "float", 0.97, 5, "obj-31", "number", "int", 1, 5, "obj-29", "number", "int", 1, 5, "obj-27", "flonum", "float", 6.08, 5, "obj-25", "number", "int", 1 ]
						}
 ],
					"stored1" : [ 0.412, 0.412, 0.412, 1.0 ],
					"style" : "",
					"textcolor" : [ 0.5, 0.5, 0.5, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"hidden" : 1,
					"id" : "obj-66",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 15.0, 53.0, 18.0, 22.0 ],
					"style" : "",
					"text" : "3"
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"id" : "obj-67",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 15.0, 194.0, 64.0, 22.0 ],
					"style" : "",
					"text" : "frqDec $1"
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"id" : "obj-68",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 81.0, 194.0, 62.0, 22.0 ],
					"style" : "",
					"text" : "frqRel $1"
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"id" : "obj-69",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 213.0, 194.0, 69.0, 22.0 ],
					"style" : "",
					"text" : "frqBase $1"
				}

			}
, 			{
				"box" : 				{
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"format" : 6,
					"htricolor" : [ 0.87, 0.82, 0.24, 1.0 ],
					"id" : "obj-70",
					"maxclass" : "flonum",
					"maximum" : 1.0,
					"minimum" : 0.0,
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "bang" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 213.0, 172.0, 51.0, 22.0 ],
					"style" : "",
					"textcolor" : [ 0.0, 0.0, 0.0, 1.0 ],
					"tricolor" : [ 0.75, 0.75, 0.75, 1.0 ],
					"triscale" : 0.9
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"id" : "obj-71",
					"maxclass" : "message",
					"numinlets" : 2,
					"numoutlets" : 1,
					"outlettype" : [ "" ],
					"patching_rect" : [ 480.0, 105.0, 35.0, 22.0 ],
					"style" : "",
					"text" : "stop"
				}

			}
, 			{
				"box" : 				{
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"format" : 6,
					"htricolor" : [ 0.87, 0.82, 0.24, 1.0 ],
					"id" : "obj-72",
					"maxclass" : "flonum",
					"minimum" : 0.0,
					"numinlets" : 1,
					"numoutlets" : 2,
					"outlettype" : [ "", "bang" ],
					"parameter_enable" : 0,
					"patching_rect" : [ 15.0, 150.0, 52.0, 22.0 ],
					"style" : "",
					"textcolor" : [ 0.0, 0.0, 0.0, 1.0 ],
					"tricolor" : [ 0.75, 0.75, 0.75, 1.0 ],
					"triscale" : 0.9
				}

			}
, 			{
				"box" : 				{
					"fontface" : 0,
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"id" : "obj-75",
					"maxclass" : "newobj",
					"numinlets" : 1,
					"numoutlets" : 5,
					"outlettype" : [ "signal", "list", "int", "bang", "bang" ],
					"patching_rect" : [ 465.0, 255.0, 202.0, 22.0 ],
					"saved_object_attributes" : 					{
						"args" : "-m0 bassdrum.csd",
						"autostart" : 0,
						"bypass" : 0,
						"input" : 1,
						"interval" : 10,
						"matchsr" : 1,
						"message" : 1,
						"output" : 1,
						"overdrive" : 0
					}
,
					"style" : "",
					"text" : "csound~ \"-m0 bassdrum.csd\" @io 1"
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"hidden" : 1,
					"id" : "obj-76",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 483.0, 196.0, 32.0, 20.0 ],
					"style" : "",
					"text" : "cIN",
					"textcolor" : [ 0.972549, 0.0, 0.0, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"id" : "obj-77",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 144.0, 56.0, 47.0, 20.0 ],
					"style" : "",
					"text" : "Hit Me!"
				}

			}
, 			{
				"box" : 				{
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"id" : "obj-78",
					"maxclass" : "comment",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 241.0, 364.0, 53.0, 20.0 ],
					"style" : "",
					"text" : "1: sine"
				}

			}
, 			{
				"box" : 				{
					"angle" : 0.0,
					"background" : 1,
					"grad1" : [ 0.117647, 0.117647, 0.117647, 1.0 ],
					"grad2" : [ 1.0, 0.65098, 0.0, 1.0 ],
					"id" : "obj-40",
					"maxclass" : "panel",
					"mode" : 1,
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 15.0, 0.0, 707.0, 48.0 ],
					"presentation" : 1,
					"presentation_rect" : [ 30.0, 15.0, 539.0, 45.0 ],
					"proportion" : 0.39,
					"style" : "",
					"varname" : "autohelp_top_panel"
				}

			}
 ],
		"lines" : [ 			{
				"patchline" : 				{
					"destination" : [ "obj-22", 0 ],
					"disabled" : 0,
					"hidden" : 1,
					"source" : [ "obj-11", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-20", 0 ],
					"disabled" : 0,
					"hidden" : 1,
					"source" : [ "obj-12", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-62", 0 ],
					"disabled" : 0,
					"hidden" : 1,
					"source" : [ "obj-14", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-75", 0 ],
					"disabled" : 0,
					"hidden" : 1,
					"source" : [ "obj-16", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-75", 0 ],
					"disabled" : 0,
					"hidden" : 1,
					"source" : [ "obj-18", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-21", 0 ],
					"disabled" : 0,
					"hidden" : 0,
					"source" : [ "obj-19", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-19", 0 ],
					"disabled" : 0,
					"hidden" : 0,
					"source" : [ "obj-20", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-45", 0 ],
					"disabled" : 0,
					"hidden" : 1,
					"midpoints" : [ 370.5, 213.0, 24.5, 213.0 ],
					"source" : [ "obj-21", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-59", 2 ],
					"disabled" : 0,
					"hidden" : 1,
					"source" : [ "obj-22", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-59", 1 ],
					"disabled" : 0,
					"hidden" : 1,
					"source" : [ "obj-23", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-67", 0 ],
					"disabled" : 0,
					"hidden" : 0,
					"source" : [ "obj-24", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-26", 0 ],
					"disabled" : 0,
					"hidden" : 0,
					"source" : [ "obj-25", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-33", 0 ],
					"disabled" : 0,
					"hidden" : 1,
					"midpoints" : [ 371.5, 363.0, 26.5, 363.0 ],
					"source" : [ "obj-26", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-28", 0 ],
					"disabled" : 0,
					"hidden" : 0,
					"source" : [ "obj-27", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-33", 0 ],
					"disabled" : 0,
					"hidden" : 1,
					"midpoints" : [ 316.5, 363.0, 26.5, 363.0 ],
					"source" : [ "obj-28", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-30", 0 ],
					"disabled" : 0,
					"hidden" : 0,
					"source" : [ "obj-29", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-33", 0 ],
					"disabled" : 0,
					"hidden" : 1,
					"midpoints" : [ 170.5, 363.0, 26.5, 363.0 ],
					"source" : [ "obj-30", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-32", 0 ],
					"disabled" : 0,
					"hidden" : 0,
					"source" : [ "obj-31", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-33", 0 ],
					"disabled" : 0,
					"hidden" : 1,
					"midpoints" : [ 250.5, 363.0, 26.5, 363.0 ],
					"source" : [ "obj-32", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-36", 0 ],
					"disabled" : 0,
					"hidden" : 0,
					"source" : [ "obj-34", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-33", 0 ],
					"disabled" : 0,
					"hidden" : 1,
					"midpoints" : [ 26.5, 363.0, 26.5, 363.0 ],
					"source" : [ "obj-35", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-33", 0 ],
					"disabled" : 0,
					"hidden" : 1,
					"midpoints" : [ 107.5, 363.0, 26.5, 363.0 ],
					"source" : [ "obj-36", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-35", 0 ],
					"disabled" : 0,
					"hidden" : 0,
					"source" : [ "obj-37", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-39", 0 ],
					"disabled" : 0,
					"hidden" : 0,
					"source" : [ "obj-38", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-46", 0 ],
					"disabled" : 0,
					"hidden" : 1,
					"source" : [ "obj-39", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-46", 0 ],
					"disabled" : 0,
					"hidden" : 1,
					"midpoints" : [ 209.5, 285.0, 26.5, 285.0 ],
					"source" : [ "obj-41", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-41", 0 ],
					"disabled" : 0,
					"hidden" : 0,
					"source" : [ "obj-42", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-45", 0 ],
					"disabled" : 0,
					"hidden" : 1,
					"midpoints" : [ 155.5, 213.0, 24.5, 213.0 ],
					"source" : [ "obj-43", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-43", 0 ],
					"disabled" : 0,
					"hidden" : 0,
					"source" : [ "obj-44", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-18", 0 ],
					"disabled" : 0,
					"hidden" : 1,
					"source" : [ "obj-47", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-50", 0 ],
					"disabled" : 0,
					"hidden" : 0,
					"source" : [ "obj-48", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-46", 0 ],
					"disabled" : 0,
					"hidden" : 1,
					"midpoints" : [ 67.5, 285.0, 26.5, 285.0 ],
					"source" : [ "obj-49", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-46", 0 ],
					"disabled" : 0,
					"hidden" : 1,
					"midpoints" : [ 139.5, 285.0, 26.5, 285.0 ],
					"source" : [ "obj-50", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-49", 0 ],
					"disabled" : 0,
					"hidden" : 0,
					"source" : [ "obj-51", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-45", 0 ],
					"disabled" : 0,
					"hidden" : 1,
					"midpoints" : [ 292.5, 213.0, 24.5, 213.0 ],
					"source" : [ "obj-52", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-52", 0 ],
					"disabled" : 0,
					"hidden" : 0,
					"source" : [ "obj-53", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-68", 0 ],
					"disabled" : 0,
					"hidden" : 0,
					"source" : [ "obj-54", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-59", 0 ],
					"disabled" : 0,
					"hidden" : 1,
					"source" : [ "obj-56", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-59", 0 ],
					"disabled" : 0,
					"hidden" : 1,
					"source" : [ "obj-58", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-60", 0 ],
					"disabled" : 0,
					"hidden" : 1,
					"source" : [ "obj-59", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-74", 0 ],
					"disabled" : 0,
					"hidden" : 1,
					"source" : [ "obj-60", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-11", 0 ],
					"disabled" : 0,
					"hidden" : 1,
					"source" : [ "obj-62", 1 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-12", 0 ],
					"disabled" : 0,
					"hidden" : 1,
					"source" : [ "obj-62", 1 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-15", 0 ],
					"disabled" : 0,
					"hidden" : 1,
					"source" : [ "obj-62", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-66", 0 ],
					"disabled" : 0,
					"hidden" : 1,
					"source" : [ "obj-62", 1 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-15", 0 ],
					"disabled" : 0,
					"hidden" : 1,
					"source" : [ "obj-63", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-15", 0 ],
					"disabled" : 0,
					"hidden" : 1,
					"source" : [ "obj-64", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-20", 0 ],
					"disabled" : 0,
					"hidden" : 1,
					"source" : [ "obj-65", 2 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-22", 0 ],
					"disabled" : 0,
					"hidden" : 1,
					"source" : [ "obj-65", 2 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-65", 0 ],
					"disabled" : 0,
					"hidden" : 1,
					"source" : [ "obj-66", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-45", 0 ],
					"disabled" : 0,
					"hidden" : 1,
					"midpoints" : [ 24.5, 213.0, 24.5, 213.0 ],
					"source" : [ "obj-67", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-45", 0 ],
					"disabled" : 0,
					"hidden" : 1,
					"midpoints" : [ 90.5, 213.0, 24.5, 213.0 ],
					"source" : [ "obj-68", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-45", 0 ],
					"disabled" : 0,
					"hidden" : 1,
					"midpoints" : [ 222.5, 213.0, 24.5, 213.0 ],
					"source" : [ "obj-69", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-69", 0 ],
					"disabled" : 0,
					"hidden" : 0,
					"source" : [ "obj-70", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-15", 0 ],
					"disabled" : 0,
					"hidden" : 1,
					"source" : [ "obj-71", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-24", 0 ],
					"disabled" : 0,
					"hidden" : 0,
					"source" : [ "obj-72", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-61", 0 ],
					"disabled" : 0,
					"hidden" : 1,
					"source" : [ "obj-75", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-73", 1 ],
					"disabled" : 0,
					"hidden" : 1,
					"source" : [ "obj-75", 0 ]
				}

			}
, 			{
				"patchline" : 				{
					"destination" : [ "obj-73", 0 ],
					"disabled" : 0,
					"hidden" : 1,
					"source" : [ "obj-75", 0 ]
				}

			}
 ],
		"dependency_cache" : [ 			{
				"name" : "csound~.mxo",
				"type" : "iLaX"
			}
 ],
		"embedsnapshot" : 0
	}

}
