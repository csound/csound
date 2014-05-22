{
	"patcher" : 	{
		"fileversion" : 1,
		"rect" : [ 154.0, 112.0, 741.0, 429.0 ],
		"bglocked" : 0,
		"defrect" : [ 154.0, 112.0, 741.0, 429.0 ],
		"openrect" : [ 0.0, 0.0, 0.0, 0.0 ],
		"openinpresentation" : 0,
		"default_fontsize" : 12.0,
		"default_fontface" : 0,
		"default_fontname" : "Arial",
		"gridonopen" : 0,
		"gridsize" : [ 15.0, 15.0 ],
		"gridsnaponopen" : 0,
		"toolbarvisible" : 1,
		"boxanimatetime" : 200,
		"imprint" : 0,
		"enablehscroll" : 1,
		"enablevscroll" : 1,
		"devicewidth" : 0.0,
		"boxes" : [ 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "csound~ controls",
					"numoutlets" : 0,
					"fontsize" : 12.0,
					"patching_rect" : [ 450.0, 75.0, 117.0, 20.0 ],
					"fontname" : "Arial",
					"numinlets" : 1,
					"id" : "obj-80"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "pvar csIN",
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ],
					"patching_rect" : [ 585.0, 182.0, 67.0, 20.0 ],
					"hidden" : 1,
					"fontname" : "Arial",
					"numinlets" : 1,
					"id" : "obj-74"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "ezdac~",
					"presentation_rect" : [ 450.0, 105.0, 56.0, 56.0 ],
					"numoutlets" : 0,
					"offgradcolor1" : [ 0.0, 0.0, 0.0, 1.0 ],
					"bgcolor" : [ 0.0, 0.0, 0.0, 1.0 ],
					"patching_rect" : [ 465.0, 285.0, 56.0, 56.0 ],
					"ongradcolor1" : [ 1.0, 0.0, 0.0, 1.0 ],
					"presentation" : 1,
					"ongradcolor2" : [ 0.709804, 0.709804, 0.0, 1.0 ],
					"offgradcolor2" : [ 0.0, 0.0, 0.0, 1.0 ],
					"numinlets" : 2,
					"id" : "obj-73"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "open",
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ],
					"patching_rect" : [ 525.0, 105.0, 38.0, 18.0 ],
					"fontname" : "Arial",
					"numinlets" : 2,
					"id" : "obj-63"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"varname" : "autohelp_top_title",
					"text" : "Bass Drum",
					"presentation_linecount" : 2,
					"presentation_rect" : [ 30.0, 15.0, 135.0, 75.0 ],
					"numoutlets" : 0,
					"fontface" : 3,
					"fontsize" : 30.0,
					"patching_rect" : [ 20.0, 3.0, 172.0, 41.0 ],
					"presentation" : 1,
					"frgb" : [ 0.93, 0.93, 0.97, 1.0 ],
					"fontname" : "Arial",
					"numinlets" : 1,
					"id" : "obj-3",
					"textcolor" : [ 0.93, 0.93, 0.97, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "bpm",
					"numoutlets" : 0,
					"fontsize" : 12.0,
					"patching_rect" : [ 699.0, 105.0, 34.0, 20.0 ],
					"fontname" : "Arial",
					"numinlets" : 1,
					"id" : "obj-79"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "waveform",
					"numoutlets" : 0,
					"fontsize" : 12.0,
					"patching_rect" : [ 236.0, 305.0, 70.0, 20.0 ],
					"fontname" : "Arial",
					"numinlets" : 1,
					"id" : "obj-1"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "2: tanh",
					"numoutlets" : 0,
					"fontsize" : 12.0,
					"patching_rect" : [ 362.0, 394.0, 113.0, 20.0 ],
					"fontname" : "Arial",
					"numinlets" : 1,
					"id" : "obj-2"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "1: moog filter",
					"numoutlets" : 0,
					"fontsize" : 12.0,
					"patching_rect" : [ 161.0, 380.0, 79.0, 20.0 ],
					"fontname" : "Arial",
					"numinlets" : 1,
					"id" : "obj-4"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "0: no filter",
					"numoutlets" : 0,
					"fontsize" : 12.0,
					"patching_rect" : [ 161.0, 365.0, 78.0, 20.0 ],
					"fontname" : "Arial",
					"numinlets" : 1,
					"id" : "obj-5"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "1: sine",
					"numoutlets" : 0,
					"fontsize" : 12.0,
					"patching_rect" : [ 362.0, 379.0, 114.0, 20.0 ],
					"fontname" : "Arial",
					"numinlets" : 1,
					"id" : "obj-6"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "0: Bram de Jong",
					"numoutlets" : 0,
					"fontsize" : 12.0,
					"patching_rect" : [ 362.0, 364.0, 113.0, 20.0 ],
					"fontname" : "Arial",
					"numinlets" : 1,
					"id" : "obj-7"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "4: saw9",
					"numoutlets" : 0,
					"fontsize" : 12.0,
					"patching_rect" : [ 241.0, 409.0, 62.0, 20.0 ],
					"fontname" : "Arial",
					"numinlets" : 1,
					"id" : "obj-8"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "3: square9",
					"numoutlets" : 0,
					"fontsize" : 12.0,
					"patching_rect" : [ 241.0, 394.0, 76.0, 20.0 ],
					"fontname" : "Arial",
					"numinlets" : 1,
					"id" : "obj-9"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "2: triangle",
					"numoutlets" : 0,
					"fontsize" : 12.0,
					"patching_rect" : [ 241.0, 379.0, 72.0, 20.0 ],
					"fontname" : "Arial",
					"numinlets" : 1,
					"id" : "obj-10"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "90.",
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ],
					"patching_rect" : [ 660.0, 60.0, 28.0, 18.0 ],
					"hidden" : 1,
					"fontname" : "Arial",
					"numinlets" : 2,
					"id" : "obj-11"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "-12.",
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ],
					"patching_rect" : [ 361.0, 131.0, 37.0, 18.0 ],
					"hidden" : 1,
					"fontname" : "Arial",
					"numinlets" : 2,
					"id" : "obj-12"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "loadbang",
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "bang" ],
					"patching_rect" : [ 360.0, 53.0, 61.0, 20.0 ],
					"hidden" : 1,
					"fontname" : "Arial",
					"numinlets" : 1,
					"id" : "obj-14"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "pvar csIN",
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ],
					"patching_rect" : [ 435.0, 135.0, 67.0, 20.0 ],
					"hidden" : 1,
					"fontname" : "Arial",
					"numinlets" : 1,
					"id" : "obj-15"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"varname" : "csIN",
					"text" : "t l",
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ],
					"patching_rect" : [ 525.0, 195.0, 21.0, 20.0 ],
					"hidden" : 1,
					"fontname" : "Arial",
					"numinlets" : 1,
					"id" : "obj-16"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "csIN",
					"numoutlets" : 0,
					"fontsize" : 12.0,
					"patching_rect" : [ 543.0, 196.0, 34.0, 20.0 ],
					"hidden" : 1,
					"frgb" : [ 0.972549, 0.0, 0.0, 1.0 ],
					"fontname" : "Arial",
					"numinlets" : 1,
					"id" : "obj-17",
					"textcolor" : [ 0.972549, 0.0, 0.0, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "prepend c",
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ],
					"patching_rect" : [ 465.0, 216.0, 68.0, 20.0 ],
					"hidden" : 1,
					"fontname" : "Arial",
					"numinlets" : 1,
					"id" : "obj-18"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "dbtoa",
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ],
					"patching_rect" : [ 361.0, 173.0, 43.0, 20.0 ],
					"fontname" : "Arial",
					"numinlets" : 1,
					"id" : "obj-19"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "flonum",
					"numoutlets" : 2,
					"fontface" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "float", "bang" ],
					"minimum" : -90.0,
					"patching_rect" : [ 361.0, 151.0, 52.0, 20.0 ],
					"maximum" : 12.0,
					"triscale" : 0.9,
					"fontname" : "Arial",
					"numinlets" : 1,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ],
					"id" : "obj-20",
					"textcolor" : [ 0.815686, 0.0, 0.0, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "amp $1",
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ],
					"patching_rect" : [ 361.0, 194.0, 55.0, 18.0 ],
					"fontname" : "Arial",
					"numinlets" : 2,
					"id" : "obj-21"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "flonum",
					"numoutlets" : 2,
					"fontsize" : 12.0,
					"outlettype" : [ "float", "bang" ],
					"minimum" : 0.0,
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"patching_rect" : [ 660.0, 105.0, 40.0, 20.0 ],
					"triscale" : 0.9,
					"fontname" : "Arial",
					"numinlets" : 1,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ],
					"id" : "obj-22"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "button",
					"numoutlets" : 1,
					"outlettype" : [ "bang" ],
					"patching_rect" : [ 135.0, 75.0, 58.0, 58.0 ],
					"fgcolor" : [ 0.72549, 0.086275, 1.0, 1.0 ],
					"numinlets" : 1,
					"id" : "obj-23"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "* 0.5",
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "float" ],
					"patching_rect" : [ 15.0, 172.0, 49.0, 20.0 ],
					"fontname" : "Arial",
					"numinlets" : 2,
					"id" : "obj-24"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "number",
					"numoutlets" : 2,
					"fontsize" : 12.0,
					"outlettype" : [ "int", "bang" ],
					"minimum" : 0,
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"patching_rect" : [ 362.0, 321.0, 37.0, 20.0 ],
					"maximum" : 2,
					"triscale" : 0.9,
					"fontname" : "Arial",
					"numinlets" : 1,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ],
					"id" : "obj-25"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "clipType $1",
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ],
					"patching_rect" : [ 362.0, 343.0, 81.0, 18.0 ],
					"fontname" : "Arial",
					"numinlets" : 2,
					"id" : "obj-26"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "flonum",
					"numoutlets" : 2,
					"fontsize" : 12.0,
					"outlettype" : [ "float", "bang" ],
					"minimum" : 0.125,
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"patching_rect" : [ 307.0, 321.0, 50.0, 20.0 ],
					"maximum" : 64.0,
					"triscale" : 0.9,
					"fontname" : "Arial",
					"numinlets" : 1,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ],
					"id" : "obj-27"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "drive $1",
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ],
					"patching_rect" : [ 307.0, 343.0, 53.0, 18.0 ],
					"fontname" : "Arial",
					"numinlets" : 2,
					"id" : "obj-28"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "number",
					"numoutlets" : 2,
					"fontsize" : 12.0,
					"outlettype" : [ "int", "bang" ],
					"minimum" : 0,
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"patching_rect" : [ 161.0, 322.0, 37.0, 20.0 ],
					"maximum" : 1,
					"triscale" : 0.9,
					"fontname" : "Arial",
					"numinlets" : 1,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ],
					"id" : "obj-29"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "filt $1",
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ],
					"patching_rect" : [ 161.0, 344.0, 49.0, 18.0 ],
					"fontname" : "Arial",
					"numinlets" : 2,
					"id" : "obj-30"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "number",
					"numoutlets" : 2,
					"fontsize" : 12.0,
					"outlettype" : [ "int", "bang" ],
					"minimum" : 1,
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"patching_rect" : [ 241.0, 322.0, 37.0, 20.0 ],
					"maximum" : 4,
					"triscale" : 0.9,
					"fontname" : "Arial",
					"numinlets" : 1,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ],
					"id" : "obj-31"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "wave $1",
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ],
					"patching_rect" : [ 241.0, 344.0, 62.0, 18.0 ],
					"fontname" : "Arial",
					"numinlets" : 2,
					"id" : "obj-32"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "pvar cIN",
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ],
					"patching_rect" : [ 17.0, 368.0, 61.0, 20.0 ],
					"hidden" : 1,
					"fontname" : "Arial",
					"numinlets" : 1,
					"id" : "obj-33"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "flonum",
					"numoutlets" : 2,
					"fontsize" : 12.0,
					"outlettype" : [ "float", "bang" ],
					"minimum" : 0.0,
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"patching_rect" : [ 98.0, 322.0, 52.0, 20.0 ],
					"maximum" : 1.0,
					"triscale" : 0.9,
					"fontname" : "Arial",
					"numinlets" : 1,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ],
					"id" : "obj-34"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "filtFrqMult $1",
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ],
					"patching_rect" : [ 17.0, 344.0, 80.0, 18.0 ],
					"fontname" : "Arial",
					"numinlets" : 2,
					"id" : "obj-35"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "filtRes $1",
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ],
					"patching_rect" : [ 98.0, 344.0, 62.0, 18.0 ],
					"fontname" : "Arial",
					"numinlets" : 2,
					"id" : "obj-36"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "flonum",
					"numoutlets" : 2,
					"fontsize" : 12.0,
					"outlettype" : [ "float", "bang" ],
					"minimum" : 0.0,
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"patching_rect" : [ 17.0, 322.0, 52.0, 20.0 ],
					"triscale" : 0.9,
					"fontname" : "Arial",
					"numinlets" : 1,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ],
					"id" : "obj-37"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "number",
					"numoutlets" : 2,
					"fontsize" : 12.0,
					"outlettype" : [ "int", "bang" ],
					"minimum" : 1,
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"patching_rect" : [ 17.0, 243.0, 37.0, 20.0 ],
					"triscale" : 0.9,
					"fontname" : "Arial",
					"numinlets" : 1,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ],
					"id" : "obj-38"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "p atk",
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ],
					"patching_rect" : [ 17.0, 264.0, 40.0, 20.0 ],
					"fontname" : "Arial",
					"numinlets" : 1,
					"id" : "obj-39",
					"patcher" : 					{
						"fileversion" : 1,
						"rect" : [ 10.0, 59.0, 311.0, 252.0 ],
						"bglocked" : 0,
						"defrect" : [ 10.0, 59.0, 311.0, 252.0 ],
						"openrect" : [ 0.0, 0.0, 0.0, 0.0 ],
						"openinpresentation" : 0,
						"default_fontsize" : 12.0,
						"default_fontface" : 0,
						"default_fontname" : "Arial",
						"gridonopen" : 0,
						"gridsize" : [ 15.0, 15.0 ],
						"gridsnaponopen" : 0,
						"toolbarvisible" : 1,
						"boxanimatetime" : 200,
						"imprint" : 0,
						"enablehscroll" : 1,
						"enablevscroll" : 1,
						"devicewidth" : 0.0,
						"boxes" : [ 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "pak ampAtk 0.",
									"numoutlets" : 1,
									"fontsize" : 9.0,
									"outlettype" : [ "" ],
									"patching_rect" : [ 14.0, 92.0, 74.0, 17.0 ],
									"fontname" : "Arial",
									"numinlets" : 2,
									"id" : "obj-1"
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "* 0.0001",
									"numoutlets" : 1,
									"fontsize" : 9.0,
									"outlettype" : [ "float" ],
									"patching_rect" : [ 19.0, 65.0, 75.0, 17.0 ],
									"fontname" : "Arial",
									"numinlets" : 2,
									"id" : "obj-2"
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "outlet",
									"numoutlets" : 0,
									"patching_rect" : [ 19.0, 116.0, 15.0, 15.0 ],
									"numinlets" : 1,
									"id" : "obj-3",
									"comment" : ""
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "inlet",
									"numoutlets" : 1,
									"outlettype" : [ "int" ],
									"patching_rect" : [ 21.0, 35.0, 15.0, 15.0 ],
									"numinlets" : 0,
									"id" : "obj-4",
									"comment" : ""
								}

							}
 ],
						"lines" : [ 							{
								"patchline" : 								{
									"source" : [ "obj-4", 0 ],
									"destination" : [ "obj-2", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-1", 0 ],
									"destination" : [ "obj-3", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-2", 0 ],
									"destination" : [ "obj-1", 1 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
 ]
					}
,
					"saved_object_attributes" : 					{
						"fontface" : 0,
						"default_fontface" : 0,
						"fontsize" : 12.0,
						"globalpatchername" : "",
						"default_fontname" : "Arial",
						"default_fontsize" : 12.0,
						"fontname" : "Arial"
					}

				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "ampSus $1",
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ],
					"patching_rect" : [ 200.0, 265.0, 72.0, 18.0 ],
					"fontname" : "Arial",
					"numinlets" : 2,
					"id" : "obj-41"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "flonum",
					"numoutlets" : 2,
					"fontsize" : 12.0,
					"outlettype" : [ "float", "bang" ],
					"minimum" : 0.001,
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"patching_rect" : [ 200.0, 243.0, 51.0, 20.0 ],
					"maximum" : 1.0,
					"triscale" : 0.9,
					"fontname" : "Arial",
					"numinlets" : 1,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ],
					"id" : "obj-42"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "frqSus $1",
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ],
					"patching_rect" : [ 146.0, 194.0, 65.0, 18.0 ],
					"fontname" : "Arial",
					"numinlets" : 2,
					"id" : "obj-43"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "flonum",
					"numoutlets" : 2,
					"fontsize" : 12.0,
					"outlettype" : [ "float", "bang" ],
					"minimum" : 0.001,
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"patching_rect" : [ 146.0, 172.0, 52.0, 20.0 ],
					"maximum" : 1.0,
					"triscale" : 0.9,
					"fontname" : "Arial",
					"numinlets" : 1,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ],
					"id" : "obj-44"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "pvar cIN",
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ],
					"patching_rect" : [ 15.0, 218.0, 61.0, 20.0 ],
					"hidden" : 1,
					"fontname" : "Arial",
					"numinlets" : 1,
					"id" : "obj-45"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "pvar cIN",
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ],
					"patching_rect" : [ 17.0, 290.0, 61.0, 20.0 ],
					"hidden" : 1,
					"fontname" : "Arial",
					"numinlets" : 1,
					"id" : "obj-46"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"varname" : "cIN",
					"text" : "t l",
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ],
					"patching_rect" : [ 465.0, 195.0, 21.0, 20.0 ],
					"hidden" : 1,
					"fontname" : "Arial",
					"numinlets" : 1,
					"id" : "obj-47"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "flonum",
					"numoutlets" : 2,
					"fontsize" : 12.0,
					"outlettype" : [ "float", "bang" ],
					"minimum" : 0.02,
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"patching_rect" : [ 130.0, 243.0, 52.0, 20.0 ],
					"triscale" : 0.9,
					"fontname" : "Arial",
					"numinlets" : 1,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ],
					"id" : "obj-48"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "ampDec $1",
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ],
					"patching_rect" : [ 58.0, 265.0, 72.0, 18.0 ],
					"fontname" : "Arial",
					"numinlets" : 2,
					"id" : "obj-49"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "ampRel $1",
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ],
					"patching_rect" : [ 130.0, 265.0, 69.0, 18.0 ],
					"fontname" : "Arial",
					"numinlets" : 2,
					"id" : "obj-50"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "flonum",
					"numoutlets" : 2,
					"fontsize" : 12.0,
					"outlettype" : [ "float", "bang" ],
					"minimum" : 0.0,
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"patching_rect" : [ 58.0, 243.0, 52.0, 20.0 ],
					"triscale" : 0.9,
					"fontname" : "Arial",
					"numinlets" : 1,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ],
					"id" : "obj-51"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "frqRange $1",
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ],
					"patching_rect" : [ 283.0, 194.0, 77.0, 18.0 ],
					"fontname" : "Arial",
					"numinlets" : 2,
					"id" : "obj-52"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "flonum",
					"numoutlets" : 2,
					"fontsize" : 12.0,
					"outlettype" : [ "float", "bang" ],
					"minimum" : 0.0,
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"patching_rect" : [ 283.0, 172.0, 52.0, 20.0 ],
					"maximum" : 1.0,
					"triscale" : 0.9,
					"fontname" : "Arial",
					"numinlets" : 1,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ],
					"id" : "obj-53"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "flonum",
					"numoutlets" : 2,
					"fontsize" : 12.0,
					"outlettype" : [ "float", "bang" ],
					"minimum" : 0.0,
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"patching_rect" : [ 81.0, 172.0, 52.0, 20.0 ],
					"triscale" : 0.9,
					"fontname" : "Arial",
					"numinlets" : 1,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ],
					"id" : "obj-54"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "metronome controls",
					"numoutlets" : 0,
					"fontsize" : 12.0,
					"patching_rect" : [ 585.0, 75.0, 131.0, 20.0 ],
					"fontname" : "Arial",
					"numinlets" : 1,
					"id" : "obj-55"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "button",
					"numoutlets" : 1,
					"outlettype" : [ "bang" ],
					"patching_rect" : [ 585.0, 105.0, 21.0, 21.0 ],
					"fgcolor" : [ 0.078431, 0.909804, 0.035294, 1.0 ],
					"numinlets" : 1,
					"id" : "obj-56"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "stop",
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ],
					"patching_rect" : [ 615.0, 105.0, 34.0, 18.0 ],
					"fontname" : "Arial",
					"numinlets" : 2,
					"id" : "obj-58"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "p MIDI Input",
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ],
					"patching_rect" : [ 585.0, 135.0, 76.0, 20.0 ],
					"hidden" : 1,
					"fontname" : "Arial",
					"numinlets" : 3,
					"id" : "obj-59",
					"color" : [ 1.0, 0.741176, 0.611765, 1.0 ],
					"patcher" : 					{
						"fileversion" : 1,
						"rect" : [ 416.0, 590.0, 508.0, 357.0 ],
						"bglocked" : 0,
						"defrect" : [ 416.0, 590.0, 508.0, 357.0 ],
						"openrect" : [ 0.0, 0.0, 0.0, 0.0 ],
						"openinpresentation" : 0,
						"default_fontsize" : 12.0,
						"default_fontface" : 0,
						"default_fontname" : "Arial",
						"gridonopen" : 0,
						"gridsize" : [ 15.0, 15.0 ],
						"gridsnaponopen" : 0,
						"toolbarvisible" : 1,
						"boxanimatetime" : 200,
						"imprint" : 0,
						"enablehscroll" : 1,
						"enablevscroll" : 1,
						"devicewidth" : 0.0,
						"boxes" : [ 							{
								"box" : 								{
									"maxclass" : "inlet",
									"numoutlets" : 1,
									"outlettype" : [ "bang" ],
									"patching_rect" : [ 173.0, 23.0, 15.0, 15.0 ],
									"numinlets" : 0,
									"id" : "obj-1",
									"comment" : ""
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "inlet",
									"numoutlets" : 1,
									"outlettype" : [ "bang" ],
									"patching_rect" : [ 153.0, 21.0, 15.0, 15.0 ],
									"numinlets" : 0,
									"id" : "obj-2",
									"comment" : ""
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "inlet",
									"numoutlets" : 1,
									"outlettype" : [ "float" ],
									"patching_rect" : [ 199.0, 23.0, 15.0, 15.0 ],
									"numinlets" : 0,
									"id" : "obj-3",
									"comment" : "BPM"
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "makenote 64 500",
									"numoutlets" : 2,
									"fontsize" : 9.0,
									"outlettype" : [ "float", "float" ],
									"patching_rect" : [ 153.0, 188.0, 88.0, 17.0 ],
									"fontname" : "Arial",
									"numinlets" : 3,
									"id" : "obj-4"
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "+ 30",
									"numoutlets" : 1,
									"fontsize" : 9.0,
									"outlettype" : [ "int" ],
									"patching_rect" : [ 153.0, 166.0, 31.0, 17.0 ],
									"fontname" : "Arial",
									"numinlets" : 2,
									"id" : "obj-5"
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "random 60",
									"numoutlets" : 1,
									"fontsize" : 9.0,
									"outlettype" : [ "int" ],
									"patching_rect" : [ 153.0, 146.0, 55.0, 17.0 ],
									"fontname" : "Arial",
									"numinlets" : 2,
									"id" : "obj-6"
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "* 1.",
									"numoutlets" : 1,
									"fontsize" : 9.0,
									"outlettype" : [ "float" ],
									"patching_rect" : [ 199.0, 64.0, 27.0, 17.0 ],
									"fontname" : "Arial",
									"numinlets" : 2,
									"id" : "obj-7"
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "* 60000.",
									"numoutlets" : 1,
									"fontsize" : 9.0,
									"outlettype" : [ "float" ],
									"patching_rect" : [ 199.0, 84.0, 52.0, 17.0 ],
									"fontname" : "Arial",
									"numinlets" : 2,
									"id" : "obj-8"
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "pow -1.",
									"numoutlets" : 1,
									"fontsize" : 9.0,
									"outlettype" : [ "float" ],
									"patching_rect" : [ 199.0, 42.0, 44.0, 17.0 ],
									"fontname" : "Arial",
									"numinlets" : 2,
									"id" : "obj-9"
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "metro 500",
									"numoutlets" : 1,
									"fontsize" : 9.0,
									"outlettype" : [ "bang" ],
									"patching_rect" : [ 153.0, 125.0, 56.0, 17.0 ],
									"fontname" : "Arial",
									"numinlets" : 2,
									"id" : "obj-10",
									"color" : [ 1.0, 0.890196, 0.090196, 1.0 ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "outlet",
									"numoutlets" : 0,
									"patching_rect" : [ 153.0, 267.0, 15.0, 15.0 ],
									"numinlets" : 1,
									"id" : "obj-11",
									"comment" : ""
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "pack 0 0",
									"numoutlets" : 1,
									"fontsize" : 9.0,
									"outlettype" : [ "" ],
									"patching_rect" : [ 153.0, 211.0, 47.0, 17.0 ],
									"fontname" : "Arial",
									"numinlets" : 2,
									"id" : "obj-12"
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "BPM",
									"numoutlets" : 0,
									"fontsize" : 9.0,
									"patching_rect" : [ 215.0, 23.0, 32.0, 17.0 ],
									"fontname" : "Arial",
									"numinlets" : 1,
									"id" : "obj-13"
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "metro control",
									"numoutlets" : 0,
									"fontsize" : 9.0,
									"patching_rect" : [ 78.0, 22.0, 71.0, 17.0 ],
									"fontname" : "Arial",
									"numinlets" : 1,
									"id" : "obj-14"
								}

							}
 ],
						"lines" : [ 							{
								"patchline" : 								{
									"source" : [ "obj-2", 0 ],
									"destination" : [ "obj-10", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-1", 0 ],
									"destination" : [ "obj-6", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-10", 0 ],
									"destination" : [ "obj-6", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-6", 0 ],
									"destination" : [ "obj-5", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-5", 0 ],
									"destination" : [ "obj-4", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-4", 0 ],
									"destination" : [ "obj-12", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-12", 0 ],
									"destination" : [ "obj-11", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-4", 1 ],
									"destination" : [ "obj-12", 1 ],
									"hidden" : 0,
									"midpoints" : [ 231.5, 208.0, 190.5, 208.0 ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-3", 0 ],
									"destination" : [ "obj-9", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-9", 0 ],
									"destination" : [ "obj-7", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-7", 0 ],
									"destination" : [ "obj-8", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-8", 0 ],
									"destination" : [ "obj-10", 1 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-8", 0 ],
									"destination" : [ "obj-4", 2 ],
									"hidden" : 0,
									"midpoints" : [ 208.5, 109.0, 231.5, 109.0 ]
								}

							}
 ]
					}
,
					"saved_object_attributes" : 					{
						"fontface" : 0,
						"default_fontface" : 0,
						"fontsize" : 12.0,
						"globalpatchername" : "",
						"default_fontname" : "Arial",
						"default_fontsize" : 12.0,
						"fontname" : "Arial"
					}

				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "midiformat 1",
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "int" ],
					"patching_rect" : [ 585.0, 158.0, 119.0, 20.0 ],
					"hidden" : 1,
					"fontname" : "Arial",
					"numinlets" : 7,
					"id" : "obj-60"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "scope~",
					"numoutlets" : 0,
					"bgcolor" : [ 0.227451, 0.227451, 0.227451, 1.0 ],
					"patching_rect" : [ 525.0, 285.0, 125.0, 113.0 ],
					"calccount" : 16,
					"rounded" : 0,
					"numinlets" : 2,
					"id" : "obj-61"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "t b b",
					"numoutlets" : 2,
					"fontsize" : 12.0,
					"outlettype" : [ "bang", "bang" ],
					"patching_rect" : [ 360.0, 75.0, 33.0, 20.0 ],
					"hidden" : 1,
					"fontname" : "Arial",
					"numinlets" : 1,
					"id" : "obj-62"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "button",
					"numoutlets" : 1,
					"outlettype" : [ "bang" ],
					"patching_rect" : [ 450.0, 105.0, 21.0, 21.0 ],
					"fgcolor" : [ 0.078431, 0.909804, 0.035294, 1.0 ],
					"numinlets" : 1,
					"id" : "obj-64"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "preset",
					"numoutlets" : 4,
					"margin" : 4,
					"outlettype" : [ "preset", "int", "preset", "int" ],
					"patching_rect" : [ 15.0, 75.0, 104.0, 57.0 ],
					"active1" : [ 0.0, 0.203922, 1.0, 1.0 ],
					"bubblesize" : 10,
					"numinlets" : 1,
					"spacing" : 2,
					"id" : "obj-65",
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
 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "3",
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ],
					"patching_rect" : [ 15.0, 53.0, 18.0, 18.0 ],
					"hidden" : 1,
					"fontname" : "Arial",
					"numinlets" : 2,
					"id" : "obj-66"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "frqDec $1",
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ],
					"patching_rect" : [ 15.0, 194.0, 64.0, 18.0 ],
					"fontname" : "Arial",
					"numinlets" : 2,
					"id" : "obj-67"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "frqRel $1",
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ],
					"patching_rect" : [ 81.0, 194.0, 62.0, 18.0 ],
					"fontname" : "Arial",
					"numinlets" : 2,
					"id" : "obj-68"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "frqBase $1",
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ],
					"patching_rect" : [ 213.0, 194.0, 69.0, 18.0 ],
					"fontname" : "Arial",
					"numinlets" : 2,
					"id" : "obj-69"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "flonum",
					"numoutlets" : 2,
					"fontsize" : 12.0,
					"outlettype" : [ "float", "bang" ],
					"minimum" : 0.0,
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"patching_rect" : [ 213.0, 172.0, 51.0, 20.0 ],
					"maximum" : 1.0,
					"triscale" : 0.9,
					"fontname" : "Arial",
					"numinlets" : 1,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ],
					"id" : "obj-70"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "stop",
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ],
					"patching_rect" : [ 480.0, 105.0, 35.0, 18.0 ],
					"fontname" : "Arial",
					"numinlets" : 2,
					"id" : "obj-71"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "flonum",
					"numoutlets" : 2,
					"fontsize" : 12.0,
					"outlettype" : [ "float", "bang" ],
					"minimum" : 0.0,
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"patching_rect" : [ 15.0, 150.0, 52.0, 20.0 ],
					"triscale" : 0.9,
					"fontname" : "Arial",
					"numinlets" : 1,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ],
					"id" : "obj-72"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "csound~ @io 1 \"-m0 bassdrum.csd\"",
					"numoutlets" : 5,
					"fontsize" : 12.0,
					"outlettype" : [ "signal", "list", "int", "bang", "bang" ],
					"patching_rect" : [ 465.0, 255.0, 264.0, 20.0 ],
					"fontname" : "Arial",
					"numinlets" : 1,
					"id" : "obj-75",
					"saved_object_attributes" : 					{
						"bypass" : 0,
						"interval" : 10,
						"output" : 1,
						"autostart" : 0,
						"input" : 1,
						"overdrive" : 0,
						"matchsr" : 1,
						"message" : 1,
						"args" : "-m0 bassdrum.csd"
					}

				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "cIN",
					"numoutlets" : 0,
					"fontsize" : 12.0,
					"patching_rect" : [ 483.0, 196.0, 32.0, 20.0 ],
					"hidden" : 1,
					"frgb" : [ 0.972549, 0.0, 0.0, 1.0 ],
					"fontname" : "Arial",
					"numinlets" : 1,
					"id" : "obj-76",
					"textcolor" : [ 0.972549, 0.0, 0.0, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "Hit Me!",
					"numoutlets" : 0,
					"fontsize" : 12.0,
					"patching_rect" : [ 144.0, 56.0, 47.0, 20.0 ],
					"fontname" : "Arial",
					"numinlets" : 1,
					"id" : "obj-77"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "1: sine",
					"numoutlets" : 0,
					"fontsize" : 12.0,
					"patching_rect" : [ 241.0, 364.0, 53.0, 20.0 ],
					"fontname" : "Arial",
					"numinlets" : 1,
					"id" : "obj-78"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "panel",
					"varname" : "autohelp_top_panel",
					"presentation_rect" : [ 30.0, 15.0, 539.0, 45.0 ],
					"numoutlets" : 0,
					"grad1" : [ 0.117647, 0.117647, 0.117647, 1.0 ],
					"patching_rect" : [ 15.0, 0.0, 707.0, 48.0 ],
					"presentation" : 1,
					"grad2" : [ 1.0, 0.65098, 0.0, 1.0 ],
					"mode" : 1,
					"numinlets" : 1,
					"id" : "obj-40",
					"background" : 1
				}

			}
 ],
		"lines" : [ 			{
				"patchline" : 				{
					"source" : [ "obj-62", 1 ],
					"destination" : [ "obj-66", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-62", 1 ],
					"destination" : [ "obj-12", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-62", 1 ],
					"destination" : [ "obj-11", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-62", 0 ],
					"destination" : [ "obj-15", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-14", 0 ],
					"destination" : [ "obj-62", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-72", 0 ],
					"destination" : [ "obj-24", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-24", 0 ],
					"destination" : [ "obj-67", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-67", 0 ],
					"destination" : [ "obj-45", 0 ],
					"hidden" : 1,
					"midpoints" : [ 24.5, 213.0, 24.5, 213.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-68", 0 ],
					"destination" : [ "obj-45", 0 ],
					"hidden" : 1,
					"midpoints" : [ 90.5, 213.0, 24.5, 213.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-69", 0 ],
					"destination" : [ "obj-45", 0 ],
					"hidden" : 1,
					"midpoints" : [ 222.5, 213.0, 24.5, 213.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-52", 0 ],
					"destination" : [ "obj-45", 0 ],
					"hidden" : 1,
					"midpoints" : [ 292.5, 213.0, 24.5, 213.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-43", 0 ],
					"destination" : [ "obj-45", 0 ],
					"hidden" : 1,
					"midpoints" : [ 155.5, 213.0, 24.5, 213.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-21", 0 ],
					"destination" : [ "obj-45", 0 ],
					"hidden" : 1,
					"midpoints" : [ 370.5, 213.0, 24.5, 213.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-38", 0 ],
					"destination" : [ "obj-39", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-39", 0 ],
					"destination" : [ "obj-46", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-49", 0 ],
					"destination" : [ "obj-46", 0 ],
					"hidden" : 1,
					"midpoints" : [ 67.5, 285.0, 26.5, 285.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-50", 0 ],
					"destination" : [ "obj-46", 0 ],
					"hidden" : 1,
					"midpoints" : [ 139.5, 285.0, 26.5, 285.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-41", 0 ],
					"destination" : [ "obj-46", 0 ],
					"hidden" : 1,
					"midpoints" : [ 209.5, 285.0, 26.5, 285.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-37", 0 ],
					"destination" : [ "obj-35", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-26", 0 ],
					"destination" : [ "obj-33", 0 ],
					"hidden" : 1,
					"midpoints" : [ 371.5, 363.0, 26.5, 363.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-28", 0 ],
					"destination" : [ "obj-33", 0 ],
					"hidden" : 1,
					"midpoints" : [ 316.5, 363.0, 26.5, 363.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-30", 0 ],
					"destination" : [ "obj-33", 0 ],
					"hidden" : 1,
					"midpoints" : [ 170.5, 363.0, 26.5, 363.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-32", 0 ],
					"destination" : [ "obj-33", 0 ],
					"hidden" : 1,
					"midpoints" : [ 250.5, 363.0, 26.5, 363.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-36", 0 ],
					"destination" : [ "obj-33", 0 ],
					"hidden" : 1,
					"midpoints" : [ 107.5, 363.0, 26.5, 363.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-35", 0 ],
					"destination" : [ "obj-33", 0 ],
					"hidden" : 1,
					"midpoints" : [ 26.5, 363.0, 26.5, 363.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-66", 0 ],
					"destination" : [ "obj-65", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-51", 0 ],
					"destination" : [ "obj-49", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-54", 0 ],
					"destination" : [ "obj-68", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-34", 0 ],
					"destination" : [ "obj-36", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-48", 0 ],
					"destination" : [ "obj-50", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-44", 0 ],
					"destination" : [ "obj-43", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-29", 0 ],
					"destination" : [ "obj-30", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-42", 0 ],
					"destination" : [ "obj-41", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-70", 0 ],
					"destination" : [ "obj-69", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-31", 0 ],
					"destination" : [ "obj-32", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-53", 0 ],
					"destination" : [ "obj-52", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-27", 0 ],
					"destination" : [ "obj-28", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-25", 0 ],
					"destination" : [ "obj-26", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-47", 0 ],
					"destination" : [ "obj-18", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-65", 2 ],
					"destination" : [ "obj-20", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-12", 0 ],
					"destination" : [ "obj-20", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-20", 0 ],
					"destination" : [ "obj-19", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-19", 0 ],
					"destination" : [ "obj-21", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-56", 0 ],
					"destination" : [ "obj-59", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-59", 0 ],
					"destination" : [ "obj-60", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-23", 0 ],
					"destination" : [ "obj-59", 1 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-65", 2 ],
					"destination" : [ "obj-22", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-11", 0 ],
					"destination" : [ "obj-22", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-22", 0 ],
					"destination" : [ "obj-59", 2 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-75", 0 ],
					"destination" : [ "obj-61", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-18", 0 ],
					"destination" : [ "obj-75", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-16", 0 ],
					"destination" : [ "obj-75", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-64", 0 ],
					"destination" : [ "obj-15", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-71", 0 ],
					"destination" : [ "obj-15", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-63", 0 ],
					"destination" : [ "obj-15", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-75", 0 ],
					"destination" : [ "obj-73", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-75", 0 ],
					"destination" : [ "obj-73", 1 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-60", 0 ],
					"destination" : [ "obj-74", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-58", 0 ],
					"destination" : [ "obj-59", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
 ]
	}

}
