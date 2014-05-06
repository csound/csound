{
	"patcher" : 	{
		"fileversion" : 1,
		"rect" : [ 94.0, 139.0, 555.0, 267.0 ],
		"bglocked" : 0,
		"defrect" : [ 94.0, 139.0, 555.0, 267.0 ],
		"openrect" : [ 0.0, 0.0, 0.0, 0.0 ],
		"openinpresentation" : 1,
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
					"text" : "open csd file:",
					"id" : "obj-14",
					"presentation_rect" : [ 422.0, 46.0, 86.0, 20.0 ],
					"fontname" : "Arial",
					"numinlets" : 1,
					"patching_rect" : [ 240.0, 135.0, 83.0, 20.0 ],
					"presentation" : 1,
					"numoutlets" : 0,
					"fontsize" : 12.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "open",
					"id" : "obj-15",
					"presentation_rect" : [ 502.0, 47.0, 38.0, 18.0 ],
					"fontname" : "Arial",
					"bgcolor" : [ 0.0, 0.94902, 1.0, 1.0 ],
					"numinlets" : 2,
					"patching_rect" : [ 263.0, 155.0, 38.0, 18.0 ],
					"presentation" : 1,
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "Code adapted from the Csound book",
					"linecount" : 2,
					"presentation_linecount" : 2,
					"id" : "obj-13",
					"textcolor" : [ 1.0, 1.0, 1.0, 1.0 ],
					"presentation_rect" : [ 411.0, 6.0, 118.0, 34.0 ],
					"fontname" : "Arial",
					"numinlets" : 1,
					"patching_rect" : [ 135.0, 0.0, 118.0, 34.0 ],
					"presentation" : 1,
					"numoutlets" : 0,
					"fontsize" : 12.0,
					"frgb" : [ 1.0, 1.0, 1.0, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "scope~",
					"id" : "obj-10",
					"presentation_rect" : [ 285.0, 75.0, 125.0, 90.0 ],
					"bgcolor" : [ 0.0, 0.0, 0.0, 1.0 ],
					"numinlets" : 2,
					"patching_rect" : [ 195.0, 210.0, 125.0, 90.0 ],
					"presentation" : 1,
					"numoutlets" : 0,
					"rounded" : 0,
					"gridcolor" : [ 0.090196, 0.078431, 0.078431, 1.0 ],
					"calccount" : 16
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : ";\r\ndsp open",
					"linecount" : 2,
					"presentation_linecount" : 2,
					"id" : "obj-61",
					"presentation_rect" : [ 420.0, 135.0, 62.0, 32.0 ],
					"fontname" : "Arial",
					"numinlets" : 2,
					"patching_rect" : [ 399.0, 244.0, 62.0, 32.0 ],
					"presentation" : 1,
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "amp",
					"id" : "obj-5",
					"presentation_rect" : [ 487.0, 96.0, 36.0, 20.0 ],
					"fontname" : "Arial",
					"numinlets" : 1,
					"patching_rect" : [ 474.0, 191.0, 36.0, 20.0 ],
					"presentation" : 1,
					"numoutlets" : 0,
					"fontsize" : 12.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "number",
					"id" : "obj-54",
					"textcolor" : [ 0.784314, 0.0, 0.0, 1.0 ],
					"presentation_rect" : [ 480.0, 75.0, 50.0, 20.0 ],
					"fontname" : "Arial",
					"numinlets" : 1,
					"patching_rect" : [ 414.0, 191.0, 50.0, 20.0 ],
					"presentation" : 1,
					"numoutlets" : 2,
					"fontface" : 1,
					"fontsize" : 12.0,
					"maximum" : 12,
					"outlettype" : [ "int", "bang" ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "ezdac~",
					"id" : "obj-6",
					"ongradcolor2" : [ 0.709804, 0.709804, 0.0, 1.0 ],
					"offgradcolor2" : [ 0.0, 0.0, 0.0, 1.0 ],
					"presentation_rect" : [ 420.0, 75.0, 56.0, 56.0 ],
					"bgcolor" : [ 0.0, 0.0, 0.0, 1.0 ],
					"numinlets" : 2,
					"patching_rect" : [ 339.0, 221.0, 56.0, 56.0 ],
					"presentation" : 1,
					"numoutlets" : 0,
					"offgradcolor1" : [ 0.0, 0.0, 0.0, 1.0 ],
					"ongradcolor1" : [ 1.0, 0.0, 0.0, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "p dB amp",
					"id" : "obj-8",
					"fontname" : "Arial",
					"numinlets" : 3,
					"patching_rect" : [ 341.0, 188.0, 64.0, 20.0 ],
					"numoutlets" : 2,
					"fontsize" : 12.0,
					"outlettype" : [ "signal", "signal" ],
					"patcher" : 					{
						"fileversion" : 1,
						"rect" : [ 311.0, 235.0, 430.0, 267.0 ],
						"bglocked" : 0,
						"defrect" : [ 311.0, 235.0, 430.0, 267.0 ],
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
									"text" : "sig~",
									"id" : "obj-1",
									"fontname" : "Arial",
									"numinlets" : 1,
									"patching_rect" : [ 269.0, 87.0, 29.0, 17.0 ],
									"numoutlets" : 1,
									"fontsize" : 9.0,
									"outlettype" : [ "signal" ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "rampsmooth~ 500 500",
									"id" : "obj-2",
									"fontname" : "Arial",
									"numinlets" : 3,
									"patching_rect" : [ 269.0, 110.0, 113.0, 17.0 ],
									"numoutlets" : 1,
									"fontsize" : 9.0,
									"outlettype" : [ "signal" ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "dbtoa",
									"id" : "obj-3",
									"fontname" : "Arial",
									"numinlets" : 1,
									"patching_rect" : [ 269.0, 66.0, 34.0, 17.0 ],
									"numoutlets" : 1,
									"fontsize" : 9.0,
									"outlettype" : [ "" ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "*~",
									"id" : "obj-4",
									"fontname" : "Arial",
									"numinlets" : 2,
									"patching_rect" : [ 177.0, 148.0, 27.0, 17.0 ],
									"numoutlets" : 1,
									"fontsize" : 9.0,
									"outlettype" : [ "signal" ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "*~",
									"id" : "obj-5",
									"fontname" : "Arial",
									"numinlets" : 2,
									"patching_rect" : [ 95.0, 148.0, 27.0, 17.0 ],
									"numoutlets" : 1,
									"fontsize" : 9.0,
									"outlettype" : [ "signal" ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "outlet",
									"id" : "obj-6",
									"numinlets" : 1,
									"patching_rect" : [ 177.0, 184.0, 15.0, 15.0 ],
									"numoutlets" : 0,
									"comment" : ""
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "outlet",
									"id" : "obj-7",
									"numinlets" : 1,
									"patching_rect" : [ 95.0, 184.0, 15.0, 15.0 ],
									"numoutlets" : 0,
									"comment" : ""
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "inlet",
									"id" : "obj-8",
									"numinlets" : 0,
									"patching_rect" : [ 269.0, 44.0, 15.0, 15.0 ],
									"numoutlets" : 1,
									"outlettype" : [ "int" ],
									"comment" : ""
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "inlet",
									"id" : "obj-9",
									"numinlets" : 0,
									"patching_rect" : [ 177.0, 44.0, 15.0, 15.0 ],
									"numoutlets" : 1,
									"outlettype" : [ "signal" ],
									"comment" : ""
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "inlet",
									"id" : "obj-10",
									"numinlets" : 0,
									"patching_rect" : [ 95.0, 44.0, 15.0, 15.0 ],
									"numoutlets" : 1,
									"outlettype" : [ "signal" ],
									"comment" : ""
								}

							}
 ],
						"lines" : [ 							{
								"patchline" : 								{
									"source" : [ "obj-1", 0 ],
									"destination" : [ "obj-2", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-3", 0 ],
									"destination" : [ "obj-1", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-8", 0 ],
									"destination" : [ "obj-3", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-2", 0 ],
									"destination" : [ "obj-4", 1 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-4", 0 ],
									"destination" : [ "obj-6", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-9", 0 ],
									"destination" : [ "obj-4", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-2", 0 ],
									"destination" : [ "obj-5", 1 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-5", 0 ],
									"destination" : [ "obj-7", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-10", 0 ],
									"destination" : [ "obj-5", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
 ]
					}
,
					"saved_object_attributes" : 					{
						"fontname" : "Arial",
						"default_fontface" : 0,
						"globalpatchername" : "",
						"default_fontname" : "Arial",
						"fontface" : 0,
						"default_fontsize" : 12.0,
						"fontsize" : 12.0
					}

				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"varname" : "autohelp_top_title",
					"text" : "fof",
					"id" : "obj-63",
					"textcolor" : [ 0.93, 0.93, 0.97, 1.0 ],
					"presentation_rect" : [ 20.0, 3.0, 121.0, 41.0 ],
					"fontname" : "Arial",
					"numinlets" : 1,
					"patching_rect" : [ 5.0, 3.0, 121.0, 41.0 ],
					"presentation" : 1,
					"numoutlets" : 0,
					"fontface" : 3,
					"fontsize" : 30.0,
					"frgb" : [ 0.93, 0.93, 0.97, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "pack clear 0",
					"id" : "obj-127",
					"fontname" : "Arial",
					"numinlets" : 2,
					"patching_rect" : [ 516.0, 102.0, 75.0, 20.0 ],
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "button",
					"id" : "obj-126",
					"numinlets" : 1,
					"patching_rect" : [ 516.0, 79.0, 20.0, 20.0 ],
					"numoutlets" : 1,
					"outlettype" : [ "bang" ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "number",
					"id" : "obj-125",
					"fontname" : "Arial",
					"numinlets" : 1,
					"patching_rect" : [ 572.0, 78.0, 50.0, 20.0 ],
					"numoutlets" : 2,
					"minimum" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "int", "bang" ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "Reverb Parameters",
					"id" : "obj-53",
					"presentation_rect" : [ 60.0, 195.0, 119.0, 20.0 ],
					"fontname" : "Arial",
					"numinlets" : 1,
					"patching_rect" : [ 509.0, 255.0, 119.0, 20.0 ],
					"presentation" : 1,
					"numoutlets" : 0,
					"fontsize" : 12.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "flonum",
					"id" : "obj-1",
					"fontname" : "Arial",
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"numinlets" : 1,
					"patching_rect" : [ 262.0, 43.0, 40.0, 20.0 ],
					"numoutlets" : 2,
					"minimum" : -60.0,
					"fontface" : 1,
					"fontsize" : 12.0,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ],
					"maximum" : 0.0,
					"outlettype" : [ "float", "bang" ],
					"triscale" : 0.9
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "dbtoa",
					"id" : "obj-2",
					"fontname" : "Arial",
					"numinlets" : 1,
					"patching_rect" : [ 262.0, 64.0, 43.0, 20.0 ],
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "amp $1",
					"id" : "obj-3",
					"fontname" : "Arial",
					"numinlets" : 2,
					"patching_rect" : [ 262.0, 86.0, 51.0, 18.0 ],
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "umenu",
					"id" : "obj-4",
					"presentation_rect" : [ 180.0, 60.0, 32.0, 23.0 ],
					"fontname" : "Arial",
					"numinlets" : 1,
					"patching_rect" : [ 165.0, 65.0, 32.0, 23.0 ],
					"presentation" : 1,
					"numoutlets" : 3,
					"fontface" : 1,
					"fontsize" : 14.0,
					"items" : [ "a", ",", "e", ",", "i", ",", "o" ],
					"outlettype" : [ "int", "", "" ],
					"types" : [  ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "unpack 0.56 0.94 0.08 0.06",
					"id" : "obj-7",
					"fontname" : "Arial",
					"numinlets" : 1,
					"patching_rect" : [ 461.0, 231.0, 186.0, 20.0 ],
					"hidden" : 1,
					"numoutlets" : 4,
					"fontsize" : 12.0,
					"outlettype" : [ "float", "float", "float", "float" ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "prepend c",
					"id" : "obj-9",
					"fontname" : "Arial",
					"numinlets" : 1,
					"patching_rect" : [ 370.0, 130.0, 68.0, 20.0 ],
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "1",
					"id" : "obj-11",
					"fontname" : "Arial",
					"numinlets" : 2,
					"patching_rect" : [ 496.0, 104.0, 18.0, 18.0 ],
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "umenu",
					"id" : "obj-16",
					"presentation_rect" : [ 163.0, 133.0, 70.0, 20.0 ],
					"arrow" : 0,
					"fontname" : "Arial",
					"numinlets" : 1,
					"patching_rect" : [ 151.0, 138.0, 70.0, 20.0 ],
					"presentation" : 1,
					"numoutlets" : 3,
					"fontsize" : 12.0,
					"items" : [ "sin", ",", "tri", ",", "squ", "(bi)", ",", "squ", "(uni)", ",", "saw", "(up)", ",", "saw", "(down)" ],
					"outlettype" : [ "int", "", "" ],
					"types" : [  ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "lfoTyp $1",
					"id" : "obj-17",
					"presentation_rect" : [ 163.0, 154.0, 69.0, 18.0 ],
					"fontname" : "Arial",
					"numinlets" : 2,
					"patching_rect" : [ 151.0, 159.0, 69.0, 18.0 ],
					"presentation" : 1,
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "lfoAmp $1",
					"id" : "obj-18",
					"presentation_rect" : [ 88.0, 154.0, 74.0, 18.0 ],
					"fontname" : "Arial",
					"numinlets" : 2,
					"patching_rect" : [ 76.0, 159.0, 74.0, 18.0 ],
					"presentation" : 1,
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "flonum",
					"id" : "obj-19",
					"presentation_rect" : [ 88.0, 133.0, 52.0, 20.0 ],
					"fontname" : "Arial",
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"numinlets" : 1,
					"patching_rect" : [ 76.0, 138.0, 52.0, 20.0 ],
					"presentation" : 1,
					"numoutlets" : 2,
					"minimum" : 0.01,
					"fontsize" : 12.0,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ],
					"outlettype" : [ "float", "bang" ],
					"triscale" : 0.9
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "pvar cIN",
					"id" : "obj-20",
					"fontname" : "Arial",
					"numinlets" : 1,
					"patching_rect" : [ 3.0, 180.0, 61.0, 20.0 ],
					"hidden" : 1,
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "lfoFrq $1",
					"id" : "obj-21",
					"presentation_rect" : [ 15.0, 154.0, 67.0, 18.0 ],
					"fontname" : "Arial",
					"numinlets" : 2,
					"patching_rect" : [ 3.0, 159.0, 67.0, 18.0 ],
					"presentation" : 1,
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "flonum",
					"id" : "obj-22",
					"presentation_rect" : [ 15.0, 133.0, 52.0, 20.0 ],
					"fontname" : "Arial",
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"numinlets" : 1,
					"patching_rect" : [ 3.0, 138.0, 52.0, 20.0 ],
					"presentation" : 1,
					"numoutlets" : 2,
					"minimum" : 0.0,
					"fontsize" : 12.0,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ],
					"outlettype" : [ "float", "bang" ],
					"triscale" : 0.9
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "port $1",
					"id" : "obj-23",
					"presentation_rect" : [ 179.0, 238.0, 55.0, 18.0 ],
					"fontname" : "Arial",
					"numinlets" : 2,
					"patching_rect" : [ 628.0, 298.0, 55.0, 18.0 ],
					"presentation" : 1,
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "flonum",
					"id" : "obj-24",
					"presentation_rect" : [ 179.0, 216.0, 52.0, 20.0 ],
					"fontname" : "Arial",
					"bgcolor" : [ 0.74902, 1.0, 0.894118, 1.0 ],
					"numinlets" : 1,
					"patching_rect" : [ 628.0, 276.0, 52.0, 20.0 ],
					"presentation" : 1,
					"numoutlets" : 2,
					"minimum" : 0.01,
					"fontsize" : 12.0,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ],
					"outlettype" : [ "float", "bang" ],
					"triscale" : 0.9
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "tab $1",
					"id" : "obj-25",
					"presentation_rect" : [ 180.0, 84.0, 43.0, 18.0 ],
					"fontname" : "Arial",
					"numinlets" : 2,
					"patching_rect" : [ 165.0, 90.0, 43.0, 18.0 ],
					"presentation" : 1,
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "attn $1",
					"id" : "obj-28",
					"presentation_rect" : [ 124.0, 238.0, 54.0, 18.0 ],
					"fontname" : "Arial",
					"numinlets" : 2,
					"patching_rect" : [ 573.0, 298.0, 54.0, 18.0 ],
					"presentation" : 1,
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "flonum",
					"id" : "obj-29",
					"presentation_rect" : [ 124.0, 216.0, 52.0, 20.0 ],
					"fontname" : "Arial",
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"numinlets" : 1,
					"patching_rect" : [ 573.0, 276.0, 52.0, 20.0 ],
					"presentation" : 1,
					"numoutlets" : 2,
					"minimum" : 0.0,
					"fontsize" : 12.0,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ],
					"maximum" : 1.0,
					"outlettype" : [ "float", "bang" ],
					"triscale" : 0.9
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "time $1",
					"id" : "obj-30",
					"presentation_rect" : [ 66.0, 238.0, 57.0, 18.0 ],
					"fontname" : "Arial",
					"numinlets" : 2,
					"patching_rect" : [ 515.0, 298.0, 57.0, 18.0 ],
					"presentation" : 1,
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "flonum",
					"id" : "obj-31",
					"presentation_rect" : [ 66.0, 216.0, 52.0, 20.0 ],
					"fontname" : "Arial",
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"numinlets" : 1,
					"patching_rect" : [ 515.0, 276.0, 52.0, 20.0 ],
					"presentation" : 1,
					"numoutlets" : 2,
					"minimum" : 0.01,
					"fontsize" : 12.0,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ],
					"maximum" : 1.0,
					"outlettype" : [ "float", "bang" ],
					"triscale" : 0.9
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "pvar cIN",
					"id" : "obj-32",
					"fontname" : "Arial",
					"numinlets" : 1,
					"patching_rect" : [ 461.0, 322.0, 61.0, 20.0 ],
					"hidden" : 1,
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "rvb $1",
					"id" : "obj-33",
					"presentation_rect" : [ 12.0, 238.0, 51.0, 18.0 ],
					"fontname" : "Arial",
					"numinlets" : 2,
					"patching_rect" : [ 461.0, 298.0, 51.0, 18.0 ],
					"presentation" : 1,
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "flonum",
					"id" : "obj-34",
					"presentation_rect" : [ 12.0, 216.0, 52.0, 20.0 ],
					"fontname" : "Arial",
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"numinlets" : 1,
					"patching_rect" : [ 461.0, 276.0, 52.0, 20.0 ],
					"presentation" : 1,
					"numoutlets" : 2,
					"minimum" : 0.0,
					"fontsize" : 12.0,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ],
					"maximum" : 2.0,
					"outlettype" : [ "float", "bang" ],
					"triscale" : 0.9
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "pvar cIN",
					"id" : "obj-35",
					"fontname" : "Arial",
					"numinlets" : 1,
					"patching_rect" : [ 3.0, 111.0, 61.0, 20.0 ],
					"hidden" : 1,
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "oct $1",
					"id" : "obj-36",
					"presentation_rect" : [ 123.0, 81.0, 45.0, 18.0 ],
					"fontname" : "Arial",
					"numinlets" : 2,
					"patching_rect" : [ 111.0, 86.0, 45.0, 18.0 ],
					"presentation" : 1,
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "flonum",
					"id" : "obj-37",
					"presentation_rect" : [ 123.0, 60.0, 50.0, 20.0 ],
					"fontname" : "Arial",
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"numinlets" : 1,
					"patching_rect" : [ 111.0, 65.0, 50.0, 20.0 ],
					"presentation" : 1,
					"numoutlets" : 2,
					"minimum" : 0.0,
					"fontsize" : 12.0,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ],
					"outlettype" : [ "float", "bang" ],
					"triscale" : 0.9
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "form $1",
					"id" : "obj-38",
					"presentation_rect" : [ 68.0, 81.0, 52.0, 18.0 ],
					"fontname" : "Arial",
					"numinlets" : 2,
					"patching_rect" : [ 56.0, 86.0, 52.0, 18.0 ],
					"presentation" : 1,
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "flonum",
					"id" : "obj-39",
					"presentation_rect" : [ 68.0, 60.0, 54.0, 20.0 ],
					"fontname" : "Arial",
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"numinlets" : 1,
					"patching_rect" : [ 56.0, 65.0, 54.0, 20.0 ],
					"presentation" : 1,
					"numoutlets" : 2,
					"minimum" : 0.0,
					"fontsize" : 12.0,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ],
					"maximum" : 1.0,
					"outlettype" : [ "float", "bang" ],
					"triscale" : 0.9
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "fund $1",
					"id" : "obj-40",
					"presentation_rect" : [ 15.0, 81.0, 51.0, 18.0 ],
					"fontname" : "Arial",
					"numinlets" : 2,
					"patching_rect" : [ 3.0, 86.0, 51.0, 18.0 ],
					"presentation" : 1,
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "flonum",
					"id" : "obj-41",
					"presentation_rect" : [ 15.0, 60.0, 51.0, 20.0 ],
					"fontname" : "Arial",
					"bgcolor" : [ 1.0, 0.952941, 0.52549, 1.0 ],
					"numinlets" : 1,
					"patching_rect" : [ 3.0, 65.0, 51.0, 20.0 ],
					"presentation" : 1,
					"numoutlets" : 2,
					"minimum" : 0.0,
					"fontsize" : 12.0,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ],
					"maximum" : 127.0,
					"outlettype" : [ "float", "bang" ],
					"triscale" : 0.9
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"varname" : "cIN",
					"text" : "t l",
					"id" : "obj-42",
					"fontname" : "Arial",
					"numinlets" : 1,
					"patching_rect" : [ 370.0, 109.0, 21.0, 20.0 ],
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "t b b b",
					"id" : "obj-43",
					"fontname" : "Arial",
					"numinlets" : 1,
					"patching_rect" : [ 325.0, 40.0, 162.0, 20.0 ],
					"hidden" : 1,
					"numoutlets" : 3,
					"fontsize" : 12.0,
					"outlettype" : [ "bang", "bang", "bang" ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "loadbang",
					"id" : "obj-44",
					"fontname" : "Arial",
					"numinlets" : 1,
					"patching_rect" : [ 325.0, 21.0, 61.0, 20.0 ],
					"hidden" : 1,
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "bang" ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "preset",
					"id" : "obj-46",
					"presentation_rect" : [ 285.0, 180.0, 136.0, 41.0 ],
					"active2" : [ 0.0, 0.070588, 1.0, 1.0 ],
					"margin" : 4,
					"numinlets" : 1,
					"patching_rect" : [ 496.0, 127.0, 136.0, 39.0 ],
					"presentation" : 1,
					"numoutlets" : 4,
					"active1" : [ 0.0, 0.203922, 1.0, 1.0 ],
					"bubblesize" : 14,
					"emptycolor" : [ 1.0, 1.0, 1.0, 1.0 ],
					"outlettype" : [ "preset", "int", "preset", "int" ],
					"spacing" : 2,
					"preset_data" : [ 						{
							"number" : 1,
							"data" : [ 5, "obj-41", "flonum", "float", 51.0, 5, "obj-39", "flonum", "float", 0.539, 5, "obj-37", "flonum", "float", 0.0, 5, "obj-22", "flonum", "float", 1.79, 5, "obj-19", "flonum", "float", 0.15, 5, "obj-16", "umenu", "int", 1, 5, "obj-4", "umenu", "int", 2, 5, "obj-1", "flonum", "float", 0.0, 5, "obj-125", "number", "int", 1, 5, "obj-54", "number", "int", -3 ]
						}
, 						{
							"number" : 2,
							"data" : [ 5, "obj-41", "flonum", "float", 53.0, 5, "obj-39", "flonum", "float", 0.5513, 5, "obj-37", "flonum", "float", 0.0, 5, "obj-22", "flonum", "float", 0.4, 5, "obj-19", "flonum", "float", 0.23, 5, "obj-16", "umenu", "int", 1, 5, "obj-4", "umenu", "int", 1, 5, "obj-1", "flonum", "float", 0.0, 5, "obj-125", "number", "int", 1, 5, "obj-54", "number", "int", -3 ]
						}
, 						{
							"number" : 3,
							"data" : [ 5, "obj-41", "flonum", "float", 56.0, 5, "obj-39", "flonum", "float", 0.58, 5, "obj-37", "flonum", "float", 0.0, 5, "obj-22", "flonum", "float", 6.0, 5, "obj-19", "flonum", "float", 0.39, 5, "obj-16", "umenu", "int", 1, 5, "obj-4", "umenu", "int", 0, 5, "obj-1", "flonum", "float", 0.0, 5, "obj-125", "number", "int", 1, 5, "obj-54", "number", "int", -3 ]
						}
, 						{
							"number" : 4,
							"data" : [ 5, "obj-41", "flonum", "float", 60.0, 5, "obj-39", "flonum", "float", 0.599, 5, "obj-37", "flonum", "float", 0.0, 5, "obj-22", "flonum", "float", 3.2, 5, "obj-19", "flonum", "float", 0.25, 5, "obj-16", "umenu", "int", 1, 5, "obj-4", "umenu", "int", 0, 5, "obj-1", "flonum", "float", 0.0, 5, "obj-125", "number", "int", 1, 5, "obj-54", "number", "int", -3 ]
						}
, 						{
							"number" : 5,
							"data" : [ 5, "obj-41", "flonum", "float", 55.0, 5, "obj-39", "flonum", "float", 0.55, 5, "obj-37", "flonum", "float", 0.0, 5, "obj-22", "flonum", "float", 3.57, 5, "obj-19", "flonum", "float", 0.19, 5, "obj-16", "umenu", "int", 0, 5, "obj-4", "umenu", "int", 3, 5, "obj-1", "flonum", "float", 0.0, 5, "obj-125", "number", "int", 1, 5, "obj-54", "number", "int", -3 ]
						}
, 						{
							"number" : 6,
							"data" : [ 5, "obj-41", "flonum", "float", 55.0, 5, "obj-39", "flonum", "float", 0.58, 5, "obj-37", "flonum", "float", 0.0, 5, "obj-22", "flonum", "float", 3.57, 5, "obj-19", "flonum", "float", 0.19, 5, "obj-16", "umenu", "int", 0, 5, "obj-4", "umenu", "int", 0, 5, "obj-1", "flonum", "float", 0.0, 5, "obj-125", "number", "int", 1, 5, "obj-54", "number", "int", -3 ]
						}
, 						{
							"number" : 7,
							"data" : [ 5, "obj-41", "flonum", "float", 63.0, 5, "obj-39", "flonum", "float", 0.581, 5, "obj-37", "flonum", "float", 0.0, 5, "obj-22", "flonum", "float", 3.51, 5, "obj-19", "flonum", "float", 0.419, 5, "obj-16", "umenu", "int", 1, 5, "obj-4", "umenu", "int", 0, 5, "obj-1", "flonum", "float", 0.0, 5, "obj-125", "number", "int", 1, 5, "obj-54", "number", "int", -3 ]
						}
, 						{
							"number" : 8,
							"data" : [ 5, "obj-41", "flonum", "float", 65.0, 5, "obj-39", "flonum", "float", 0.581, 5, "obj-37", "flonum", "float", 0.15, 5, "obj-22", "flonum", "float", 0.19, 5, "obj-19", "flonum", "float", 0.419, 5, "obj-16", "umenu", "int", 1, 5, "obj-4", "umenu", "int", 0, 5, "obj-1", "flonum", "float", 0.0, 5, "obj-125", "number", "int", 1, 5, "obj-54", "number", "int", -3 ]
						}
 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "button",
					"id" : "obj-47",
					"fgcolor" : [ 0.156863, 0.8, 0.54902, 1.0 ],
					"numinlets" : 1,
					"patching_rect" : [ 325.0, 65.0, 15.0, 15.0 ],
					"numoutlets" : 1,
					"outlettype" : [ "bang" ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "stop",
					"id" : "obj-48",
					"fontname" : "Arial",
					"numinlets" : 2,
					"patching_rect" : [ 341.0, 85.0, 35.0, 18.0 ],
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "csound -m0 fof.csd",
					"id" : "obj-49",
					"fontname" : "Arial",
					"bgcolor" : [ 1.0, 0.890196, 0.090196, 1.0 ],
					"numinlets" : 2,
					"patching_rect" : [ 341.0, 65.0, 132.0, 18.0 ],
					"numoutlets" : 1,
					"fontsize" : 12.0,
					"outlettype" : [ "" ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "csound~",
					"id" : "obj-50",
					"fontname" : "Arial",
					"numinlets" : 2,
					"patching_rect" : [ 341.0, 154.0, 133.0, 20.0 ],
					"numoutlets" : 6,
					"fontsize" : 12.0,
					"outlettype" : [ "signal", "signal", "list", "int", "bang", "bang" ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "cIN",
					"id" : "obj-51",
					"textcolor" : [ 1.0, 0.035294, 0.035294, 1.0 ],
					"fontname" : "Arial",
					"numinlets" : 1,
					"patching_rect" : [ 348.0, 111.0, 30.0, 20.0 ],
					"numoutlets" : 0,
					"fontsize" : 12.0,
					"frgb" : [ 1.0, 0.035294, 0.035294, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "panel",
					"varname" : "autohelp_top_panel",
					"id" : "obj-64",
					"background" : 1,
					"presentation_rect" : [ 15.0, 0.0, 525.0, 45.0 ],
					"grad1" : [ 0.0, 0.0, 0.0, 1.0 ],
					"numinlets" : 1,
					"patching_rect" : [ 0.0, 0.0, 259.0, 47.0 ],
					"presentation" : 1,
					"grad2" : [ 0.0, 0.298039, 1.0, 1.0 ],
					"numoutlets" : 0,
					"mode" : 1
				}

			}
 ],
		"lines" : [ 			{
				"patchline" : 				{
					"source" : [ "obj-15", 0 ],
					"destination" : [ "obj-50", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-50", 0 ],
					"destination" : [ "obj-10", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-50", 1 ],
					"destination" : [ "obj-8", 1 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-50", 0 ],
					"destination" : [ "obj-8", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-54", 0 ],
					"destination" : [ "obj-8", 2 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-8", 1 ],
					"destination" : [ "obj-6", 1 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-8", 0 ],
					"destination" : [ "obj-6", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-41", 0 ],
					"destination" : [ "obj-40", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-36", 0 ],
					"destination" : [ "obj-35", 0 ],
					"hidden" : 1,
					"midpoints" : [ 120.5, 106.0, 12.5, 106.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-38", 0 ],
					"destination" : [ "obj-35", 0 ],
					"hidden" : 1,
					"midpoints" : [ 65.5, 106.0, 12.5, 106.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-40", 0 ],
					"destination" : [ "obj-35", 0 ],
					"hidden" : 1,
					"midpoints" : [ 12.5, 106.0, 12.5, 106.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-25", 0 ],
					"destination" : [ "obj-35", 0 ],
					"hidden" : 1,
					"midpoints" : [ 174.5, 106.0, 12.5, 106.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-3", 0 ],
					"destination" : [ "obj-35", 0 ],
					"hidden" : 1,
					"midpoints" : [ 271.5, 106.0, 12.5, 106.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-22", 0 ],
					"destination" : [ "obj-21", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-17", 0 ],
					"destination" : [ "obj-20", 0 ],
					"hidden" : 1,
					"midpoints" : [ 160.5, 177.0, 12.5, 177.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-21", 0 ],
					"destination" : [ "obj-20", 0 ],
					"hidden" : 1,
					"midpoints" : [ 12.5, 175.0, 12.5, 175.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-18", 0 ],
					"destination" : [ "obj-20", 0 ],
					"hidden" : 1,
					"midpoints" : [ 85.5, 177.0, 12.5, 177.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-39", 0 ],
					"destination" : [ "obj-38", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-19", 0 ],
					"destination" : [ "obj-18", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-37", 0 ],
					"destination" : [ "obj-36", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-16", 0 ],
					"destination" : [ "obj-17", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-4", 0 ],
					"destination" : [ "obj-25", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-1", 0 ],
					"destination" : [ "obj-2", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-2", 0 ],
					"destination" : [ "obj-3", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-44", 0 ],
					"destination" : [ "obj-43", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-43", 0 ],
					"destination" : [ "obj-47", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-43", 1 ],
					"destination" : [ "obj-49", 0 ],
					"hidden" : 1,
					"midpoints" : [ 406.0, 61.0, 350.5, 61.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-48", 0 ],
					"destination" : [ "obj-50", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-47", 0 ],
					"destination" : [ "obj-50", 0 ],
					"hidden" : 1,
					"midpoints" : [ 334.0, 119.0, 350.5, 119.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-49", 0 ],
					"destination" : [ "obj-50", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-9", 0 ],
					"destination" : [ "obj-50", 0 ],
					"hidden" : 0,
					"midpoints" : [ 379.5, 151.0, 350.5, 151.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-42", 0 ],
					"destination" : [ "obj-9", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-43", 2 ],
					"destination" : [ "obj-7", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-46", 2 ],
					"destination" : [ "obj-34", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-7", 0 ],
					"destination" : [ "obj-34", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-34", 0 ],
					"destination" : [ "obj-33", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-23", 0 ],
					"destination" : [ "obj-32", 0 ],
					"hidden" : 1,
					"midpoints" : [ 637.5, 317.0, 470.5, 317.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-28", 0 ],
					"destination" : [ "obj-32", 0 ],
					"hidden" : 1,
					"midpoints" : [ 582.5, 317.0, 470.5, 317.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-30", 0 ],
					"destination" : [ "obj-32", 0 ],
					"hidden" : 1,
					"midpoints" : [ 524.5, 317.0, 470.5, 317.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-33", 0 ],
					"destination" : [ "obj-32", 0 ],
					"hidden" : 1,
					"midpoints" : [ 470.5, 315.0, 470.5, 315.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-46", 2 ],
					"destination" : [ "obj-31", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-7", 1 ],
					"destination" : [ "obj-31", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-31", 0 ],
					"destination" : [ "obj-30", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-43", 2 ],
					"destination" : [ "obj-11", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-11", 0 ],
					"destination" : [ "obj-46", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-46", 2 ],
					"destination" : [ "obj-29", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-7", 2 ],
					"destination" : [ "obj-29", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-29", 0 ],
					"destination" : [ "obj-28", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-46", 2 ],
					"destination" : [ "obj-24", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-7", 3 ],
					"destination" : [ "obj-24", 0 ],
					"hidden" : 1,
					"midpoints" : [ 637.5, 271.0, 637.5, 271.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-24", 0 ],
					"destination" : [ "obj-23", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-126", 0 ],
					"destination" : [ "obj-127", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-125", 0 ],
					"destination" : [ "obj-127", 1 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-127", 0 ],
					"destination" : [ "obj-46", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
 ]
	}

}
