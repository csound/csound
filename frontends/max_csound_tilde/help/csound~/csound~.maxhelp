{
	"patcher" : 	{
		"fileversion" : 1,
		"rect" : [ 171.0, 51.0, 1002.0, 572.0 ],
		"bglocked" : 0,
		"defrect" : [ 171.0, 51.0, 1002.0, 572.0 ],
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
					"maxclass" : "newobj",
					"varname" : "init_midi",
					"text" : "t l",
					"numinlets" : 1,
					"hidden" : 1,
					"numoutlets" : 1,
					"patching_rect" : [ 239.0, 394.0, 20.0, 20.0 ],
					"outlettype" : [ "" ],
					"id" : "obj-20",
					"fontname" : "Arial",
					"fontsize" : 12.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "p scripting",
					"numinlets" : 0,
					"numoutlets" : 0,
					"patching_rect" : [ 790.0, 299.0, 67.0, 20.0 ],
					"id" : "obj-5",
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"patcher" : 					{
						"fileversion" : 1,
						"rect" : [ 242.0, 306.0, 980.0, 401.0 ],
						"bglocked" : 0,
						"defrect" : [ 242.0, 306.0, 980.0, 401.0 ],
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
									"maxclass" : "comment",
									"text" : "The parse message allows you to create Max objects by including scripting statements in your csd file.  Each statement must have this format:\n\n<~> @maxclass objectclass ... </~>\n\n ",
									"linecount" : 6,
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 450.0, 270.0, 459.0, 89.0 ],
									"id" : "obj-77",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "3",
									"numinlets" : 1,
									"frgb" : [ 0.176471, 0.301961, 1.0, 1.0 ],
									"textcolor" : [ 0.176471, 0.301961, 1.0, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 150.0, 210.0, 38.0, 48.0 ],
									"id" : "obj-75",
									"fontname" : "Arial",
									"fontface" : 1,
									"fontsize" : 36.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "2",
									"numinlets" : 1,
									"frgb" : [ 0.176471, 0.301961, 1.0, 1.0 ],
									"textcolor" : [ 0.176471, 0.301961, 1.0, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 105.0, 210.0, 38.0, 48.0 ],
									"id" : "obj-74",
									"fontname" : "Arial",
									"fontface" : 1,
									"fontsize" : 36.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "1",
									"numinlets" : 1,
									"frgb" : [ 0.176471, 0.301961, 1.0, 1.0 ],
									"textcolor" : [ 0.176471, 0.301961, 1.0, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 30.0, 330.0, 38.0, 48.0 ],
									"id" : "obj-54",
									"fontname" : "Arial",
									"fontface" : 1,
									"fontsize" : 36.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "3) Parse the csd file.",
									"numinlets" : 1,
									"frgb" : [ 0.0, 0.14902, 0.964706, 1.0 ],
									"textcolor" : [ 0.0, 0.14902, 0.964706, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 240.0, 365.0, 163.0, 23.0 ],
									"id" : "obj-35",
									"fontname" : "Arial",
									"fontsize" : 14.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "2) Start [csound~].",
									"numinlets" : 1,
									"frgb" : [ 0.0, 0.14902, 0.964706, 1.0 ],
									"textcolor" : [ 0.0, 0.14902, 0.964706, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 240.0, 340.0, 142.0, 23.0 ],
									"id" : "obj-33",
									"fontname" : "Arial",
									"fontsize" : 14.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "1) Start DSP. ",
									"numinlets" : 1,
									"frgb" : [ 0.0, 0.14902, 0.964706, 1.0 ],
									"textcolor" : [ 0.0, 0.14902, 0.964706, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 240.0, 315.0, 118.0, 23.0 ],
									"id" : "obj-34",
									"fontname" : "Arial",
									"fontsize" : 14.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "ezdac~",
									"numinlets" : 2,
									"bgcolor" : [ 0.0, 0.0, 0.0, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 60.0, 330.0, 44.0, 44.0 ],
									"offgradcolor1" : [ 0.392157, 0.392157, 0.392157, 1.0 ],
									"ongradcolor1" : [ 1.0, 0.0, 0.0, 1.0 ],
									"id" : "obj-118",
									"ongradcolor2" : [ 1.0, 1.0, 0.133333, 1.0 ],
									"offgradcolor2" : [ 0.0, 0.0, 0.0, 1.0 ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "scope~",
									"numinlets" : 2,
									"bgcolor" : [ 0.0, 0.0, 0.0, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 105.0, 330.0, 124.0, 60.0 ],
									"id" : "obj-53",
									"bufsize" : 64,
									"calccount" : 128
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "csound~",
									"numinlets" : 1,
									"frgb" : [ 0.815686, 0.0, 0.0, 1.0 ],
									"textcolor" : [ 0.815686, 0.0, 0.0, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 2.0, 301.0, 61.0, 20.0 ],
									"id" : "obj-26",
									"fontname" : "Arial",
									"fontface" : 2,
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "parse tosub",
									"numinlets" : 2,
									"numoutlets" : 1,
									"patching_rect" : [ 195.0, 255.0, 76.0, 18.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-5",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "parse",
									"numinlets" : 2,
									"numoutlets" : 1,
									"patching_rect" : [ 150.0, 255.0, 42.0, 18.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-4",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "start",
									"numinlets" : 2,
									"numoutlets" : 1,
									"patching_rect" : [ 105.0, 255.0, 34.0, 18.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-3",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "stop",
									"numinlets" : 2,
									"numoutlets" : 1,
									"patching_rect" : [ 60.0, 255.0, 34.0, 18.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-2",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"varname" : "csound~",
									"text" : "csound~ @args scripting.csd",
									"numinlets" : 2,
									"numoutlets" : 6,
									"patching_rect" : [ 60.0, 300.0, 171.0, 20.0 ],
									"outlettype" : [ "signal", "signal", "list", "int", "bang", "bang" ],
									"id" : "obj-1",
									"fontname" : "Arial",
									"fontsize" : 12.0,
									"saved_object_attributes" : 									{
										"matchsr" : 1,
										"bypass" : 0,
										"overdrive" : 0,
										"output" : 1,
										"input" : 1,
										"interval" : 10,
										"message" : 1,
										"autostart" : 0,
										"args" : "scripting.csd"
									}

								}

							}
 ],
						"lines" : [ 							{
								"patchline" : 								{
									"source" : [ "obj-1", 1 ],
									"destination" : [ "obj-53", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-1", 1 ],
									"destination" : [ "obj-118", 1 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-1", 0 ],
									"destination" : [ "obj-118", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-5", 0 ],
									"destination" : [ "obj-1", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-4", 0 ],
									"destination" : [ "obj-1", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-2", 0 ],
									"destination" : [ "obj-1", 0 ],
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
 ]
					}
,
					"saved_object_attributes" : 					{
						"default_fontname" : "Arial",
						"default_fontsize" : 12.0,
						"fontname" : "Arial",
						"globalpatchername" : "",
						"fontface" : 0,
						"fontsize" : 12.0,
						"default_fontface" : 0
					}

				}

			}
, 			{
				"box" : 				{
					"maxclass" : "number",
					"numinlets" : 1,
					"numoutlets" : 2,
					"patching_rect" : [ 678.0, 465.0, 42.0, 20.0 ],
					"outlettype" : [ "int", "bang" ],
					"id" : "obj-79",
					"fontname" : "Arial",
					"fontface" : 1,
					"fontsize" : 12.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "thispatcher",
					"numinlets" : 1,
					"hidden" : 1,
					"numoutlets" : 2,
					"patching_rect" : [ 285.0, 131.0, 71.0, 20.0 ],
					"outlettype" : [ "", "" ],
					"id" : "obj-91",
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"save" : [ "#N", "thispatcher", ";", "#Q", "end", ";" ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "dirty",
					"numinlets" : 2,
					"hidden" : 1,
					"numoutlets" : 1,
					"patching_rect" : [ 285.0, 108.0, 32.5, 18.0 ],
					"outlettype" : [ "" ],
					"id" : "obj-58",
					"fontname" : "Arial",
					"fontsize" : 12.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "Object names are red.",
					"numinlets" : 1,
					"frgb" : [ 0.815686, 0.0, 0.0, 1.0 ],
					"textcolor" : [ 0.815686, 0.0, 0.0, 1.0 ],
					"numoutlets" : 0,
					"patching_rect" : [ 825.0, 420.0, 135.0, 20.0 ],
					"id" : "obj-57",
					"fontname" : "Arial",
					"fontface" : 2,
					"fontsize" : 12.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "bypass $1",
					"numinlets" : 2,
					"bgcolor" : [ 1.0, 0.992157, 0.94902, 1.0 ],
					"numoutlets" : 1,
					"patching_rect" : [ 398.0, 162.0, 67.0, 18.0 ],
					"outlettype" : [ "" ],
					"id" : "obj-34",
					"fontname" : "Arial",
					"fontsize" : 12.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "toggle",
					"numinlets" : 1,
					"numoutlets" : 1,
					"patching_rect" : [ 382.0, 162.0, 15.0, 15.0 ],
					"outlettype" : [ "int" ],
					"id" : "obj-35"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "Disable audio and MIDI processing.",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 467.0, 162.0, 245.0, 20.0 ],
					"id" : "obj-56",
					"fontname" : "Arial",
					"fontsize" : 12.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "pattrforward csound~",
					"numinlets" : 1,
					"hidden" : 1,
					"numoutlets" : 1,
					"patching_rect" : [ 347.0, 273.0, 127.0, 20.0 ],
					"outlettype" : [ "" ],
					"id" : "obj-33",
					"fontname" : "Arial",
					"fontsize" : 12.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"varname" : "autohelp_top_title",
					"text" : "csound~ v1.1.3",
					"presentation_linecount" : 2,
					"numinlets" : 1,
					"presentation_rect" : [ 50.0, 33.0, 121.0, 75.0 ],
					"frgb" : [ 0.93, 0.93, 0.97, 1.0 ],
					"textcolor" : [ 0.93, 0.93, 0.97, 1.0 ],
					"numoutlets" : 0,
					"patching_rect" : [ 20.0, 3.0, 228.0, 41.0 ],
					"presentation" : 1,
					"id" : "obj-24",
					"fontname" : "Arial",
					"fontface" : 3,
					"fontsize" : 30.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "amp",
					"numinlets" : 1,
					"frgb" : [ 0.815686, 0.0, 0.0, 1.0 ],
					"textcolor" : [ 0.815686, 0.0, 0.0, 1.0 ],
					"numoutlets" : 0,
					"patching_rect" : [ 467.0, 434.0, 43.0, 20.0 ],
					"id" : "obj-54",
					"fontname" : "Arial",
					"fontface" : 2,
					"fontsize" : 12.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "number",
					"varname" : "amp",
					"numinlets" : 1,
					"numoutlets" : 2,
					"patching_rect" : [ 418.0, 435.0, 50.0, 20.0 ],
					"outlettype" : [ "int", "bang" ],
					"id" : "obj-14",
					"fontname" : "Arial",
					"maximum" : 12,
					"fontsize" : 12.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "p amp",
					"numinlets" : 3,
					"numoutlets" : 2,
					"patching_rect" : [ 360.0, 435.0, 56.0, 20.0 ],
					"outlettype" : [ "signal", "signal" ],
					"id" : "obj-21",
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"patcher" : 					{
						"fileversion" : 1,
						"rect" : [ 20.0, 74.0, 350.0, 322.0 ],
						"bglocked" : 0,
						"defrect" : [ 20.0, 74.0, 350.0, 322.0 ],
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
									"maxclass" : "outlet",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 90.0, 148.0, 21.0, 21.0 ],
									"id" : "obj-1",
									"comment" : ""
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "outlet",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 33.0, 148.0, 21.0, 21.0 ],
									"id" : "obj-2",
									"comment" : ""
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "dbtoa",
									"numinlets" : 1,
									"numoutlets" : 1,
									"patching_rect" : [ 129.0, 70.0, 48.0, 20.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-3",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "*~ 1.",
									"numinlets" : 2,
									"numoutlets" : 1,
									"patching_rect" : [ 90.0, 105.0, 49.0, 20.0 ],
									"outlettype" : [ "signal" ],
									"id" : "obj-4",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "*~ 1.",
									"numinlets" : 2,
									"numoutlets" : 1,
									"patching_rect" : [ 33.0, 105.0, 49.0, 20.0 ],
									"outlettype" : [ "signal" ],
									"id" : "obj-5",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "inlet",
									"numinlets" : 0,
									"numoutlets" : 1,
									"patching_rect" : [ 129.0, 41.0, 21.0, 21.0 ],
									"outlettype" : [ "int" ],
									"id" : "obj-6",
									"comment" : ""
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "inlet",
									"numinlets" : 0,
									"numoutlets" : 1,
									"patching_rect" : [ 90.0, 41.0, 21.0, 21.0 ],
									"outlettype" : [ "signal" ],
									"id" : "obj-7",
									"comment" : ""
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "inlet",
									"numinlets" : 0,
									"numoutlets" : 1,
									"patching_rect" : [ 33.0, 41.0, 21.0, 21.0 ],
									"outlettype" : [ "signal" ],
									"id" : "obj-8",
									"comment" : ""
								}

							}
 ],
						"lines" : [ 							{
								"patchline" : 								{
									"source" : [ "obj-3", 0 ],
									"destination" : [ "obj-4", 1 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-6", 0 ],
									"destination" : [ "obj-3", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-4", 0 ],
									"destination" : [ "obj-1", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-7", 0 ],
									"destination" : [ "obj-4", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-3", 0 ],
									"destination" : [ "obj-5", 1 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-5", 0 ],
									"destination" : [ "obj-2", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-8", 0 ],
									"destination" : [ "obj-5", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
 ]
					}
,
					"saved_object_attributes" : 					{
						"default_fontname" : "Arial",
						"default_fontsize" : 12.0,
						"fontname" : "Arial",
						"globalpatchername" : "",
						"fontface" : 0,
						"fontsize" : 12.0,
						"default_fontface" : 0
					}

				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "messages",
					"numinlets" : 1,
					"frgb" : [ 0.815686, 0.0, 0.0, 1.0 ],
					"textcolor" : [ 0.815686, 0.0, 0.0, 1.0 ],
					"numoutlets" : 0,
					"patching_rect" : [ 211.0, 256.0, 71.0, 20.0 ],
					"id" : "obj-12",
					"fontname" : "Arial",
					"fontface" : 2,
					"fontsize" : 12.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "pattrforward messages",
					"numinlets" : 1,
					"numoutlets" : 1,
					"patching_rect" : [ 489.0, 413.0, 138.0, 20.0 ],
					"outlettype" : [ "" ],
					"id" : "obj-11",
					"fontname" : "Arial",
					"fontsize" : 12.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "pattrforward init_midi",
					"numinlets" : 1,
					"numoutlets" : 1,
					"patching_rect" : [ 554.0, 388.0, 125.0, 20.0 ],
					"outlettype" : [ "" ],
					"id" : "obj-31",
					"fontname" : "Arial",
					"fontsize" : 12.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "csound~",
					"numinlets" : 1,
					"frgb" : [ 0.815686, 0.0, 0.0, 1.0 ],
					"textcolor" : [ 0.815686, 0.0, 0.0, 1.0 ],
					"numoutlets" : 0,
					"patching_rect" : [ 604.0, 360.0, 63.0, 20.0 ],
					"id" : "obj-26",
					"fontname" : "Arial",
					"fontface" : 2,
					"fontsize" : 12.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "init_midi",
					"numinlets" : 1,
					"hidden" : 1,
					"frgb" : [ 0.815686, 0.0, 0.0, 1.0 ],
					"textcolor" : [ 0.815686, 0.0, 0.0, 1.0 ],
					"numoutlets" : 0,
					"patching_rect" : [ 261.0, 394.0, 59.0, 20.0 ],
					"id" : "obj-22",
					"fontname" : "Arial",
					"fontface" : 2,
					"fontsize" : 12.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "multislider",
					"numinlets" : 1,
					"candicane8" : [ 0.027451, 0.447059, 0.501961, 1.0 ],
					"setstyle" : 5,
					"candicane2" : [ 0.145098, 0.203922, 0.356863, 1.0 ],
					"numoutlets" : 2,
					"candicane7" : [ 0.878431, 0.243137, 0.145098, 1.0 ],
					"patching_rect" : [ 540.0, 465.0, 135.0, 101.0 ],
					"outlettype" : [ "", "" ],
					"candicane6" : [ 0.733333, 0.035294, 0.788235, 1.0 ],
					"id" : "obj-23",
					"candicane5" : [ 0.584314, 0.827451, 0.431373, 1.0 ],
					"candicane4" : [ 0.439216, 0.619608, 0.070588, 1.0 ],
					"peakcolor" : [ 1.0, 0.098039, 0.117647, 1.0 ],
					"setminmax" : [ 0.0, 100.0 ],
					"candicane3" : [ 0.290196, 0.411765, 0.713726, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "p cpu usage",
					"numinlets" : 0,
					"hidden" : 1,
					"numoutlets" : 1,
					"patching_rect" : [ 677.0, 485.0, 78.0, 20.0 ],
					"outlettype" : [ "" ],
					"id" : "obj-7",
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"patcher" : 					{
						"fileversion" : 1,
						"rect" : [ 426.0, 289.0, 147.0, 210.0 ],
						"bglocked" : 0,
						"defrect" : [ 426.0, 289.0, 147.0, 210.0 ],
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
									"text" : "loadmess 1",
									"numinlets" : 1,
									"numoutlets" : 1,
									"patching_rect" : [ 16.0, 13.0, 76.0, 20.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-3",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "outlet",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 16.0, 103.0, 24.0, 24.0 ],
									"id" : "obj-2",
									"comment" : ""
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "metro 100",
									"numinlets" : 2,
									"numoutlets" : 1,
									"patching_rect" : [ 16.0, 43.0, 75.0, 20.0 ],
									"outlettype" : [ "bang" ],
									"id" : "obj-4",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "adstatus cpu",
									"numinlets" : 2,
									"numoutlets" : 2,
									"patching_rect" : [ 16.0, 73.0, 85.0, 20.0 ],
									"outlettype" : [ "", "int" ],
									"id" : "obj-5",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
 ],
						"lines" : [ 							{
								"patchline" : 								{
									"source" : [ "obj-3", 0 ],
									"destination" : [ "obj-4", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-4", 0 ],
									"destination" : [ "obj-5", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-5", 0 ],
									"destination" : [ "obj-2", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
 ]
					}
,
					"saved_object_attributes" : 					{
						"default_fontname" : "Arial",
						"default_fontsize" : 12.0,
						"fontname" : "Arial",
						"globalpatchername" : "",
						"fontface" : 0,
						"fontsize" : 12.0,
						"default_fontface" : 0
					}

				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "cpu usage",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 719.0, 463.0, 67.0, 20.0 ],
					"id" : "obj-28",
					"fontname" : "Arial",
					"fontsize" : 12.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : ";\rdsp open",
					"linecount" : 2,
					"numinlets" : 2,
					"bgcolor" : [ 1.0, 0.701961, 0.0, 1.0 ],
					"numoutlets" : 1,
					"patching_rect" : [ 360.0, 534.0, 62.0, 32.0 ],
					"outlettype" : [ "" ],
					"id" : "obj-30",
					"fontname" : "Arial",
					"fontsize" : 12.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "ezdac~",
					"numinlets" : 2,
					"bgcolor" : [ 0.0, 0.0, 0.0, 1.0 ],
					"numoutlets" : 0,
					"patching_rect" : [ 360.0, 465.0, 65.0, 65.0 ],
					"offgradcolor1" : [ 0.392157, 0.392157, 0.392157, 1.0 ],
					"ongradcolor1" : [ 1.0, 0.0, 0.0, 1.0 ],
					"id" : "obj-118",
					"ongradcolor2" : [ 1.0, 1.0, 0.133333, 1.0 ],
					"offgradcolor2" : [ 0.0, 0.0, 0.0, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "p \"chn string channels\"",
					"numinlets" : 0,
					"numoutlets" : 0,
					"patching_rect" : [ 789.0, 127.0, 134.0, 20.0 ],
					"id" : "obj-6",
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"patcher" : 					{
						"fileversion" : 1,
						"rect" : [ 277.0, 293.0, 652.0, 444.0 ],
						"bglocked" : 0,
						"defrect" : [ 277.0, 293.0, 652.0, 444.0 ],
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
									"maxclass" : "comment",
									"text" : "Since my_string is also an output channel, whenever it's value changes, it will be output here.",
									"linecount" : 2,
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 193.0, 338.0, 287.0, 34.0 ],
									"id" : "obj-17",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "Set my_string channel.",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 422.0, 277.0, 139.0, 20.0 ],
									"id" : "obj-16",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "Set my_string channel.",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 295.0, 250.0, 139.0, 20.0 ],
									"id" : "obj-15",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "Print my_string channel value to Max window.",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 226.0, 218.0, 259.0, 20.0 ],
									"id" : "obj-14",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "Set my_string channel to \"Aaaahhhhchoo!\".",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 203.0, 189.0, 244.0, 20.0 ],
									"id" : "obj-13",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "Set my_string channel to \"Hello World\".",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 168.0, 160.0, 244.0, 20.0 ],
									"id" : "obj-12",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "e i3 0 1",
									"numinlets" : 2,
									"bgcolor" : [ 1.0, 1.0, 1.0, 1.0 ],
									"numoutlets" : 1,
									"patching_rect" : [ 175.0, 218.0, 50.0, 18.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-10",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "3",
									"numinlets" : 1,
									"frgb" : [ 0.0, 0.05098, 1.0, 1.0 ],
									"textcolor" : [ 0.0, 0.05098, 1.0, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 122.0, 113.0, 38.0, 48.0 ],
									"id" : "obj-9",
									"fontname" : "Arial",
									"fontface" : 1,
									"fontsize" : 36.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "1) Start DSP.\n2) Start Csound.\n3) Send some messages, see what happens.",
									"linecount" : 3,
									"numinlets" : 1,
									"frgb" : [ 0.0, 0.05098, 1.0, 1.0 ],
									"textcolor" : [ 0.0, 0.05098, 1.0, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 20.0, 61.0, 274.0, 48.0 ],
									"id" : "obj-8",
									"fontname" : "Arial",
									"fontface" : 1,
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "1",
									"numinlets" : 1,
									"frgb" : [ 0.0, 0.05098, 1.0, 1.0 ],
									"textcolor" : [ 0.0, 0.05098, 1.0, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 90.0, 360.0, 38.0, 48.0 ],
									"id" : "obj-7",
									"fontname" : "Arial",
									"fontface" : 1,
									"fontsize" : 36.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "2",
									"numinlets" : 1,
									"frgb" : [ 0.0, 0.05098, 1.0, 1.0 ],
									"textcolor" : [ 0.0, 0.05098, 1.0, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 9.0, 256.0, 38.0, 48.0 ],
									"id" : "obj-6",
									"fontname" : "Arial",
									"fontface" : 1,
									"fontsize" : 36.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "button",
									"numinlets" : 1,
									"numoutlets" : 1,
									"patching_rect" : [ 38.0, 246.0, 66.0, 66.0 ],
									"outlettype" : [ "bang" ],
									"id" : "obj-4"
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "my_string \"H:/Projects/Fun with fire.aiff\"",
									"numinlets" : 2,
									"bgcolor" : [ 1.0, 1.0, 1.0, 1.0 ],
									"numoutlets" : 1,
									"patching_rect" : [ 182.0, 401.0, 463.0, 32.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-3",
									"fontname" : "Arial",
									"fontsize" : 24.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "prepend set",
									"numinlets" : 1,
									"numoutlets" : 1,
									"patching_rect" : [ 182.0, 377.0, 74.0, 20.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-2",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "ezdac~",
									"numinlets" : 2,
									"bgcolor" : [ 0.0, 0.0, 0.0, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 117.0, 358.0, 52.0, 52.0 ],
									"offgradcolor1" : [ 0.392157, 0.392157, 0.392157, 1.0 ],
									"ongradcolor1" : [ 1.0, 0.0, 0.0, 1.0 ],
									"id" : "obj-118",
									"ongradcolor2" : [ 1.0, 1.0, 0.133333, 1.0 ],
									"offgradcolor2" : [ 0.0, 0.0, 0.0, 1.0 ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "c my_string \"H:/Projects/Fun with fire.aiff\"",
									"numinlets" : 2,
									"bgcolor" : [ 1.0, 1.0, 1.0, 1.0 ],
									"numoutlets" : 1,
									"patching_rect" : [ 192.0, 278.0, 230.0, 18.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-63",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "c my_string Whoa!!!",
									"numinlets" : 2,
									"bgcolor" : [ 1.0, 1.0, 1.0, 1.0 ],
									"numoutlets" : 1,
									"patching_rect" : [ 177.0, 251.0, 117.0, 18.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-62",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "e i2 0 1",
									"numinlets" : 2,
									"bgcolor" : [ 1.0, 1.0, 1.0, 1.0 ],
									"numoutlets" : 1,
									"patching_rect" : [ 151.0, 189.0, 50.0, 18.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-61",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "e i1 0 1",
									"numinlets" : 2,
									"bgcolor" : [ 1.0, 1.0, 1.0, 1.0 ],
									"numoutlets" : 1,
									"patching_rect" : [ 117.0, 160.0, 50.0, 18.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-60",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "chn string channels",
									"numinlets" : 1,
									"frgb" : [ 0.337255, 0.254902, 0.113725, 1.0 ],
									"textcolor" : [ 0.337255, 0.254902, 0.113725, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 19.0, 10.0, 332.0, 48.0 ],
									"id" : "obj-1",
									"fontname" : "Arial",
									"fontsize" : 36.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "csound~ chnstring.csd",
									"numinlets" : 2,
									"numoutlets" : 6,
									"patching_rect" : [ 117.0, 316.0, 182.0, 20.0 ],
									"outlettype" : [ "signal", "signal", "list", "int", "bang", "bang" ],
									"id" : "obj-52",
									"fontname" : "Arial",
									"fontsize" : 12.0,
									"saved_object_attributes" : 									{
										"matchsr" : 1,
										"bypass" : 0,
										"overdrive" : 0,
										"output" : 1,
										"input" : 1,
										"interval" : 10,
										"message" : 1,
										"autostart" : 0,
										"args" : "chnstring.csd"
									}

								}

							}
, 							{
								"box" : 								{
									"maxclass" : "panel",
									"numinlets" : 1,
									"bgcolor" : [ 0.866667, 0.882353, 0.847059, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 14.0, 6.0, 331.0, 56.0 ],
									"id" : "obj-18"
								}

							}
 ],
						"lines" : [ 							{
								"patchline" : 								{
									"source" : [ "obj-10", 0 ],
									"destination" : [ "obj-52", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-4", 0 ],
									"destination" : [ "obj-52", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-52", 0 ],
									"destination" : [ "obj-118", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-52", 1 ],
									"destination" : [ "obj-118", 1 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-63", 0 ],
									"destination" : [ "obj-52", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-62", 0 ],
									"destination" : [ "obj-52", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-61", 0 ],
									"destination" : [ "obj-52", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-60", 0 ],
									"destination" : [ "obj-52", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-52", 2 ],
									"destination" : [ "obj-2", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-2", 0 ],
									"destination" : [ "obj-3", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
 ]
					}
,
					"saved_object_attributes" : 					{
						"default_fontname" : "Arial",
						"default_fontsize" : 12.0,
						"fontname" : "Arial",
						"globalpatchername" : "",
						"fontface" : 0,
						"fontsize" : 12.0,
						"default_fontface" : 0
					}

				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "p \"csound~ arguments\"",
					"numinlets" : 0,
					"numoutlets" : 0,
					"patching_rect" : [ 789.0, 105.0, 139.0, 20.0 ],
					"id" : "obj-29",
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"patcher" : 					{
						"fileversion" : 1,
						"rect" : [ 625.0, 273.0, 541.0, 443.0 ],
						"bglocked" : 0,
						"defrect" : [ 625.0, 273.0, 541.0, 443.0 ],
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
									"maxclass" : "comment",
									"text" : "csound~ arguments",
									"numinlets" : 1,
									"frgb" : [ 0.301961, 0.294118, 0.376471, 1.0 ],
									"textcolor" : [ 0.301961, 0.294118, 0.376471, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 8.0, 2.0, 172.0, 25.0 ],
									"id" : "obj-27",
									"fontname" : "Arial",
									"fontface" : 1,
									"fontsize" : 16.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "You can specify attributes and the csound command arguments within the [csound~] box.",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 17.0, 32.0, 503.0, 20.0 ],
									"id" : "obj-16",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "Even if you add the Csound message to [csound~] argument list, you must still send \"start\" or \"bang\" to compile and start the performance (unless you added \"@autostart 1\").",
									"linecount" : 2,
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 14.0, 188.0, 507.0, 34.0 ],
									"id" : "obj-1",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "Specify csound command arguments.\nEnable/Disable auto compile & start (default = 0)\nEnable/Disable audio and MIDI processing (default = 0)\n# of input audio channels (default = 2)\nEnable/Disable input control processing (default = 1)\nSet interval for rate of output control msg's (default = 10ms)\n# of input/output audio channels (default = 2)\nAuto recompile if Max and Csound sample-rates are mismatched (default = 1)\nTells you if csound~ is processing audio without latency (not editable).\n# of output audio channels (default = 2)\nEnable/Disable output control  processing (default = 1)\nEnable/Disable overdrive mode (default = 0)\nEnable/Disable output to Max window (default = 1)",
									"linecount" : 13,
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 90.0, 244.0, 439.0, 186.0 ],
									"id" : "obj-4",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "@args\n@autostart\n@bypass\n@i\n@input\n@interval\n@io\n@matchsr\n@nolatency \n@o\n@output @overdrive   @message",
									"linecount" : 13,
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 15.0, 244.0, 80.0, 186.0 ],
									"id" : "obj-5",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "Attributes:",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 14.0, 226.0, 88.0, 23.0 ],
									"id" : "obj-7",
									"fontname" : "Arial",
									"fontface" : 3,
									"fontsize" : 14.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "[csound~ @args \"badkitty.csd -m0\"]",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 137.0, 121.0, 277.0, 20.0 ],
									"id" : "obj-8",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "[csound~ @args hello.csd @io 4 @overdrive 1]",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 137.0, 85.0, 274.0, 20.0 ],
									"id" : "obj-9",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "If the csound command has spaces, put double-quotes around it.",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 14.0, 163.0, 506.0, 20.0 ],
									"id" : "obj-10",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "[csound~ @i 2 @o 4 @args \"fun.orc fun.sco\"]",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 137.0, 103.0, 276.0, 20.0 ],
									"id" : "obj-12",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "panel",
									"numinlets" : 1,
									"bgcolor" : [ 1.0, 1.0, 0.556863, 1.0 ],
									"numoutlets" : 0,
									"border" : 2,
									"bordercolor" : [ 0.85098, 0.85098, 0.85098, 1.0 ],
									"patching_rect" : [ 135.0, 81.0, 289.0, 66.0 ],
									"id" : "obj-13",
									"rounded" : 0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "Examples:",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 135.0, 63.0, 100.0, 20.0 ],
									"id" : "obj-14",
									"fontname" : "Arial",
									"fontface" : 2,
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "panel",
									"numinlets" : 1,
									"bgcolor" : [ 1.0, 1.0, 1.0, 1.0 ],
									"numoutlets" : 0,
									"border" : 2,
									"bordercolor" : [ 0.666667, 0.666667, 0.666667, 1.0 ],
									"patching_rect" : [ 8.0, 25.0, 526.0, 411.0 ],
									"id" : "obj-28",
									"rounded" : 12
								}

							}
 ],
						"lines" : [  ]
					}
,
					"saved_object_attributes" : 					{
						"default_fontname" : "Arial",
						"default_fontsize" : 12.0,
						"fontname" : "Arial",
						"globalpatchername" : "",
						"fontface" : 0,
						"fontsize" : 12.0,
						"default_fontface" : 0
					}

				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "p \"run message\"",
					"numinlets" : 0,
					"numoutlets" : 0,
					"patching_rect" : [ 790.0, 278.0, 100.0, 20.0 ],
					"id" : "obj-27",
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"patcher" : 					{
						"fileversion" : 1,
						"rect" : [ 342.0, 214.0, 865.0, 357.0 ],
						"bglocked" : 0,
						"defrect" : [ 342.0, 214.0, 865.0, 357.0 ],
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
									"maxclass" : "comment",
									"text" : "You can use either Max style or POSIX style paths.",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 390.0, 320.0, 355.0, 20.0 ],
									"id" : "obj-1",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "If a path contains spaces, put double-quotes around it.",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 546.0, 146.0, 305.0, 20.0 ],
									"id" : "obj-20",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "In Mac OSX, many apps are actually folders.  To get the actual binary executable, right click the app in Finder, select \"Show Package Contents\", navigate to Contents/MacOS/, the name of the executable will be there.  Right-click on it, select \"Get Info\".  The absolute path to the folder containing the executable is under the \"Where\" section.  ",
									"linecount" : 6,
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 390.0, 225.0, 370.0, 89.0 ],
									"id" : "obj-19",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "If a command is not in your PATH environment variable, you must specify an absolute path.  ",
									"linecount" : 2,
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 390.0, 180.0, 355.0, 34.0 ],
									"id" : "obj-17",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "Relative paths are relative to the folder containing the patch that contains [csound~].",
									"linecount" : 3,
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 150.0, 300.0, 204.0, 48.0 ],
									"id" : "obj-16",
									"fontname" : "Arial",
									"fontface" : 3,
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "You can also use relative paths:",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 150.0, 180.0, 204.0, 20.0 ],
									"id" : "obj-15",
									"fontname" : "Arial",
									"fontface" : 3,
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "edit hello.csd",
									"numinlets" : 2,
									"bgcolor" : [ 0.92549, 0.92549, 0.92549, 1.0 ],
									"numoutlets" : 1,
									"patching_rect" : [ 195.0, 210.0, 81.0, 18.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-13",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "edit sub/hello.csd",
									"numinlets" : 2,
									"bgcolor" : [ 0.92549, 0.92549, 0.92549, 1.0 ],
									"numoutlets" : 1,
									"patching_rect" : [ 195.0, 270.0, 105.0, 18.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-12",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "edit ../hello.csd",
									"numinlets" : 2,
									"bgcolor" : [ 0.92549, 0.92549, 0.92549, 1.0 ],
									"numoutlets" : 1,
									"patching_rect" : [ 195.0, 240.0, 91.0, 18.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-11",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "csound~ 1.0.6 or later required.",
									"numinlets" : 1,
									"frgb" : [ 0.65098, 0.0, 0.019608, 1.0 ],
									"textcolor" : [ 0.65098, 0.0, 0.019608, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 546.0, 99.0, 215.0, 20.0 ],
									"id" : "obj-9",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "The \"run\" message allows you to execute commands.",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 546.0, 120.0, 302.0, 20.0 ],
									"id" : "obj-10",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "run",
									"numinlets" : 1,
									"frgb" : [ 0.337255, 0.254902, 0.113725, 1.0 ],
									"textcolor" : [ 0.337255, 0.254902, 0.113725, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 546.0, 50.0, 68.0, 48.0 ],
									"id" : "obj-33",
									"fontname" : "Arial",
									"fontsize" : 36.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "panel",
									"numinlets" : 1,
									"bgcolor" : [ 0.952941, 0.933333, 0.917647, 1.0 ],
									"numoutlets" : 0,
									"border" : 1,
									"patching_rect" : [ 540.0, 45.0, 319.0, 128.0 ],
									"id" : "obj-106",
									"rounded" : 0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "pattrforward parent::csound~",
									"numinlets" : 1,
									"numoutlets" : 1,
									"patching_rect" : [ 15.0, 270.0, 165.0, 20.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-8",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "\"/Users/davis/Applications/Audacity 1.3.7.1/Audacity.app/Contents/MacOS/Audacity\" /Users/davis/Music/Csound/berg.wav",
									"linecount" : 2,
									"numinlets" : 2,
									"bgcolor" : [ 0.92549, 0.92549, 0.92549, 1.0 ],
									"numoutlets" : 1,
									"patching_rect" : [ 75.0, 135.0, 462.0, 32.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-7",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "pvanal -n256 -w4 mono.aif mono.pvx",
									"numinlets" : 2,
									"bgcolor" : [ 0.92549, 0.92549, 0.92549, 1.0 ],
									"numoutlets" : 1,
									"patching_rect" : [ 60.0, 105.0, 208.0, 18.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-6",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "/Applications/TextEdit.app/Contents/MacOS/TextEdit readbuf.csd",
									"numinlets" : 2,
									"bgcolor" : [ 0.92549, 0.92549, 0.92549, 1.0 ],
									"numoutlets" : 1,
									"patching_rect" : [ 30.0, 45.0, 357.0, 18.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-5",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "/Applications/Utilities/Terminal.app/Contents/MacOS/Terminal",
									"numinlets" : 2,
									"bgcolor" : [ 0.92549, 0.92549, 0.92549, 1.0 ],
									"numoutlets" : 1,
									"patching_rect" : [ 45.0, 75.0, 345.0, 18.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-4",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "\"C:/Program Files/Audacity 1.3 Beta (Unicode)/Audacity.exe\" \"H:/samples/01 Bipolar 175bpm Ebmin/kick01.wav\"",
									"numinlets" : 2,
									"bgcolor" : [ 0.92549, 0.92549, 0.92549, 1.0 ],
									"numoutlets" : 1,
									"patching_rect" : [ 15.0, 15.0, 651.0, 18.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-2",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "prepend run",
									"numinlets" : 1,
									"numoutlets" : 1,
									"patching_rect" : [ 15.0, 240.0, 75.0, 20.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-112",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
 ],
						"lines" : [ 							{
								"patchline" : 								{
									"source" : [ "obj-6", 0 ],
									"destination" : [ "obj-112", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-5", 0 ],
									"destination" : [ "obj-112", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-4", 0 ],
									"destination" : [ "obj-112", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-2", 0 ],
									"destination" : [ "obj-112", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-112", 0 ],
									"destination" : [ "obj-8", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-7", 0 ],
									"destination" : [ "obj-112", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
 ]
					}
,
					"saved_object_attributes" : 					{
						"default_fontname" : "Arial",
						"default_fontsize" : 12.0,
						"fontname" : "Arial",
						"globalpatchername" : "",
						"fontface" : 0,
						"fontsize" : 12.0,
						"default_fontface" : 0
					}

				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "open",
					"numinlets" : 2,
					"bgcolor" : [ 0.909804, 0.909804, 0.909804, 1.0 ],
					"numoutlets" : 1,
					"patching_rect" : [ 360.0, 330.0, 37.0, 18.0 ],
					"outlettype" : [ "" ],
					"id" : "obj-25",
					"fontname" : "Arial",
					"fontsize" : 12.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "More Info:",
					"numinlets" : 1,
					"frgb" : [ 0.337255, 0.254902, 0.113725, 1.0 ],
					"textcolor" : [ 0.337255, 0.254902, 0.113725, 1.0 ],
					"numoutlets" : 0,
					"patching_rect" : [ 789.0, 57.0, 91.0, 25.0 ],
					"id" : "obj-115",
					"fontname" : "Arial",
					"fontface" : 2,
					"fontsize" : 16.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"varname" : "loadsamp message",
					"text" : "p \"loadsamp message\"",
					"numinlets" : 0,
					"numoutlets" : 0,
					"patching_rect" : [ 790.0, 191.0, 140.0, 20.0 ],
					"id" : "obj-114",
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"patcher" : 					{
						"fileversion" : 1,
						"rect" : [ 430.0, 176.0, 791.0, 547.0 ],
						"bglocked" : 0,
						"defrect" : [ 430.0, 176.0, 791.0, 547.0 ],
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
									"maxclass" : "comment",
									"text" : "loadsamp loads an audio file into a Csound table.",
									"linecount" : 2,
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 15.0, 0.0, 198.0, 34.0 ],
									"id" : "obj-33",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "pattrforward path",
									"numinlets" : 1,
									"numoutlets" : 1,
									"patching_rect" : [ 626.0, 135.0, 102.0, 20.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-11",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "path",
									"numinlets" : 1,
									"hidden" : 1,
									"frgb" : [ 0.815686, 0.0, 0.0, 1.0 ],
									"textcolor" : [ 0.815686, 0.0, 0.0, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 0.0, 184.0, 43.0, 20.0 ],
									"id" : "obj-10",
									"fontname" : "Arial",
									"fontface" : 2,
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"varname" : "path",
									"text" : "t l",
									"numinlets" : 1,
									"hidden" : 1,
									"numoutlets" : 1,
									"patching_rect" : [ 34.0, 184.0, 19.0, 20.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-4",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "To avoid crashes when replacing tables, make sure no instruments are accessing that table.",
									"linecount" : 3,
									"numinlets" : 1,
									"frgb" : [ 0.666667, 0.0, 0.0, 1.0 ],
									"textcolor" : [ 0.666667, 0.0, 0.0, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 15.0, 107.0, 198.0, 48.0 ],
									"id" : "obj-36",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "1 - Start DSP\n2 - Start Csound\n3 - Get a path to an audio file\n4 - Read audio into Csound table\n5 - Play instr 1",
									"linecount" : 5,
									"numinlets" : 1,
									"frgb" : [ 0.117647, 0.203922, 0.682353, 1.0 ],
									"textcolor" : [ 0.117647, 0.203922, 0.682353, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 15.0, 32.0, 198.0, 75.0 ],
									"id" : "obj-34",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "Play instr 1",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 320.0, 341.0, 68.0, 20.0 ],
									"id" : "obj-12",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "2",
									"numinlets" : 1,
									"frgb" : [ 0.176471, 0.301961, 1.0, 1.0 ],
									"textcolor" : [ 0.176471, 0.301961, 1.0, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 360.0, 0.0, 38.0, 48.0 ],
									"id" : "obj-1",
									"fontname" : "Arial",
									"fontface" : 1,
									"fontsize" : 36.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "flonum",
									"numinlets" : 1,
									"numoutlets" : 2,
									"patching_rect" : [ 405.0, 391.0, 50.0, 20.0 ],
									"minimum" : 0.1,
									"outlettype" : [ "float", "bang" ],
									"id" : "obj-79",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "amp",
									"numinlets" : 1,
									"frgb" : [ 0.815686, 0.0, 0.0, 1.0 ],
									"textcolor" : [ 0.815686, 0.0, 0.0, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 392.0, 75.0, 43.0, 20.0 ],
									"id" : "obj-54",
									"fontname" : "Arial",
									"fontface" : 2,
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"varname" : "_",
									"text" : "pattr _ @bindto length",
									"numinlets" : 1,
									"numoutlets" : 3,
									"patching_rect" : [ 405.0, 361.0, 130.0, 20.0 ],
									"outlettype" : [ "", "", "" ],
									"id" : "obj-41",
									"fontname" : "Arial",
									"fontsize" : 12.0,
									"restore" : [ 10.078095 ],
									"saved_object_attributes" : 									{
										"parameter_enable" : 0
									}

								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "length",
									"numinlets" : 1,
									"frgb" : [ 0.815686, 0.0, 0.0, 1.0 ],
									"textcolor" : [ 0.815686, 0.0, 0.0, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 574.0, 135.0, 43.0, 20.0 ],
									"id" : "obj-16",
									"fontname" : "Arial",
									"fontface" : 2,
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "start",
									"numinlets" : 2,
									"bgcolor" : [ 0.835294, 1.0, 0.698039, 1.0 ],
									"numoutlets" : 1,
									"patching_rect" : [ 326.0, 15.0, 34.0, 18.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-27",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "ezdac~",
									"numinlets" : 2,
									"bgcolor" : [ 0.0, 0.0, 0.0, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 281.0, 105.0, 44.0, 44.0 ],
									"offgradcolor1" : [ 0.392157, 0.392157, 0.392157, 1.0 ],
									"ongradcolor1" : [ 1.0, 0.0, 0.0, 1.0 ],
									"id" : "obj-118",
									"ongradcolor2" : [ 1.0, 1.0, 0.133333, 1.0 ],
									"offgradcolor2" : [ 0.0, 0.0, 0.0, 1.0 ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "4",
									"numinlets" : 1,
									"frgb" : [ 0.176471, 0.301961, 1.0, 1.0 ],
									"textcolor" : [ 0.176471, 0.301961, 1.0, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 77.0, 164.0, 38.0, 48.0 ],
									"id" : "obj-25",
									"fontname" : "Arial",
									"fontface" : 1,
									"fontsize" : 36.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "Fill table 1.  Load all channels (0). Start reading at time 0. Read 44100 frames.",
									"linecount" : 2,
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 163.0, 295.0, 343.0, 34.0 ],
									"id" : "obj-21",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "Replace table 1 (\"-\" sign means replace). Load right channel (2). Start reading at 0.5 seconds. Read 16 seconds.",
									"linecount" : 2,
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 151.0, 252.0, 373.0, 34.0 ],
									"id" : "obj-20",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "Fill table 1.  Load left channel (1). Start reading at time 0. Read 3 seconds.",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 132.0, 213.0, 414.0, 20.0 ],
									"id" : "obj-18",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "5",
									"numinlets" : 1,
									"frgb" : [ 0.176471, 0.301961, 1.0, 1.0 ],
									"textcolor" : [ 0.176471, 0.301961, 1.0, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 301.0, 360.0, 38.0, 48.0 ],
									"id" : "obj-15",
									"fontname" : "Arial",
									"fontface" : 1,
									"fontsize" : 36.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "flonum",
									"varname" : "length",
									"numinlets" : 1,
									"numoutlets" : 2,
									"patching_rect" : [ 506.0, 135.0, 68.0, 23.0 ],
									"outlettype" : [ "float", "bang" ],
									"id" : "obj-14",
									"fontname" : "Arial",
									"fontsize" : 14.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "3",
									"numinlets" : 1,
									"frgb" : [ 0.176471, 0.301961, 1.0, 1.0 ],
									"textcolor" : [ 0.176471, 0.301961, 1.0, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 585.0, 0.0, 38.0, 48.0 ],
									"id" : "obj-7",
									"fontname" : "Arial",
									"fontface" : 1,
									"fontsize" : 36.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "p Get Length and Path",
									"numinlets" : 1,
									"numoutlets" : 2,
									"patching_rect" : [ 506.0, 105.0, 139.0, 20.0 ],
									"outlettype" : [ "float", "" ],
									"id" : "obj-8",
									"fontname" : "Arial",
									"fontsize" : 12.0,
									"patcher" : 									{
										"fileversion" : 1,
										"rect" : [ 656.0, 96.0, 226.0, 220.0 ],
										"bglocked" : 0,
										"defrect" : [ 656.0, 96.0, 226.0, 220.0 ],
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
										"boxes" : [ 											{
												"box" : 												{
													"maxclass" : "outlet",
													"numinlets" : 1,
													"numoutlets" : 0,
													"patching_rect" : [ 150.0, 150.0, 27.0, 27.0 ],
													"id" : "obj-6",
													"comment" : ""
												}

											}
, 											{
												"box" : 												{
													"maxclass" : "outlet",
													"numinlets" : 1,
													"numoutlets" : 0,
													"patching_rect" : [ 60.0, 150.0, 27.0, 27.0 ],
													"id" : "obj-1",
													"comment" : ""
												}

											}
, 											{
												"box" : 												{
													"maxclass" : "inlet",
													"numinlets" : 0,
													"numoutlets" : 1,
													"patching_rect" : [ 15.0, 15.0, 27.0, 27.0 ],
													"outlettype" : [ "" ],
													"id" : "obj-2",
													"comment" : ""
												}

											}
, 											{
												"box" : 												{
													"maxclass" : "newobj",
													"text" : "* 0.001",
													"numinlets" : 2,
													"numoutlets" : 1,
													"patching_rect" : [ 60.0, 120.0, 69.0, 20.0 ],
													"outlettype" : [ "float" ],
													"id" : "obj-3",
													"fontname" : "Arial",
													"fontsize" : 12.0
												}

											}
, 											{
												"box" : 												{
													"maxclass" : "newobj",
													"text" : "sfinfo~",
													"numinlets" : 1,
													"numoutlets" : 6,
													"patching_rect" : [ 15.0, 90.0, 94.0, 20.0 ],
													"outlettype" : [ "int", "int", "float", "float", "", "" ],
													"id" : "obj-4",
													"fontname" : "Arial",
													"fontsize" : 12.0
												}

											}
, 											{
												"box" : 												{
													"maxclass" : "newobj",
													"text" : "prepend open",
													"numinlets" : 1,
													"numoutlets" : 1,
													"patching_rect" : [ 15.0, 60.0, 91.0, 20.0 ],
													"outlettype" : [ "" ],
													"id" : "obj-5",
													"fontname" : "Arial",
													"fontsize" : 12.0
												}

											}
 ],
										"lines" : [ 											{
												"patchline" : 												{
													"source" : [ "obj-3", 0 ],
													"destination" : [ "obj-1", 0 ],
													"hidden" : 0,
													"midpoints" : [  ]
												}

											}
, 											{
												"patchline" : 												{
													"source" : [ "obj-4", 3 ],
													"destination" : [ "obj-3", 0 ],
													"hidden" : 0,
													"midpoints" : [  ]
												}

											}
, 											{
												"patchline" : 												{
													"source" : [ "obj-5", 0 ],
													"destination" : [ "obj-4", 0 ],
													"hidden" : 0,
													"midpoints" : [  ]
												}

											}
, 											{
												"patchline" : 												{
													"source" : [ "obj-2", 0 ],
													"destination" : [ "obj-5", 0 ],
													"hidden" : 0,
													"midpoints" : [  ]
												}

											}
, 											{
												"patchline" : 												{
													"source" : [ "obj-2", 0 ],
													"destination" : [ "obj-6", 0 ],
													"hidden" : 0,
													"midpoints" : [  ]
												}

											}
 ]
									}
,
									"saved_object_attributes" : 									{
										"default_fontname" : "Arial",
										"default_fontsize" : 12.0,
										"fontname" : "Arial",
										"globalpatchername" : "",
										"fontface" : 0,
										"fontsize" : 12.0,
										"default_fontface" : 0
									}

								}

							}
, 							{
								"box" : 								{
									"maxclass" : "button",
									"numinlets" : 1,
									"numoutlets" : 1,
									"patching_rect" : [ 708.0, 0.0, 29.0, 29.0 ],
									"outlettype" : [ "bang" ],
									"id" : "obj-26",
									"fgcolor" : [ 1.0, 0.0, 0.0, 1.0 ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "drop audio files here",
									"numinlets" : 1,
									"frgb" : [ 0.337255, 0.254902, 0.113725, 1.0 ],
									"textcolor" : [ 0.337255, 0.254902, 0.113725, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 516.0, 61.0, 184.0, 27.0 ],
									"id" : "obj-28",
									"fontname" : "Arial",
									"fontface" : 2,
									"fontsize" : 18.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "opendialog",
									"numinlets" : 1,
									"numoutlets" : 2,
									"patching_rect" : [ 708.0, 55.0, 71.0, 20.0 ],
									"outlettype" : [ "", "bang" ],
									"id" : "obj-9",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "dropfile",
									"numinlets" : 1,
									"numoutlets" : 2,
									"bordercolor" : [ 0.564706, 0.564706, 0.564706, 1.0 ],
									"patching_rect" : [ 506.0, 0.0, 197.0, 90.0 ],
									"outlettype" : [ "", "" ],
									"id" : "obj-48",
									"ignoreclick" : 1,
									"types" : [  ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "set_and_send",
									"numinlets" : 1,
									"frgb" : [ 0.815686, 0.0, 0.0, 1.0 ],
									"textcolor" : [ 0.815686, 0.0, 0.0, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 116.0, 465.0, 89.0, 20.0 ],
									"id" : "obj-5",
									"fontname" : "Arial",
									"fontface" : 2,
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"varname" : "set_and_send",
									"text" : "p set_and_send",
									"numinlets" : 1,
									"numoutlets" : 1,
									"patching_rect" : [ 15.0, 465.0, 100.0, 20.0 ],
									"outlettype" : [ "bang" ],
									"id" : "obj-2",
									"fontname" : "Arial",
									"fontsize" : 12.0,
									"patcher" : 									{
										"fileversion" : 1,
										"rect" : [ 95.0, 75.0, 277.0, 240.0 ],
										"bglocked" : 0,
										"defrect" : [ 95.0, 75.0, 277.0, 240.0 ],
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
										"boxes" : [ 											{
												"box" : 												{
													"maxclass" : "newobj",
													"text" : "t b l",
													"numinlets" : 1,
													"numoutlets" : 2,
													"patching_rect" : [ 90.0, 75.0, 32.5, 20.0 ],
													"outlettype" : [ "bang", "" ],
													"id" : "obj-6",
													"fontname" : "Arial",
													"fontsize" : 12.0
												}

											}
, 											{
												"box" : 												{
													"maxclass" : "newobj",
													"text" : "prepend set",
													"numinlets" : 1,
													"numoutlets" : 1,
													"patching_rect" : [ 135.0, 105.0, 76.0, 20.0 ],
													"outlettype" : [ "" ],
													"id" : "obj-3",
													"fontname" : "Arial",
													"fontsize" : 12.0
												}

											}
, 											{
												"box" : 												{
													"maxclass" : "outlet",
													"numinlets" : 1,
													"numoutlets" : 0,
													"patching_rect" : [ 90.0, 150.0, 25.0, 25.0 ],
													"id" : "obj-2",
													"comment" : ""
												}

											}
, 											{
												"box" : 												{
													"maxclass" : "inlet",
													"numinlets" : 0,
													"numoutlets" : 1,
													"patching_rect" : [ 90.0, 45.0, 25.0, 25.0 ],
													"outlettype" : [ "" ],
													"id" : "obj-1",
													"comment" : ""
												}

											}
 ],
										"lines" : [ 											{
												"patchline" : 												{
													"source" : [ "obj-6", 0 ],
													"destination" : [ "obj-2", 0 ],
													"hidden" : 0,
													"midpoints" : [  ]
												}

											}
, 											{
												"patchline" : 												{
													"source" : [ "obj-6", 1 ],
													"destination" : [ "obj-3", 0 ],
													"hidden" : 0,
													"midpoints" : [  ]
												}

											}
, 											{
												"patchline" : 												{
													"source" : [ "obj-1", 0 ],
													"destination" : [ "obj-6", 0 ],
													"hidden" : 0,
													"midpoints" : [  ]
												}

											}
, 											{
												"patchline" : 												{
													"source" : [ "obj-3", 0 ],
													"destination" : [ "obj-2", 0 ],
													"hidden" : 0,
													"midpoints" : [  ]
												}

											}
 ]
									}
,
									"saved_object_attributes" : 									{
										"default_fontname" : "Arial",
										"default_fontsize" : 12.0,
										"fontname" : "Arial",
										"globalpatchername" : "",
										"fontface" : 0,
										"fontsize" : 12.0,
										"default_fontface" : 0
									}

								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "Last loadsamp message sent to csound~:",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 240.0, 475.0, 237.0, 20.0 ],
									"id" : "obj-71",
									"fontname" : "Arial",
									"fontface" : 2,
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "button",
									"numinlets" : 1,
									"numoutlets" : 1,
									"patching_rect" : [ 15.0, 405.0, 20.0, 20.0 ],
									"outlettype" : [ "bang" ],
									"id" : "obj-70"
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "event length (sec)",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 455.0, 391.0, 107.0, 20.0 ],
									"id" : "obj-57",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "number",
									"varname" : "amp",
									"numinlets" : 1,
									"numoutlets" : 2,
									"patching_rect" : [ 341.0, 75.0, 50.0, 20.0 ],
									"outlettype" : [ "int", "bang" ],
									"id" : "obj-55",
									"fontname" : "Arial",
									"maximum" : 12,
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "pack_it",
									"numinlets" : 1,
									"frgb" : [ 0.815686, 0.0, 0.0, 1.0 ],
									"textcolor" : [ 0.815686, 0.0, 0.0, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 7.0, 372.0, 52.0, 20.0 ],
									"id" : "obj-50",
									"fontname" : "Arial",
									"fontface" : 2,
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "p amp",
									"numinlets" : 3,
									"numoutlets" : 2,
									"patching_rect" : [ 281.0, 75.0, 56.0, 20.0 ],
									"outlettype" : [ "signal", "signal" ],
									"id" : "obj-49",
									"fontname" : "Arial",
									"fontsize" : 12.0,
									"patcher" : 									{
										"fileversion" : 1,
										"rect" : [ 20.0, 74.0, 350.0, 322.0 ],
										"bglocked" : 0,
										"defrect" : [ 20.0, 74.0, 350.0, 322.0 ],
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
										"boxes" : [ 											{
												"box" : 												{
													"maxclass" : "outlet",
													"numinlets" : 1,
													"numoutlets" : 0,
													"patching_rect" : [ 90.0, 148.0, 21.0, 21.0 ],
													"id" : "obj-1",
													"comment" : ""
												}

											}
, 											{
												"box" : 												{
													"maxclass" : "outlet",
													"numinlets" : 1,
													"numoutlets" : 0,
													"patching_rect" : [ 33.0, 148.0, 21.0, 21.0 ],
													"id" : "obj-2",
													"comment" : ""
												}

											}
, 											{
												"box" : 												{
													"maxclass" : "newobj",
													"text" : "dbtoa",
													"numinlets" : 1,
													"numoutlets" : 1,
													"patching_rect" : [ 129.0, 70.0, 48.0, 20.0 ],
													"outlettype" : [ "" ],
													"id" : "obj-3",
													"fontname" : "Arial",
													"fontsize" : 12.0
												}

											}
, 											{
												"box" : 												{
													"maxclass" : "newobj",
													"text" : "*~ 1.",
													"numinlets" : 2,
													"numoutlets" : 1,
													"patching_rect" : [ 90.0, 105.0, 49.0, 20.0 ],
													"outlettype" : [ "signal" ],
													"id" : "obj-4",
													"fontname" : "Arial",
													"fontsize" : 12.0
												}

											}
, 											{
												"box" : 												{
													"maxclass" : "newobj",
													"text" : "*~ 1.",
													"numinlets" : 2,
													"numoutlets" : 1,
													"patching_rect" : [ 33.0, 105.0, 49.0, 20.0 ],
													"outlettype" : [ "signal" ],
													"id" : "obj-5",
													"fontname" : "Arial",
													"fontsize" : 12.0
												}

											}
, 											{
												"box" : 												{
													"maxclass" : "inlet",
													"numinlets" : 0,
													"numoutlets" : 1,
													"patching_rect" : [ 129.0, 41.0, 21.0, 21.0 ],
													"outlettype" : [ "int" ],
													"id" : "obj-6",
													"comment" : ""
												}

											}
, 											{
												"box" : 												{
													"maxclass" : "inlet",
													"numinlets" : 0,
													"numoutlets" : 1,
													"patching_rect" : [ 90.0, 41.0, 21.0, 21.0 ],
													"outlettype" : [ "signal" ],
													"id" : "obj-7",
													"comment" : ""
												}

											}
, 											{
												"box" : 												{
													"maxclass" : "inlet",
													"numinlets" : 0,
													"numoutlets" : 1,
													"patching_rect" : [ 33.0, 41.0, 21.0, 21.0 ],
													"outlettype" : [ "signal" ],
													"id" : "obj-8",
													"comment" : ""
												}

											}
 ],
										"lines" : [ 											{
												"patchline" : 												{
													"source" : [ "obj-8", 0 ],
													"destination" : [ "obj-5", 0 ],
													"hidden" : 0,
													"midpoints" : [  ]
												}

											}
, 											{
												"patchline" : 												{
													"source" : [ "obj-5", 0 ],
													"destination" : [ "obj-2", 0 ],
													"hidden" : 0,
													"midpoints" : [  ]
												}

											}
, 											{
												"patchline" : 												{
													"source" : [ "obj-3", 0 ],
													"destination" : [ "obj-5", 1 ],
													"hidden" : 0,
													"midpoints" : [  ]
												}

											}
, 											{
												"patchline" : 												{
													"source" : [ "obj-7", 0 ],
													"destination" : [ "obj-4", 0 ],
													"hidden" : 0,
													"midpoints" : [  ]
												}

											}
, 											{
												"patchline" : 												{
													"source" : [ "obj-4", 0 ],
													"destination" : [ "obj-1", 0 ],
													"hidden" : 0,
													"midpoints" : [  ]
												}

											}
, 											{
												"patchline" : 												{
													"source" : [ "obj-6", 0 ],
													"destination" : [ "obj-3", 0 ],
													"hidden" : 0,
													"midpoints" : [  ]
												}

											}
, 											{
												"patchline" : 												{
													"source" : [ "obj-3", 0 ],
													"destination" : [ "obj-4", 1 ],
													"hidden" : 0,
													"midpoints" : [  ]
												}

											}
 ]
									}
,
									"saved_object_attributes" : 									{
										"default_fontname" : "Arial",
										"default_fontsize" : 12.0,
										"fontname" : "Arial",
										"globalpatchername" : "",
										"fontface" : 0,
										"fontsize" : 12.0,
										"default_fontface" : 0
									}

								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "loadsamp -1 2 H:/samples/Recordings/deil/Jelia_Montage.aif 0.5 16.",
									"numinlets" : 2,
									"bgcolor" : [ 0.921569, 1.0, 0.482353, 1.0 ],
									"numoutlets" : 1,
									"patching_rect" : [ 15.0, 495.0, 738.0, 21.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-40",
									"fontname" : "Arial",
									"fontface" : 1,
									"fontsize" : 14.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "pattrforward csound~",
									"numinlets" : 1,
									"numoutlets" : 1,
									"patching_rect" : [ 15.0, 525.0, 126.0, 20.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-29",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"varname" : "pack_it",
									"text" : "unpack 0 0 s 0. 0.",
									"numinlets" : 1,
									"numoutlets" : 5,
									"patching_rect" : [ 57.0, 373.0, 187.0, 20.0 ],
									"outlettype" : [ "int", "int", "", "float", "float" ],
									"id" : "obj-13",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "pack e i1 0 1.",
									"numinlets" : 4,
									"numoutlets" : 1,
									"patching_rect" : [ 330.0, 421.0, 94.0, 20.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-22",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "pattrforward csound~",
									"numinlets" : 1,
									"numoutlets" : 1,
									"patching_rect" : [ 330.0, 445.0, 126.0, 20.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-24",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "button",
									"numinlets" : 1,
									"numoutlets" : 1,
									"patching_rect" : [ 330.0, 361.0, 45.0, 45.0 ],
									"outlettype" : [ "bang" ],
									"id" : "obj-30",
									"fgcolor" : [ 0.74902, 1.0, 0.0, 1.0 ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "pattrforward pack_it",
									"numinlets" : 1,
									"hidden" : 1,
									"numoutlets" : 1,
									"patching_rect" : [ 30.0, 334.0, 117.0, 20.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-68",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "1 0 $1 0. -44100",
									"numinlets" : 2,
									"bgcolor" : [ 0.941176, 0.941176, 0.941176, 1.0 ],
									"numoutlets" : 1,
									"patching_rect" : [ 60.0, 304.0, 100.0, 18.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-63",
									"fontname" : "Arial",
									"fontface" : 1,
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "1 1 $1 0. 3.",
									"numinlets" : 2,
									"bgcolor" : [ 0.941176, 0.941176, 0.941176, 1.0 ],
									"numoutlets" : 1,
									"patching_rect" : [ 60.0, 214.0, 70.0, 18.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-61",
									"fontname" : "Arial",
									"fontface" : 1,
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "-1 2 $1 0.5 16.",
									"numinlets" : 2,
									"bgcolor" : [ 0.941176, 0.941176, 0.941176, 1.0 ],
									"numoutlets" : 1,
									"patching_rect" : [ 60.0, 259.0, 89.0, 18.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-59",
									"fontname" : "Arial",
									"fontface" : 1,
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "1",
									"numinlets" : 1,
									"frgb" : [ 0.176471, 0.301961, 1.0, 1.0 ],
									"textcolor" : [ 0.176471, 0.301961, 1.0, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 252.0, 102.0, 38.0, 48.0 ],
									"id" : "obj-65",
									"fontname" : "Arial",
									"fontface" : 1,
									"fontsize" : 36.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "scope~",
									"numinlets" : 2,
									"bgcolor" : [ 0.0, 0.0, 0.0, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 326.0, 105.0, 124.0, 60.0 ],
									"id" : "obj-53",
									"bufsize" : 64,
									"calccount" : 128
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "number",
									"numinlets" : 1,
									"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
									"numoutlets" : 2,
									"patching_rect" : [ 57.0, 405.0, 38.0, 20.0 ],
									"triscale" : 0.9,
									"outlettype" : [ "int", "bang" ],
									"id" : "obj-3",
									"fontname" : "Arial",
									"fontsize" : 12.0,
									"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "flonum",
									"numinlets" : 1,
									"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
									"numoutlets" : 2,
									"patching_rect" : [ 225.0, 405.0, 40.0, 20.0 ],
									"triscale" : 0.9,
									"outlettype" : [ "float", "bang" ],
									"id" : "obj-17",
									"fontname" : "Arial",
									"fontsize" : 12.0,
									"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "flonum",
									"numinlets" : 1,
									"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
									"numoutlets" : 2,
									"patching_rect" : [ 183.0, 405.0, 40.0, 20.0 ],
									"triscale" : 0.9,
									"outlettype" : [ "float", "bang" ],
									"id" : "obj-19",
									"fontname" : "Arial",
									"fontsize" : 12.0,
									"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "number",
									"numinlets" : 1,
									"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
									"numoutlets" : 2,
									"patching_rect" : [ 99.0, 405.0, 37.0, 20.0 ],
									"triscale" : 0.9,
									"minimum" : 0,
									"outlettype" : [ "int", "bang" ],
									"id" : "obj-23",
									"fontname" : "Arial",
									"fontsize" : 12.0,
									"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "pack loadsamp 1 1 mybuf 0. 0.",
									"numinlets" : 6,
									"numoutlets" : 1,
									"patching_rect" : [ 15.0, 435.0, 229.0, 20.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-32",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "stop",
									"numinlets" : 2,
									"bgcolor" : [ 0.941176, 0.941176, 0.941176, 1.0 ],
									"numoutlets" : 1,
									"patching_rect" : [ 281.0, 15.0, 35.0, 18.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-43",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"varname" : "csound~",
									"text" : "csound~ @args readbuf.csd",
									"numinlets" : 2,
									"numoutlets" : 6,
									"patching_rect" : [ 281.0, 45.0, 166.0, 20.0 ],
									"outlettype" : [ "signal", "signal", "list", "int", "bang", "bang" ],
									"id" : "obj-45",
									"fontname" : "Arial",
									"fontsize" : 12.0,
									"saved_object_attributes" : 									{
										"matchsr" : 1,
										"bypass" : 0,
										"overdrive" : 0,
										"output" : 1,
										"input" : 1,
										"interval" : 10,
										"message" : 1,
										"autostart" : 0,
										"args" : "readbuf.csd"
									}

								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "csound~",
									"numinlets" : 1,
									"frgb" : [ 0.815686, 0.0, 0.0, 1.0 ],
									"textcolor" : [ 0.815686, 0.0, 0.0, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 224.0, 45.0, 58.0, 20.0 ],
									"id" : "obj-46",
									"fontname" : "Arial",
									"fontface" : 2,
									"fontsize" : 12.0
								}

							}
 ],
						"lines" : [ 							{
								"patchline" : 								{
									"source" : [ "obj-4", 0 ],
									"destination" : [ "obj-63", 0 ],
									"hidden" : 1,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-4", 0 ],
									"destination" : [ "obj-59", 0 ],
									"hidden" : 1,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-4", 0 ],
									"destination" : [ "obj-61", 0 ],
									"hidden" : 1,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-8", 1 ],
									"destination" : [ "obj-11", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-45", 0 ],
									"destination" : [ "obj-53", 0 ],
									"hidden" : 1,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-45", 0 ],
									"destination" : [ "obj-49", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-43", 0 ],
									"destination" : [ "obj-45", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-45", 1 ],
									"destination" : [ "obj-49", 1 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-27", 0 ],
									"destination" : [ "obj-45", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-41", 0 ],
									"destination" : [ "obj-79", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-79", 0 ],
									"destination" : [ "obj-22", 3 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-49", 0 ],
									"destination" : [ "obj-118", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-49", 1 ],
									"destination" : [ "obj-118", 1 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-8", 0 ],
									"destination" : [ "obj-14", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-26", 0 ],
									"destination" : [ "obj-9", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-9", 0 ],
									"destination" : [ "obj-8", 0 ],
									"hidden" : 0,
									"midpoints" : [ 717.5, 97.0, 515.5, 97.0 ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-48", 0 ],
									"destination" : [ "obj-8", 0 ],
									"hidden" : 0,
									"midpoints" : [ 515.5, 100.0, 515.5, 100.0 ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-30", 0 ],
									"destination" : [ "obj-22", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-13", 0 ],
									"destination" : [ "obj-3", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-13", 1 ],
									"destination" : [ "obj-23", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-13", 3 ],
									"destination" : [ "obj-19", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-13", 4 ],
									"destination" : [ "obj-17", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-59", 0 ],
									"destination" : [ "obj-68", 0 ],
									"hidden" : 1,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-63", 0 ],
									"destination" : [ "obj-68", 0 ],
									"hidden" : 1,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-22", 0 ],
									"destination" : [ "obj-24", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-55", 0 ],
									"destination" : [ "obj-49", 2 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-61", 0 ],
									"destination" : [ "obj-68", 0 ],
									"hidden" : 1,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-13", 0 ],
									"destination" : [ "obj-70", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-2", 0 ],
									"destination" : [ "obj-40", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-40", 0 ],
									"destination" : [ "obj-29", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-70", 0 ],
									"destination" : [ "obj-32", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-13", 2 ],
									"destination" : [ "obj-32", 3 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-17", 0 ],
									"destination" : [ "obj-32", 5 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-19", 0 ],
									"destination" : [ "obj-32", 4 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-23", 0 ],
									"destination" : [ "obj-32", 2 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-3", 0 ],
									"destination" : [ "obj-32", 1 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-32", 0 ],
									"destination" : [ "obj-2", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
 ]
					}
,
					"saved_object_attributes" : 					{
						"default_fontname" : "Arial",
						"default_fontsize" : 12.0,
						"fontname" : "Arial",
						"globalpatchername" : "",
						"fontface" : 0,
						"fontsize" : 12.0,
						"default_fontface" : 0
					}

				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "pattrforward csound~",
					"numinlets" : 1,
					"hidden" : 1,
					"numoutlets" : 1,
					"patching_rect" : [ 0.0, 211.0, 127.0, 20.0 ],
					"outlettype" : [ "" ],
					"id" : "obj-113",
					"fontname" : "Arial",
					"fontsize" : 12.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "pattrforward csound~",
					"numinlets" : 1,
					"hidden" : 1,
					"numoutlets" : 1,
					"patching_rect" : [ 15.0, 531.0, 126.0, 20.0 ],
					"outlettype" : [ "" ],
					"id" : "obj-108",
					"fontname" : "Arial",
					"fontsize" : 12.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "p \"record play\"",
					"numinlets" : 0,
					"numoutlets" : 0,
					"patching_rect" : [ 790.0, 256.0, 88.0, 20.0 ],
					"id" : "obj-105",
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"patcher" : 					{
						"fileversion" : 1,
						"rect" : [ 718.0, 382.0, 479.0, 263.0 ],
						"bglocked" : 0,
						"defrect" : [ 718.0, 382.0, 479.0, 263.0 ],
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
									"text" : "pattrforward parent::csound~",
									"numinlets" : 1,
									"numoutlets" : 1,
									"patching_rect" : [ 30.0, 225.0, 165.0, 20.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-112",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "Playback speed (1 == no change in speed).",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 101.0, 75.0, 247.0, 20.0 ],
									"id" : "obj-3",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "loadmess 1.",
									"numinlets" : 1,
									"numoutlets" : 1,
									"patching_rect" : [ 30.0, 24.0, 77.0, 20.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-2",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "tempo $1",
									"numinlets" : 2,
									"numoutlets" : 1,
									"patching_rect" : [ 30.0, 75.0, 67.0, 18.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-6",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "flonum",
									"numinlets" : 1,
									"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
									"numoutlets" : 2,
									"patching_rect" : [ 30.0, 50.0, 40.0, 20.0 ],
									"triscale" : 0.9,
									"minimum" : 0.01,
									"outlettype" : [ "float", "bang" ],
									"id" : "obj-7",
									"fontname" : "Arial",
									"maximum" : 8.0,
									"fontsize" : 12.0,
									"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "Read recording from file.",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 109.0, 195.0, 145.0, 20.0 ],
									"id" : "obj-24",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "Write recording to file.",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 115.0, 165.0, 132.0, 20.0 ],
									"id" : "obj-25",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "read rec.xml",
									"numinlets" : 2,
									"numoutlets" : 1,
									"patching_rect" : [ 30.0, 195.0, 77.0, 18.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-26",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "write rec.xml",
									"numinlets" : 2,
									"numoutlets" : 1,
									"patching_rect" : [ 30.0, 165.0, 78.0, 18.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-27",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "Start/Stop playback of recorded events and MIDI data.",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 157.0, 135.0, 302.0, 20.0 ],
									"id" : "obj-28",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "Start/Stop recording events and MIDI data.",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 195.0, 105.0, 238.0, 20.0 ],
									"id" : "obj-29",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "playstop",
									"numinlets" : 2,
									"numoutlets" : 1,
									"patching_rect" : [ 90.0, 135.0, 64.0, 18.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-30",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "playstart",
									"numinlets" : 2,
									"numoutlets" : 1,
									"patching_rect" : [ 30.0, 135.0, 59.0, 18.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-31",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "recordstop",
									"numinlets" : 2,
									"numoutlets" : 1,
									"patching_rect" : [ 114.0, 105.0, 78.0, 18.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-32",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "recordstart",
									"numinlets" : 2,
									"numoutlets" : 1,
									"patching_rect" : [ 30.0, 105.0, 83.0, 18.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-33",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
 ],
						"lines" : [ 							{
								"patchline" : 								{
									"source" : [ "obj-2", 0 ],
									"destination" : [ "obj-7", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-7", 0 ],
									"destination" : [ "obj-6", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-26", 0 ],
									"destination" : [ "obj-112", 0 ],
									"hidden" : 1,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-27", 0 ],
									"destination" : [ "obj-112", 0 ],
									"hidden" : 1,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-30", 0 ],
									"destination" : [ "obj-112", 0 ],
									"hidden" : 1,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-31", 0 ],
									"destination" : [ "obj-112", 0 ],
									"hidden" : 1,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-33", 0 ],
									"destination" : [ "obj-112", 0 ],
									"hidden" : 1,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-32", 0 ],
									"destination" : [ "obj-112", 0 ],
									"hidden" : 1,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-6", 0 ],
									"destination" : [ "obj-112", 0 ],
									"hidden" : 1,
									"midpoints" : [  ]
								}

							}
 ]
					}
,
					"saved_object_attributes" : 					{
						"default_fontname" : "Arial",
						"default_fontsize" : 12.0,
						"fontname" : "Arial",
						"globalpatchername" : "",
						"fontface" : 0,
						"fontsize" : 12.0,
						"default_fontface" : 0
					}

				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "p \"audio input/output\"",
					"numinlets" : 0,
					"numoutlets" : 0,
					"patching_rect" : [ 790.0, 83.0, 124.0, 20.0 ],
					"id" : "obj-10",
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"patcher" : 					{
						"fileversion" : 1,
						"rect" : [ 317.0, 172.0, 925.0, 429.0 ],
						"bglocked" : 0,
						"defrect" : [ 317.0, 172.0, 925.0, 429.0 ],
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
									"maxclass" : "comment",
									"text" : "If you're having a hard time finding a valid value for kr after setting ksmps, don't bother. Just leave the kr statement out of the orchestra.",
									"linecount" : 3,
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 590.0, 174.0, 306.0, 48.0 ],
									"id" : "obj-25",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "Max signal vector size = 32 ksmps = 32, 16, 8, 4, 2, 1",
									"linecount" : 2,
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 657.0, 81.0, 162.0, 34.0 ],
									"id" : "obj-39",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "If ksmps does NOT evenly divide the Max signal vector size, [csound~] will incur ksmps samples of delay when processing audio.",
									"linecount" : 3,
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 589.0, 120.0, 306.0, 48.0 ],
									"id" : "obj-40",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "If ksmps evenly divides the Max signal vector size, [csound~] will run with zero latency when processing audio. For example, the following vector size and ksmps values would incur zero latency:",
									"linecount" : 4,
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 589.0, 14.0, 310.0, 62.0 ],
									"id" : "obj-42",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "panel",
									"numinlets" : 1,
									"bgcolor" : [ 0.964706, 0.952941, 0.917647, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 584.0, 8.0, 319.0, 224.0 ],
									"id" : "obj-43",
									"rounded" : 0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "loadmess 1",
									"numinlets" : 1,
									"hidden" : 1,
									"numoutlets" : 1,
									"patching_rect" : [ 552.0, 366.0, 74.0, 20.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-21",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "When using the \"noscale\" flag, remember to add the line \"0dbfs = 1\" in your orchestra header.",
									"linecount" : 2,
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 307.0, 165.0, 276.0, 34.0 ],
									"id" : "obj-38",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "3",
									"numinlets" : 1,
									"frgb" : [ 0.176471, 0.301961, 1.0, 1.0 ],
									"textcolor" : [ 0.176471, 0.301961, 1.0, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 59.0, 28.0, 38.0, 48.0 ],
									"id" : "obj-37",
									"fontname" : "Arial",
									"fontface" : 1,
									"fontsize" : 36.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "2",
									"numinlets" : 1,
									"frgb" : [ 0.176471, 0.301961, 1.0, 1.0 ],
									"textcolor" : [ 0.176471, 0.301961, 1.0, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 409.0, 276.0, 38.0, 48.0 ],
									"id" : "obj-36",
									"fontname" : "Arial",
									"fontface" : 1,
									"fontsize" : 36.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "1",
									"numinlets" : 1,
									"frgb" : [ 0.176471, 0.301961, 1.0, 1.0 ],
									"textcolor" : [ 0.176471, 0.301961, 1.0, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 56.0, 266.0, 38.0, 48.0 ],
									"id" : "obj-54",
									"fontname" : "Arial",
									"fontface" : 1,
									"fontsize" : 36.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "3) Play with the pitches.",
									"numinlets" : 1,
									"frgb" : [ 0.0, 0.14902, 0.964706, 1.0 ],
									"textcolor" : [ 0.0, 0.14902, 0.964706, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 465.0, 300.0, 163.0, 23.0 ],
									"id" : "obj-35",
									"fontname" : "Arial",
									"fontsize" : 14.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "button",
									"numinlets" : 1,
									"numoutlets" : 1,
									"patching_rect" : [ 333.0, 262.0, 74.0, 74.0 ],
									"outlettype" : [ "bang" ],
									"id" : "obj-41",
									"fgcolor" : [ 0.0, 1.0, 0.301961, 1.0 ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "2) Start [csound~].",
									"numinlets" : 1,
									"frgb" : [ 0.0, 0.14902, 0.964706, 1.0 ],
									"textcolor" : [ 0.0, 0.14902, 0.964706, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 465.0, 275.0, 142.0, 23.0 ],
									"id" : "obj-33",
									"fontname" : "Arial",
									"fontsize" : 14.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "1) Start DSP. ",
									"numinlets" : 1,
									"frgb" : [ 0.0, 0.14902, 0.964706, 1.0 ],
									"textcolor" : [ 0.0, 0.14902, 0.964706, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 465.0, 250.0, 118.0, 23.0 ],
									"id" : "obj-34",
									"fontname" : "Arial",
									"fontsize" : 14.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "scope~",
									"numinlets" : 2,
									"numoutlets" : 0,
									"patching_rect" : [ 149.0, 258.0, 130.0, 130.0 ],
									"id" : "obj-2"
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "dB amplitude",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 166.0, 201.0, 86.0, 20.0 ],
									"id" : "obj-1",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "audio i/o",
									"numinlets" : 1,
									"frgb" : [ 0.337255, 0.254902, 0.113725, 1.0 ],
									"textcolor" : [ 0.337255, 0.254902, 0.113725, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 307.0, 15.0, 167.0, 48.0 ],
									"id" : "obj-3",
									"fontname" : "Arial",
									"fontsize" : 36.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "pitch 2",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 179.0, 23.0, 65.0, 20.0 ],
									"id" : "obj-4",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "pitch 1",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 88.0, 23.0, 56.0, 20.0 ],
									"id" : "obj-5",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "In this patch, we're sending two sine waves into [csound~]. The orchestra will mix the two signals so that they are output in stereo.",
									"linecount" : 3,
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 307.0, 112.0, 270.0, 48.0 ],
									"id" : "obj-6",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "All [csound~] inlets accept audio signals. To access these signals in your csd/orc file, use the \"inch\" opcode.",
									"linecount" : 3,
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 307.0, 62.0, 269.0, 48.0 ],
									"id" : "obj-7",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "mtof",
									"numinlets" : 1,
									"numoutlets" : 1,
									"patching_rect" : [ 88.0, 65.0, 39.0, 20.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-8",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "flonum",
									"numinlets" : 1,
									"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
									"numoutlets" : 2,
									"patching_rect" : [ 88.0, 43.0, 40.0, 20.0 ],
									"triscale" : 0.9,
									"minimum" : 0.0,
									"outlettype" : [ "float", "bang" ],
									"id" : "obj-9",
									"fontname" : "Arial",
									"maximum" : 127.0,
									"fontsize" : 12.0,
									"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "ch 2",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 117.0, 175.0, 40.0, 20.0 ],
									"id" : "obj-10",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "ch 1",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 66.0, 175.0, 40.0, 20.0 ],
									"id" : "obj-11",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "ch 2",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 191.0, 120.0, 40.0, 20.0 ],
									"id" : "obj-12",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "cycle~",
									"numinlets" : 2,
									"numoutlets" : 1,
									"patching_rect" : [ 88.0, 87.0, 51.0, 20.0 ],
									"outlettype" : [ "signal" ],
									"id" : "obj-13",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "mtof",
									"numinlets" : 1,
									"numoutlets" : 1,
									"patching_rect" : [ 181.0, 65.0, 39.0, 20.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-14",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "flonum",
									"numinlets" : 1,
									"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
									"numoutlets" : 2,
									"patching_rect" : [ 181.0, 43.0, 40.0, 20.0 ],
									"triscale" : 0.9,
									"minimum" : 0.0,
									"outlettype" : [ "float", "bang" ],
									"id" : "obj-15",
									"fontname" : "Arial",
									"maximum" : 127.0,
									"fontsize" : 12.0,
									"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "cycle~",
									"numinlets" : 2,
									"numoutlets" : 1,
									"patching_rect" : [ 181.0, 87.0, 51.0, 20.0 ],
									"outlettype" : [ "signal" ],
									"id" : "obj-16",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "flonum",
									"numinlets" : 1,
									"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
									"numoutlets" : 2,
									"patching_rect" : [ 126.0, 202.0, 40.0, 20.0 ],
									"triscale" : 0.9,
									"outlettype" : [ "float", "bang" ],
									"id" : "obj-17",
									"fontname" : "Arial",
									"maximum" : 0.0,
									"fontsize" : 12.0,
									"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "p amp",
									"numinlets" : 3,
									"numoutlets" : 2,
									"patching_rect" : [ 88.0, 226.0, 57.0, 20.0 ],
									"outlettype" : [ "signal", "signal" ],
									"id" : "obj-18",
									"fontname" : "Arial",
									"fontsize" : 12.0,
									"patcher" : 									{
										"fileversion" : 1,
										"rect" : [ 20.0, 74.0, 350.0, 322.0 ],
										"bglocked" : 0,
										"defrect" : [ 20.0, 74.0, 350.0, 322.0 ],
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
										"boxes" : [ 											{
												"box" : 												{
													"maxclass" : "outlet",
													"numinlets" : 1,
													"numoutlets" : 0,
													"patching_rect" : [ 90.0, 148.0, 21.0, 21.0 ],
													"id" : "obj-1",
													"comment" : ""
												}

											}
, 											{
												"box" : 												{
													"maxclass" : "outlet",
													"numinlets" : 1,
													"numoutlets" : 0,
													"patching_rect" : [ 33.0, 148.0, 21.0, 21.0 ],
													"id" : "obj-2",
													"comment" : ""
												}

											}
, 											{
												"box" : 												{
													"maxclass" : "newobj",
													"text" : "dbtoa",
													"numinlets" : 1,
													"numoutlets" : 1,
													"patching_rect" : [ 129.0, 70.0, 48.0, 20.0 ],
													"outlettype" : [ "" ],
													"id" : "obj-3",
													"fontname" : "Arial",
													"fontsize" : 12.0
												}

											}
, 											{
												"box" : 												{
													"maxclass" : "newobj",
													"text" : "*~ 1.",
													"numinlets" : 2,
													"numoutlets" : 1,
													"patching_rect" : [ 90.0, 105.0, 49.0, 20.0 ],
													"outlettype" : [ "signal" ],
													"id" : "obj-4",
													"fontname" : "Arial",
													"fontsize" : 12.0
												}

											}
, 											{
												"box" : 												{
													"maxclass" : "newobj",
													"text" : "*~ 1.",
													"numinlets" : 2,
													"numoutlets" : 1,
													"patching_rect" : [ 33.0, 105.0, 49.0, 20.0 ],
													"outlettype" : [ "signal" ],
													"id" : "obj-5",
													"fontname" : "Arial",
													"fontsize" : 12.0
												}

											}
, 											{
												"box" : 												{
													"maxclass" : "inlet",
													"numinlets" : 0,
													"numoutlets" : 1,
													"patching_rect" : [ 129.0, 41.0, 21.0, 21.0 ],
													"outlettype" : [ "float" ],
													"id" : "obj-6",
													"comment" : ""
												}

											}
, 											{
												"box" : 												{
													"maxclass" : "inlet",
													"numinlets" : 0,
													"numoutlets" : 1,
													"patching_rect" : [ 90.0, 41.0, 21.0, 21.0 ],
													"outlettype" : [ "signal" ],
													"id" : "obj-7",
													"comment" : ""
												}

											}
, 											{
												"box" : 												{
													"maxclass" : "inlet",
													"numinlets" : 0,
													"numoutlets" : 1,
													"patching_rect" : [ 33.0, 41.0, 21.0, 21.0 ],
													"outlettype" : [ "signal" ],
													"id" : "obj-8",
													"comment" : ""
												}

											}
 ],
										"lines" : [ 											{
												"patchline" : 												{
													"source" : [ "obj-3", 0 ],
													"destination" : [ "obj-4", 1 ],
													"hidden" : 0,
													"midpoints" : [  ]
												}

											}
, 											{
												"patchline" : 												{
													"source" : [ "obj-6", 0 ],
													"destination" : [ "obj-3", 0 ],
													"hidden" : 0,
													"midpoints" : [  ]
												}

											}
, 											{
												"patchline" : 												{
													"source" : [ "obj-4", 0 ],
													"destination" : [ "obj-1", 0 ],
													"hidden" : 0,
													"midpoints" : [  ]
												}

											}
, 											{
												"patchline" : 												{
													"source" : [ "obj-7", 0 ],
													"destination" : [ "obj-4", 0 ],
													"hidden" : 0,
													"midpoints" : [  ]
												}

											}
, 											{
												"patchline" : 												{
													"source" : [ "obj-3", 0 ],
													"destination" : [ "obj-5", 1 ],
													"hidden" : 0,
													"midpoints" : [  ]
												}

											}
, 											{
												"patchline" : 												{
													"source" : [ "obj-5", 0 ],
													"destination" : [ "obj-2", 0 ],
													"hidden" : 0,
													"midpoints" : [  ]
												}

											}
, 											{
												"patchline" : 												{
													"source" : [ "obj-8", 0 ],
													"destination" : [ "obj-5", 0 ],
													"hidden" : 0,
													"midpoints" : [  ]
												}

											}
 ]
									}
,
									"saved_object_attributes" : 									{
										"default_fontname" : "Arial",
										"default_fontsize" : 12.0,
										"fontname" : "Arial",
										"globalpatchername" : "",
										"fontface" : 0,
										"fontsize" : 12.0,
										"default_fontface" : 0
									}

								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"varname" : "csIN[1]",
									"text" : "pattrforward csIN",
									"numinlets" : 1,
									"numoutlets" : 1,
									"patching_rect" : [ 333.0, 397.0, 102.0, 20.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-19",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "message 1",
									"numinlets" : 2,
									"numoutlets" : 1,
									"patching_rect" : [ 468.0, 368.0, 73.0, 18.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-20",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "preset",
									"numinlets" : 1,
									"numoutlets" : 4,
									"patching_rect" : [ 552.0, 392.0, 33.0, 20.0 ],
									"margin" : 4,
									"outlettype" : [ "preset", "int", "preset", "int" ],
									"bubblesize" : 10,
									"id" : "obj-22",
									"spacing" : 2,
									"preset_data" : [ 										{
											"number" : 1,
											"data" : [ 5, "obj-17", "flonum", "float", -12.0, 5, "obj-15", "flonum", "float", 57.0, 5, "obj-9", "flonum", "float", 55.0 ]
										}
 ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"varname" : "csIN",
									"text" : "t l",
									"numinlets" : 1,
									"numoutlets" : 1,
									"patching_rect" : [ 39.0, 110.0, 21.0, 20.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-23",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "t b b b",
									"numinlets" : 1,
									"numoutlets" : 3,
									"patching_rect" : [ 333.0, 338.0, 45.0, 20.0 ],
									"outlettype" : [ "bang", "bang", "bang" ],
									"id" : "obj-24",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "ezdac~",
									"numinlets" : 2,
									"numoutlets" : 0,
									"patching_rect" : [ 88.0, 259.0, 57.0, 57.0 ],
									"id" : "obj-26"
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "button",
									"numinlets" : 1,
									"numoutlets" : 1,
									"patching_rect" : [ 333.0, 368.0, 20.0, 20.0 ],
									"outlettype" : [ "bang" ],
									"id" : "obj-27",
									"fgcolor" : [ 0.156863, 0.8, 0.54902, 1.0 ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "stop",
									"numinlets" : 2,
									"numoutlets" : 1,
									"patching_rect" : [ 294.0, 368.0, 35.0, 18.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-28",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "csound audio.csd",
									"numinlets" : 2,
									"bgcolor" : [ 1.0, 0.890196, 0.090196, 1.0 ],
									"numoutlets" : 1,
									"patching_rect" : [ 355.0, 368.0, 109.0, 18.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-29",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "csound~",
									"numinlets" : 2,
									"numoutlets" : 6,
									"patching_rect" : [ 88.0, 147.0, 112.0, 20.0 ],
									"outlettype" : [ "signal", "signal", "list", "int", "bang", "bang" ],
									"id" : "obj-30",
									"fontname" : "Arial",
									"fontface" : 3,
									"fontsize" : 12.0,
									"saved_object_attributes" : 									{
										"matchsr" : 1,
										"bypass" : 0,
										"overdrive" : 0,
										"output" : 1,
										"input" : 1,
										"interval" : 10,
										"message" : 1,
										"autostart" : 0,
										"args" : ""
									}

								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "csIN",
									"numinlets" : 1,
									"frgb" : [ 1.0, 0.035294, 0.035294, 1.0 ],
									"textcolor" : [ 1.0, 0.035294, 0.035294, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 8.0, 110.0, 33.0, 20.0 ],
									"id" : "obj-31",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "ch 1",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 99.0, 120.0, 40.0, 20.0 ],
									"id" : "obj-32",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "panel",
									"numinlets" : 1,
									"bgcolor" : [ 0.952941, 0.933333, 0.917647, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 300.0, 8.0, 281.0, 194.0 ],
									"id" : "obj-106",
									"rounded" : 0
								}

							}
 ],
						"lines" : [ 							{
								"patchline" : 								{
									"source" : [ "obj-13", 0 ],
									"destination" : [ "obj-30", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-30", 0 ],
									"destination" : [ "obj-18", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-30", 1 ],
									"destination" : [ "obj-18", 1 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-16", 0 ],
									"destination" : [ "obj-30", 1 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-23", 0 ],
									"destination" : [ "obj-30", 0 ],
									"hidden" : 0,
									"midpoints" : [ 48.5, 138.0, 97.5, 138.0 ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-14", 0 ],
									"destination" : [ "obj-16", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-15", 0 ],
									"destination" : [ "obj-14", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-17", 0 ],
									"destination" : [ "obj-18", 2 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-8", 0 ],
									"destination" : [ "obj-13", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-9", 0 ],
									"destination" : [ "obj-8", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-18", 0 ],
									"destination" : [ "obj-26", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-18", 1 ],
									"destination" : [ "obj-26", 1 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-18", 1 ],
									"destination" : [ "obj-2", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-41", 0 ],
									"destination" : [ "obj-24", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-24", 0 ],
									"destination" : [ "obj-27", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-24", 1 ],
									"destination" : [ "obj-29", 0 ],
									"hidden" : 0,
									"midpoints" : [ 355.5, 362.0, 364.5, 362.0 ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-24", 2 ],
									"destination" : [ "obj-20", 0 ],
									"hidden" : 0,
									"midpoints" : [ 368.5, 362.5, 477.5, 362.5 ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-21", 0 ],
									"destination" : [ "obj-22", 0 ],
									"hidden" : 1,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-20", 0 ],
									"destination" : [ "obj-19", 0 ],
									"hidden" : 0,
									"midpoints" : [ 477.5, 392.0, 342.5, 392.0 ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-29", 0 ],
									"destination" : [ "obj-19", 0 ],
									"hidden" : 0,
									"midpoints" : [ 364.5, 392.0, 342.5, 392.0 ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-27", 0 ],
									"destination" : [ "obj-19", 0 ],
									"hidden" : 0,
									"midpoints" : [ 342.5, 392.0, 342.5, 392.0 ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-28", 0 ],
									"destination" : [ "obj-19", 0 ],
									"hidden" : 0,
									"midpoints" : [ 303.5, 392.0, 342.5, 392.0 ]
								}

							}
 ]
					}
,
					"saved_object_attributes" : 					{
						"default_fontname" : "Arial",
						"default_fontsize" : 12.0,
						"fontname" : "Arial",
						"globalpatchername" : "",
						"fontface" : 0,
						"fontsize" : 12.0,
						"default_fontface" : 0
					}

				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "Open the csd file in the default csd text editor.\nYou can also double-click [csound~].",
					"linecount" : 2,
					"numinlets" : 1,
					"frgb" : [ 0.301961, 0.294118, 0.376471, 1.0 ],
					"textcolor" : [ 0.301961, 0.294118, 0.376471, 1.0 ],
					"numoutlets" : 0,
					"patching_rect" : [ 399.0, 321.0, 295.0, 34.0 ],
					"id" : "obj-1",
					"fontname" : "Arial",
					"fontface" : 1,
					"fontsize" : 12.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "p \"writebuf message\"",
					"numinlets" : 0,
					"numoutlets" : 0,
					"patching_rect" : [ 790.0, 320.0, 125.0, 20.0 ],
					"id" : "obj-2",
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"patcher" : 					{
						"fileversion" : 1,
						"rect" : [ 298.0, 188.0, 933.0, 520.0 ],
						"bglocked" : 0,
						"defrect" : [ 298.0, 188.0, 933.0, 520.0 ],
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
									"text" : "loadmess sizeinsamps 512",
									"numinlets" : 1,
									"numoutlets" : 1,
									"patching_rect" : [ 15.0, 15.0, 164.0, 20.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-36",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "csound~",
									"numinlets" : 1,
									"frgb" : [ 0.815686, 0.0, 0.0, 1.0 ],
									"textcolor" : [ 0.815686, 0.0, 0.0, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 832.0, 245.0, 57.0, 20.0 ],
									"id" : "obj-61",
									"fontname" : "Arial",
									"fontface" : 2,
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "amp",
									"numinlets" : 1,
									"frgb" : [ 0.815686, 0.0, 0.0, 1.0 ],
									"textcolor" : [ 0.815686, 0.0, 0.0, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 101.0, 209.0, 43.0, 20.0 ],
									"id" : "obj-58",
									"fontname" : "Arial",
									"fontface" : 2,
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "ezdac~",
									"numinlets" : 2,
									"bgcolor" : [ 0.0, 0.0, 0.0, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 15.0, 270.0, 44.0, 44.0 ],
									"offgradcolor1" : [ 0.392157, 0.392157, 0.392157, 1.0 ],
									"ongradcolor1" : [ 1.0, 0.0, 0.0, 1.0 ],
									"id" : "obj-118",
									"ongradcolor2" : [ 1.0, 1.0, 0.133333, 1.0 ],
									"offgradcolor2" : [ 0.0, 0.0, 0.0, 1.0 ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "number",
									"varname" : "amp",
									"numinlets" : 1,
									"numoutlets" : 2,
									"patching_rect" : [ 52.0, 209.0, 50.0, 20.0 ],
									"outlettype" : [ "int", "bang" ],
									"id" : "obj-59",
									"fontname" : "Arial",
									"maximum" : 12,
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "p amp",
									"numinlets" : 3,
									"numoutlets" : 2,
									"patching_rect" : [ 15.0, 240.0, 56.0, 20.0 ],
									"outlettype" : [ "signal", "signal" ],
									"id" : "obj-60",
									"fontname" : "Arial",
									"fontsize" : 12.0,
									"patcher" : 									{
										"fileversion" : 1,
										"rect" : [ 20.0, 74.0, 350.0, 322.0 ],
										"bglocked" : 0,
										"defrect" : [ 20.0, 74.0, 350.0, 322.0 ],
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
										"boxes" : [ 											{
												"box" : 												{
													"maxclass" : "outlet",
													"numinlets" : 1,
													"numoutlets" : 0,
													"patching_rect" : [ 90.0, 148.0, 21.0, 21.0 ],
													"id" : "obj-1",
													"comment" : ""
												}

											}
, 											{
												"box" : 												{
													"maxclass" : "outlet",
													"numinlets" : 1,
													"numoutlets" : 0,
													"patching_rect" : [ 33.0, 148.0, 21.0, 21.0 ],
													"id" : "obj-2",
													"comment" : ""
												}

											}
, 											{
												"box" : 												{
													"maxclass" : "newobj",
													"text" : "dbtoa",
													"numinlets" : 1,
													"numoutlets" : 1,
													"patching_rect" : [ 129.0, 70.0, 48.0, 20.0 ],
													"outlettype" : [ "" ],
													"id" : "obj-3",
													"fontname" : "Arial",
													"fontsize" : 12.0
												}

											}
, 											{
												"box" : 												{
													"maxclass" : "newobj",
													"text" : "*~ 1.",
													"numinlets" : 2,
													"numoutlets" : 1,
													"patching_rect" : [ 90.0, 105.0, 49.0, 20.0 ],
													"outlettype" : [ "signal" ],
													"id" : "obj-4",
													"fontname" : "Arial",
													"fontsize" : 12.0
												}

											}
, 											{
												"box" : 												{
													"maxclass" : "newobj",
													"text" : "*~ 1.",
													"numinlets" : 2,
													"numoutlets" : 1,
													"patching_rect" : [ 33.0, 105.0, 49.0, 20.0 ],
													"outlettype" : [ "signal" ],
													"id" : "obj-5",
													"fontname" : "Arial",
													"fontsize" : 12.0
												}

											}
, 											{
												"box" : 												{
													"maxclass" : "inlet",
													"numinlets" : 0,
													"numoutlets" : 1,
													"patching_rect" : [ 129.0, 41.0, 21.0, 21.0 ],
													"outlettype" : [ "int" ],
													"id" : "obj-6",
													"comment" : ""
												}

											}
, 											{
												"box" : 												{
													"maxclass" : "inlet",
													"numinlets" : 0,
													"numoutlets" : 1,
													"patching_rect" : [ 90.0, 41.0, 21.0, 21.0 ],
													"outlettype" : [ "signal" ],
													"id" : "obj-7",
													"comment" : ""
												}

											}
, 											{
												"box" : 												{
													"maxclass" : "inlet",
													"numinlets" : 0,
													"numoutlets" : 1,
													"patching_rect" : [ 33.0, 41.0, 21.0, 21.0 ],
													"outlettype" : [ "signal" ],
													"id" : "obj-8",
													"comment" : ""
												}

											}
 ],
										"lines" : [ 											{
												"patchline" : 												{
													"source" : [ "obj-3", 0 ],
													"destination" : [ "obj-4", 1 ],
													"hidden" : 0,
													"midpoints" : [  ]
												}

											}
, 											{
												"patchline" : 												{
													"source" : [ "obj-6", 0 ],
													"destination" : [ "obj-3", 0 ],
													"hidden" : 0,
													"midpoints" : [  ]
												}

											}
, 											{
												"patchline" : 												{
													"source" : [ "obj-4", 0 ],
													"destination" : [ "obj-1", 0 ],
													"hidden" : 0,
													"midpoints" : [  ]
												}

											}
, 											{
												"patchline" : 												{
													"source" : [ "obj-7", 0 ],
													"destination" : [ "obj-4", 0 ],
													"hidden" : 0,
													"midpoints" : [  ]
												}

											}
, 											{
												"patchline" : 												{
													"source" : [ "obj-3", 0 ],
													"destination" : [ "obj-5", 1 ],
													"hidden" : 0,
													"midpoints" : [  ]
												}

											}
, 											{
												"patchline" : 												{
													"source" : [ "obj-5", 0 ],
													"destination" : [ "obj-2", 0 ],
													"hidden" : 0,
													"midpoints" : [  ]
												}

											}
, 											{
												"patchline" : 												{
													"source" : [ "obj-8", 0 ],
													"destination" : [ "obj-5", 0 ],
													"hidden" : 0,
													"midpoints" : [  ]
												}

											}
 ]
									}
,
									"saved_object_attributes" : 									{
										"default_fontname" : "Arial",
										"default_fontsize" : 12.0,
										"fontname" : "Arial",
										"globalpatchername" : "",
										"fontface" : 0,
										"fontsize" : 12.0,
										"default_fontface" : 0
									}

								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "1) Start DSP.\n2) Start [csound~].\n3) Select a Csound table.\n4) Copy the Csound table to [buffer~ wtab].",
									"linecount" : 4,
									"numinlets" : 1,
									"frgb" : [ 0.0, 0.14902, 0.964706, 1.0 ],
									"textcolor" : [ 0.0, 0.14902, 0.964706, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 630.0, 390.0, 283.0, 71.0 ],
									"id" : "obj-54",
									"fontname" : "Arial",
									"fontsize" : 14.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "1",
									"numinlets" : 1,
									"frgb" : [ 0.176471, 0.301961, 1.0, 1.0 ],
									"textcolor" : [ 0.176471, 0.301961, 1.0, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 57.0, 268.0, 38.0, 48.0 ],
									"id" : "obj-53",
									"fontname" : "Arial",
									"fontface" : 1,
									"fontsize" : 36.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "4",
									"numinlets" : 1,
									"frgb" : [ 0.176471, 0.301961, 1.0, 1.0 ],
									"textcolor" : [ 0.176471, 0.301961, 1.0, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 142.0, 267.0, 38.0, 48.0 ],
									"id" : "obj-51",
									"fontname" : "Arial",
									"fontface" : 1,
									"fontsize" : 36.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "3",
									"numinlets" : 1,
									"frgb" : [ 0.176471, 0.301961, 1.0, 1.0 ],
									"textcolor" : [ 0.176471, 0.301961, 1.0, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 325.0, 106.0, 38.0, 48.0 ],
									"id" : "obj-50",
									"fontname" : "Arial",
									"fontface" : 1,
									"fontsize" : 36.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "2",
									"numinlets" : 1,
									"frgb" : [ 0.176471, 0.301961, 1.0, 1.0 ],
									"textcolor" : [ 0.176471, 0.301961, 1.0, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 678.0, 115.0, 38.0, 48.0 ],
									"id" : "obj-15",
									"fontname" : "Arial",
									"fontface" : 1,
									"fontsize" : 36.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "loadmess 1",
									"numinlets" : 1,
									"hidden" : 1,
									"numoutlets" : 1,
									"patching_rect" : [ 810.0, 15.0, 74.0, 20.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-1",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "Click this button to start [csound~].",
									"linecount" : 2,
									"numinlets" : 1,
									"frgb" : [ 0.337255, 0.254902, 0.113725, 1.0 ],
									"textcolor" : [ 0.337255, 0.254902, 0.113725, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 793.0, 128.0, 130.0, 39.0 ],
									"id" : "obj-52",
									"fontname" : "Arial",
									"fontsize" : 14.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "button",
									"numinlets" : 1,
									"numoutlets" : 1,
									"patching_rect" : [ 706.0, 100.0, 87.0, 87.0 ],
									"outlettype" : [ "bang" ],
									"id" : "obj-42",
									"fgcolor" : [ 0.0, 1.0, 0.301961, 1.0 ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "csound~ 1.0.4 or later required.",
									"numinlets" : 1,
									"frgb" : [ 0.65098, 0.0, 0.019608, 1.0 ],
									"textcolor" : [ 0.65098, 0.0, 0.019608, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 252.0, 60.0, 179.0, 20.0 ],
									"id" : "obj-2",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "In this example, the writebuf.csd orchestra has 3 tables. The tables contain single cycles of 3 wave types. Clicking on the red button will load one of them in the buffer~ named \"wtab\".",
									"linecount" : 3,
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 429.0, 44.0, 359.0, 48.0 ],
									"id" : "obj-3",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "Bang me to send \"writebuf\" message and set [cycle~] to play the copied data.",
									"linecount" : 4,
									"numinlets" : 1,
									"frgb" : [ 0.407843, 0.0, 0.0, 1.0 ],
									"textcolor" : [ 0.407843, 0.0, 0.0, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 142.0, 209.0, 129.0, 62.0 ],
									"id" : "obj-4",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "+ 1",
									"numinlets" : 2,
									"numoutlets" : 1,
									"patching_rect" : [ 248.0, 143.0, 34.0, 20.0 ],
									"outlettype" : [ "int" ],
									"id" : "obj-5",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "umenu",
									"numinlets" : 1,
									"items" : [ "sine", ",", "triangle", ",", "square" ],
									"numoutlets" : 3,
									"patching_rect" : [ 248.0, 120.0, 76.0, 20.0 ],
									"outlettype" : [ "int", "", "" ],
									"id" : "obj-6",
									"fontname" : "Arial",
									"fontsize" : 12.0,
									"types" : [  ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "set wtab",
									"numinlets" : 2,
									"hidden" : 1,
									"numoutlets" : 1,
									"patching_rect" : [ 145.0, 343.0, 55.0, 18.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-7",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "t b b",
									"numinlets" : 1,
									"hidden" : 1,
									"numoutlets" : 2,
									"patching_rect" : [ 173.0, 318.0, 37.0, 20.0 ],
									"outlettype" : [ "bang", "bang" ],
									"id" : "obj-8",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "scope~",
									"numinlets" : 2,
									"bgcolor" : [ 0.219608, 0.235294, 0.215686, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 15.0, 90.0, 191.0, 78.0 ],
									"id" : "obj-9",
									"rounded" : 0,
									"fgcolor" : [ 1.0, 1.0, 1.0, 1.0 ],
									"range" : [ -1.01, 1.01 ],
									"calccount" : 16
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "cycle~ 64.7 wtab",
									"numinlets" : 2,
									"bgcolor" : [ 0.917647, 0.764706, 0.984314, 1.0 ],
									"numoutlets" : 1,
									"patching_rect" : [ 15.0, 180.0, 100.0, 20.0 ],
									"outlettype" : [ "signal" ],
									"id" : "obj-10",
									"fontname" : "Arial",
									"color" : [ 1.0, 1.0, 1.0, 0.0 ],
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "If > 0, copy one channel (1 = left, 2 = right, etc...).",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 321.0, 243.0, 274.0, 20.0 ],
									"id" : "obj-12",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "If = 0, copy all channels (assume Csound table has the same number of channels as buffer~).",
									"linecount" : 2,
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 321.0, 208.0, 330.0, 34.0 ],
									"id" : "obj-13",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "number",
									"numinlets" : 1,
									"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
									"numoutlets" : 2,
									"patching_rect" : [ 248.0, 166.0, 37.0, 20.0 ],
									"triscale" : 0.9,
									"minimum" : 1,
									"outlettype" : [ "int", "bang" ],
									"id" : "obj-14",
									"fontname" : "Arial",
									"fontsize" : 12.0,
									"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "preset",
									"numinlets" : 1,
									"numoutlets" : 4,
									"patching_rect" : [ 810.0, 38.0, 37.0, 22.0 ],
									"margin" : 4,
									"outlettype" : [ "preset", "int", "preset", "int" ],
									"bubblesize" : 12,
									"id" : "obj-16",
									"spacing" : 2,
									"preset_data" : [ 										{
											"number" : 1,
											"data" : [ 6, "<invalid>", "gain~", "list", 100, 10.0, 6, "<invalid>", "gain~", "list", 100, 10.0, 5, "<invalid>", "number", "int", 100, 5, "obj-30", "number", "int", 0, 5, "obj-26", "flonum", "float", 0.0, 5, "obj-24", "flonum", "float", 0.0, 5, "obj-14", "number", "int", 1, 5, "obj-6", "umenu", "int", 0 ]
										}
 ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "The \"writebuf\" message will copy audio data from a Csound table to a [buffer~].",
									"linecount" : 2,
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 429.0, 11.0, 359.0, 34.0 ],
									"id" : "obj-17",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "Copy 128 samples (starting at time 0 seconds) from Csound table 7 to channel 1 of buffer~ \"mybuf\". Treat Csound table 7 as a single channel table.",
									"linecount" : 3,
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 287.0, 463.0, 297.0, 48.0 ],
									"id" : "obj-18",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "writebuf 7 1 wtab 0. -128",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 127.0, 463.0, 157.0, 20.0 ],
									"id" : "obj-19",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "Copy 12 seconds of multichannel audio (starting at time 0 seconds) from Csound table 1 to buffer~ \"mybuf\". Assume table has same number of channels as buffer~.",
									"linecount" : 3,
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 287.0, 409.0, 320.0, 48.0 ],
									"id" : "obj-20",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "Examples:",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 127.0, 388.0, 162.0, 20.0 ],
									"id" : "obj-21",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "writebuf 1 0 wtab 0. 12.",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 127.0, 409.0, 159.0, 20.0 ],
									"id" : "obj-22",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "arg #5 (optional, default = 0): Amount of time to read in seconds. 0 means read in all data or as much data as possible. If negative, time is in sample frames.",
									"linecount" : 3,
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 433.0, 322.0, 310.0, 48.0 ],
									"id" : "obj-23",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "flonum",
									"numinlets" : 1,
									"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
									"numoutlets" : 2,
									"patching_rect" : [ 392.0, 322.0, 40.0, 20.0 ],
									"triscale" : 0.9,
									"outlettype" : [ "float", "bang" ],
									"id" : "obj-24",
									"fontname" : "Arial",
									"fontsize" : 12.0,
									"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "arg #4 (optional, default = 0): Read offset time in seconds. If negative, time is in sample frames.",
									"linecount" : 2,
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 397.0, 287.0, 277.0, 34.0 ],
									"id" : "obj-25",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "flonum",
									"numinlets" : 1,
									"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
									"numoutlets" : 2,
									"patching_rect" : [ 356.0, 287.0, 40.0, 20.0 ],
									"triscale" : 0.9,
									"outlettype" : [ "float", "bang" ],
									"id" : "obj-26",
									"fontname" : "Arial",
									"fontsize" : 12.0,
									"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "arg #1: Csound table number to read from.",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 285.0, 166.0, 275.0, 20.0 ],
									"id" : "obj-27",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "arg #3: buffer~ name to write to.",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 358.0, 264.0, 187.0, 20.0 ],
									"id" : "obj-28",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "wtab",
									"numinlets" : 2,
									"numoutlets" : 1,
									"patching_rect" : [ 320.0, 266.0, 36.0, 18.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-29",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "number",
									"numinlets" : 1,
									"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
									"numoutlets" : 2,
									"patching_rect" : [ 284.0, 187.0, 37.0, 20.0 ],
									"triscale" : 0.9,
									"minimum" : 0,
									"outlettype" : [ "int", "bang" ],
									"id" : "obj-30",
									"fontname" : "Arial",
									"fontsize" : 12.0,
									"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "pattrforward csound~",
									"numinlets" : 1,
									"hidden" : 1,
									"numoutlets" : 1,
									"patching_rect" : [ 212.0, 367.0, 126.0, 20.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-31",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "button",
									"numinlets" : 1,
									"numoutlets" : 1,
									"patching_rect" : [ 173.0, 269.0, 47.0, 47.0 ],
									"outlettype" : [ "bang" ],
									"id" : "obj-32",
									"fgcolor" : [ 1.0, 0.0, 0.0, 1.0 ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "buffer~ wtab",
									"numinlets" : 1,
									"bgcolor" : [ 0.917647, 0.764706, 0.984314, 1.0 ],
									"numoutlets" : 2,
									"patching_rect" : [ 15.0, 45.0, 77.0, 20.0 ],
									"outlettype" : [ "float", "bang" ],
									"id" : "obj-33",
									"fontname" : "Arial",
									"color" : [ 1.0, 1.0, 1.0, 0.0 ],
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "pack writebuf 1 1 wtab 0. 0.",
									"numinlets" : 6,
									"numoutlets" : 1,
									"patching_rect" : [ 212.0, 343.0, 199.0, 20.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-34",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "writebuf",
									"numinlets" : 1,
									"frgb" : [ 0.337255, 0.254902, 0.113725, 1.0 ],
									"textcolor" : [ 0.337255, 0.254902, 0.113725, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 252.0, 11.0, 159.0, 48.0 ],
									"id" : "obj-35",
									"fontname" : "Arial",
									"fontsize" : 36.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "t b b",
									"numinlets" : 1,
									"hidden" : 1,
									"numoutlets" : 2,
									"patching_rect" : [ 706.0, 190.0, 36.0, 20.0 ],
									"outlettype" : [ "bang", "bang" ],
									"id" : "obj-41",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "button",
									"numinlets" : 1,
									"numoutlets" : 1,
									"patching_rect" : [ 706.0, 215.0, 15.0, 15.0 ],
									"outlettype" : [ "bang" ],
									"id" : "obj-44",
									"fgcolor" : [ 1.0, 0.890196, 0.090196, 1.0 ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "stop",
									"numinlets" : 2,
									"numoutlets" : 1,
									"patching_rect" : [ 669.0, 215.0, 35.0, 18.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-45",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "csound writebuf.csd",
									"numinlets" : 2,
									"bgcolor" : [ 1.0, 0.890196, 0.090196, 1.0 ],
									"numoutlets" : 1,
									"patching_rect" : [ 723.0, 215.0, 121.0, 18.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-46",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"varname" : "csound~",
									"text" : "csound~",
									"numinlets" : 2,
									"numoutlets" : 6,
									"patching_rect" : [ 723.0, 245.0, 108.0, 20.0 ],
									"outlettype" : [ "signal", "signal", "list", "int", "bang", "bang" ],
									"id" : "obj-47",
									"fontname" : "Arial",
									"fontsize" : 12.0,
									"saved_object_attributes" : 									{
										"matchsr" : 1,
										"bypass" : 0,
										"overdrive" : 0,
										"output" : 1,
										"input" : 1,
										"interval" : 10,
										"message" : 1,
										"autostart" : 0,
										"args" : ""
									}

								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "arg #2: Target buffer~ channel.",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 321.0, 187.0, 207.0, 20.0 ],
									"id" : "obj-49",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "panel",
									"numinlets" : 1,
									"bgcolor" : [ 0.929412, 1.0, 0.52549, 1.0 ],
									"numoutlets" : 0,
									"border" : 1,
									"patching_rect" : [ 247.0, 3.0, 543.0, 91.0 ],
									"id" : "obj-106",
									"rounded" : 0
								}

							}
 ],
						"lines" : [ 							{
								"patchline" : 								{
									"source" : [ "obj-36", 0 ],
									"destination" : [ "obj-33", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-34", 0 ],
									"destination" : [ "obj-31", 0 ],
									"hidden" : 1,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-10", 0 ],
									"destination" : [ "obj-60", 1 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-10", 0 ],
									"destination" : [ "obj-60", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-59", 0 ],
									"destination" : [ "obj-60", 2 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-60", 1 ],
									"destination" : [ "obj-118", 1 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-60", 0 ],
									"destination" : [ "obj-118", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-24", 0 ],
									"destination" : [ "obj-34", 5 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-26", 0 ],
									"destination" : [ "obj-34", 4 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-29", 0 ],
									"destination" : [ "obj-34", 3 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-30", 0 ],
									"destination" : [ "obj-34", 2 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-14", 0 ],
									"destination" : [ "obj-34", 1 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-8", 1 ],
									"destination" : [ "obj-34", 0 ],
									"hidden" : 1,
									"midpoints" : [ 200.5, 339.0, 221.5, 339.0 ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-45", 0 ],
									"destination" : [ "obj-47", 0 ],
									"hidden" : 1,
									"midpoints" : [ 678.5, 238.0, 732.5, 238.0 ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-44", 0 ],
									"destination" : [ "obj-47", 0 ],
									"hidden" : 1,
									"midpoints" : [ 715.0, 238.0, 732.5, 238.0 ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-46", 0 ],
									"destination" : [ "obj-47", 0 ],
									"hidden" : 1,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-5", 0 ],
									"destination" : [ "obj-14", 0 ],
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
									"source" : [ "obj-32", 0 ],
									"destination" : [ "obj-8", 0 ],
									"hidden" : 1,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-8", 0 ],
									"destination" : [ "obj-7", 0 ],
									"hidden" : 1,
									"midpoints" : [ 182.5, 339.0, 154.5, 339.0 ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-42", 0 ],
									"destination" : [ "obj-41", 0 ],
									"hidden" : 1,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-41", 0 ],
									"destination" : [ "obj-44", 0 ],
									"hidden" : 1,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-41", 1 ],
									"destination" : [ "obj-46", 0 ],
									"hidden" : 1,
									"midpoints" : [ 732.5, 211.0, 732.5, 211.0 ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-1", 0 ],
									"destination" : [ "obj-16", 0 ],
									"hidden" : 1,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-7", 0 ],
									"destination" : [ "obj-10", 0 ],
									"hidden" : 1,
									"midpoints" : [ 154.5, 386.0, 9.0, 386.0, 9.0, 172.0, 24.5, 172.0 ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-10", 0 ],
									"destination" : [ "obj-9", 0 ],
									"hidden" : 1,
									"midpoints" : [  ]
								}

							}
 ]
					}
,
					"saved_object_attributes" : 					{
						"default_fontname" : "Arial",
						"default_fontsize" : 12.0,
						"fontname" : "Arial",
						"globalpatchername" : "",
						"fontface" : 0,
						"fontsize" : 12.0,
						"default_fontface" : 0
					}

				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"varname" : "readbuf message",
					"text" : "p \"readbuf message\"",
					"numinlets" : 0,
					"numoutlets" : 0,
					"patching_rect" : [ 790.0, 234.0, 124.0, 20.0 ],
					"id" : "obj-3",
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"patcher" : 					{
						"fileversion" : 1,
						"rect" : [ 461.0, 161.0, 800.0, 546.0 ],
						"bglocked" : 0,
						"defrect" : [ 461.0, 161.0, 800.0, 546.0 ],
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
									"maxclass" : "comment",
									"text" : "readbuf reads [buffer~] audio data\ninto a Csound table.",
									"linecount" : 2,
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 15.0, 0.0, 222.0, 34.0 ],
									"id" : "obj-33",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "Play instr 1",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 336.0, 326.0, 68.0, 20.0 ],
									"id" : "obj-4",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "3",
									"numinlets" : 1,
									"frgb" : [ 0.176471, 0.301961, 1.0, 1.0 ],
									"textcolor" : [ 0.176471, 0.301961, 1.0, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 544.0, 0.0, 38.0, 48.0 ],
									"id" : "obj-7",
									"fontname" : "Arial",
									"fontface" : 1,
									"fontsize" : 36.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "drop audio files here",
									"numinlets" : 1,
									"frgb" : [ 0.337255, 0.254902, 0.113725, 1.0 ],
									"textcolor" : [ 0.337255, 0.254902, 0.113725, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 475.0, 61.0, 184.0, 27.0 ],
									"id" : "obj-28",
									"fontname" : "Arial",
									"fontface" : 2,
									"fontsize" : 18.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "dropfile",
									"numinlets" : 1,
									"numoutlets" : 2,
									"bordercolor" : [ 0.564706, 0.564706, 0.564706, 1.0 ],
									"patching_rect" : [ 465.0, 0.0, 197.0, 90.0 ],
									"outlettype" : [ "", "" ],
									"id" : "obj-48",
									"ignoreclick" : 1,
									"types" : [  ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "To avoid crashes when replacing tables, make sure no instruments are accessing that table.",
									"linecount" : 3,
									"numinlets" : 1,
									"frgb" : [ 0.666667, 0.0, 0.0, 1.0 ],
									"textcolor" : [ 0.666667, 0.0, 0.0, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 15.0, 109.0, 198.0, 48.0 ],
									"id" : "obj-36",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "1 - Start DSP\n2 - Start Csound\n3 - Load a file into mybuf\n4 - Read mybuf into Csound table\n5 - Play instr 1",
									"linecount" : 5,
									"numinlets" : 1,
									"frgb" : [ 0.176471, 0.301961, 1.0, 1.0 ],
									"textcolor" : [ 0.176471, 0.301961, 1.0, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 15.0, 34.0, 198.0, 75.0 ],
									"id" : "obj-34",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "create instance of instr 1",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 438.0, 404.0, 148.0, 20.0 ],
									"id" : "obj-12",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "2",
									"numinlets" : 1,
									"frgb" : [ 0.176471, 0.301961, 1.0, 1.0 ],
									"textcolor" : [ 0.176471, 0.301961, 1.0, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 364.0, 0.0, 38.0, 48.0 ],
									"id" : "obj-1",
									"fontname" : "Arial",
									"fontface" : 1,
									"fontsize" : 36.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "flonum",
									"numinlets" : 1,
									"numoutlets" : 2,
									"patching_rect" : [ 420.0, 375.0, 50.0, 20.0 ],
									"minimum" : 0.1,
									"outlettype" : [ "float", "bang" ],
									"id" : "obj-79",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "amp",
									"numinlets" : 1,
									"frgb" : [ 0.815686, 0.0, 0.0, 1.0 ],
									"textcolor" : [ 0.815686, 0.0, 0.0, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 396.0, 75.0, 43.0, 20.0 ],
									"id" : "obj-54",
									"fontname" : "Arial",
									"fontface" : 2,
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"varname" : "_",
									"text" : "pattr _ @bindto length",
									"numinlets" : 1,
									"numoutlets" : 3,
									"patching_rect" : [ 420.0, 345.0, 130.0, 20.0 ],
									"outlettype" : [ "", "", "" ],
									"id" : "obj-41",
									"fontname" : "Arial",
									"fontsize" : 12.0,
									"restore" : [ 10.078095 ],
									"saved_object_attributes" : 									{
										"parameter_enable" : 0
									}

								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "length",
									"numinlets" : 1,
									"frgb" : [ 0.815686, 0.0, 0.0, 1.0 ],
									"textcolor" : [ 0.815686, 0.0, 0.0, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 728.0, 132.0, 43.0, 20.0 ],
									"id" : "obj-16",
									"fontname" : "Arial",
									"fontface" : 2,
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "start",
									"numinlets" : 2,
									"bgcolor" : [ 0.835294, 1.0, 0.698039, 1.0 ],
									"numoutlets" : 1,
									"patching_rect" : [ 330.0, 15.0, 34.0, 18.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-27",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "ezdac~",
									"numinlets" : 2,
									"bgcolor" : [ 0.0, 0.0, 0.0, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 285.0, 105.0, 44.0, 44.0 ],
									"offgradcolor1" : [ 0.392157, 0.392157, 0.392157, 1.0 ],
									"ongradcolor1" : [ 1.0, 0.0, 0.0, 1.0 ],
									"id" : "obj-118",
									"ongradcolor2" : [ 1.0, 1.0, 0.133333, 1.0 ],
									"offgradcolor2" : [ 0.0, 0.0, 0.0, 1.0 ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "4",
									"numinlets" : 1,
									"frgb" : [ 0.176471, 0.301961, 1.0, 1.0 ],
									"textcolor" : [ 0.176471, 0.301961, 1.0, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 7.0, 172.0, 38.0, 48.0 ],
									"id" : "obj-25",
									"fontname" : "Arial",
									"fontface" : 1,
									"fontsize" : 36.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "Don't replace table 1.  Load all channels.  Use mybuf as source.  Start reading at time 0. Read 44100 frames.",
									"linecount" : 2,
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 168.0, 283.0, 315.0, 34.0 ],
									"id" : "obj-21",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "Replace table 1 (may crash if not careful).  Load right channel.  Use mybuf as source.  Start reading at 2 seconds. Read 16 seconds.",
									"linecount" : 2,
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 149.0, 232.0, 377.0, 34.0 ],
									"id" : "obj-20",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "Don't replace table 1.  Load left channel.  Use mybuf as source.  Start reading at time 0. Read 3 seconds.",
									"linecount" : 2,
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 138.0, 180.0, 299.0, 34.0 ],
									"id" : "obj-18",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "5",
									"numinlets" : 1,
									"frgb" : [ 0.176471, 0.301961, 1.0, 1.0 ],
									"textcolor" : [ 0.176471, 0.301961, 1.0, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 315.0, 345.0, 38.0, 48.0 ],
									"id" : "obj-15",
									"fontname" : "Arial",
									"fontface" : 1,
									"fontsize" : 36.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "flonum",
									"varname" : "length",
									"numinlets" : 1,
									"numoutlets" : 2,
									"patching_rect" : [ 660.0, 132.0, 68.0, 23.0 ],
									"outlettype" : [ "float", "bang" ],
									"id" : "obj-14",
									"fontname" : "Arial",
									"fontsize" : 14.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "p Get File Length (sec)",
									"numinlets" : 1,
									"numoutlets" : 1,
									"patching_rect" : [ 660.0, 102.0, 134.0, 20.0 ],
									"outlettype" : [ "float" ],
									"id" : "obj-8",
									"fontname" : "Arial",
									"fontsize" : 12.0,
									"patcher" : 									{
										"fileversion" : 1,
										"rect" : [ 656.0, 96.0, 226.0, 220.0 ],
										"bglocked" : 0,
										"defrect" : [ 656.0, 96.0, 226.0, 220.0 ],
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
										"boxes" : [ 											{
												"box" : 												{
													"maxclass" : "outlet",
													"numinlets" : 1,
													"numoutlets" : 0,
													"patching_rect" : [ 90.0, 150.0, 27.0, 27.0 ],
													"id" : "obj-1",
													"comment" : ""
												}

											}
, 											{
												"box" : 												{
													"maxclass" : "inlet",
													"numinlets" : 0,
													"numoutlets" : 1,
													"patching_rect" : [ 45.0, 30.0, 27.0, 27.0 ],
													"outlettype" : [ "" ],
													"id" : "obj-2",
													"comment" : ""
												}

											}
, 											{
												"box" : 												{
													"maxclass" : "newobj",
													"text" : "* 0.001",
													"numinlets" : 2,
													"numoutlets" : 1,
													"patching_rect" : [ 90.0, 120.0, 69.0, 20.0 ],
													"outlettype" : [ "float" ],
													"id" : "obj-3",
													"fontname" : "Arial",
													"fontsize" : 12.0
												}

											}
, 											{
												"box" : 												{
													"maxclass" : "newobj",
													"text" : "sfinfo~",
													"numinlets" : 1,
													"numoutlets" : 6,
													"patching_rect" : [ 45.0, 90.0, 94.0, 20.0 ],
													"outlettype" : [ "int", "int", "float", "float", "", "" ],
													"id" : "obj-4",
													"fontname" : "Arial",
													"fontsize" : 12.0
												}

											}
, 											{
												"box" : 												{
													"maxclass" : "newobj",
													"text" : "prepend open",
													"numinlets" : 1,
													"numoutlets" : 1,
													"patching_rect" : [ 45.0, 60.0, 91.0, 20.0 ],
													"outlettype" : [ "" ],
													"id" : "obj-5",
													"fontname" : "Arial",
													"fontsize" : 12.0
												}

											}
 ],
										"lines" : [ 											{
												"patchline" : 												{
													"source" : [ "obj-2", 0 ],
													"destination" : [ "obj-5", 0 ],
													"hidden" : 0,
													"midpoints" : [  ]
												}

											}
, 											{
												"patchline" : 												{
													"source" : [ "obj-5", 0 ],
													"destination" : [ "obj-4", 0 ],
													"hidden" : 0,
													"midpoints" : [  ]
												}

											}
, 											{
												"patchline" : 												{
													"source" : [ "obj-4", 3 ],
													"destination" : [ "obj-3", 0 ],
													"hidden" : 0,
													"midpoints" : [  ]
												}

											}
, 											{
												"patchline" : 												{
													"source" : [ "obj-3", 0 ],
													"destination" : [ "obj-1", 0 ],
													"hidden" : 0,
													"midpoints" : [  ]
												}

											}
 ]
									}
,
									"saved_object_attributes" : 									{
										"default_fontname" : "Arial",
										"default_fontsize" : 12.0,
										"fontname" : "Arial",
										"globalpatchername" : "",
										"fontface" : 0,
										"fontsize" : 12.0,
										"default_fontface" : 0
									}

								}

							}
, 							{
								"box" : 								{
									"maxclass" : "button",
									"numinlets" : 1,
									"numoutlets" : 1,
									"patching_rect" : [ 667.0, 21.0, 29.0, 29.0 ],
									"outlettype" : [ "bang" ],
									"id" : "obj-26",
									"fgcolor" : [ 1.0, 0.0, 0.0, 1.0 ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "opendialog",
									"numinlets" : 1,
									"numoutlets" : 2,
									"patching_rect" : [ 667.0, 52.0, 71.0, 20.0 ],
									"outlettype" : [ "", "bang" ],
									"id" : "obj-9",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "pak read name 0. -1. 2",
									"numinlets" : 5,
									"numoutlets" : 1,
									"patching_rect" : [ 495.0, 102.0, 137.0, 20.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-11",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "buffer~ mybuf 1 2",
									"numinlets" : 1,
									"numoutlets" : 2,
									"patching_rect" : [ 495.0, 132.0, 153.0, 23.0 ],
									"outlettype" : [ "float", "bang" ],
									"id" : "obj-31",
									"fontname" : "Arial",
									"fontface" : 1,
									"fontsize" : 14.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "set_and_send",
									"numinlets" : 1,
									"frgb" : [ 0.815686, 0.0, 0.0, 1.0 ],
									"textcolor" : [ 0.815686, 0.0, 0.0, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 131.0, 450.0, 89.0, 20.0 ],
									"id" : "obj-5",
									"fontname" : "Arial",
									"fontface" : 2,
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"varname" : "set_and_send",
									"text" : "p set_and_send",
									"numinlets" : 1,
									"numoutlets" : 1,
									"patching_rect" : [ 30.0, 450.0, 100.0, 20.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-2",
									"fontname" : "Arial",
									"fontsize" : 12.0,
									"patcher" : 									{
										"fileversion" : 1,
										"rect" : [ 95.0, 75.0, 277.0, 240.0 ],
										"bglocked" : 0,
										"defrect" : [ 95.0, 75.0, 277.0, 240.0 ],
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
										"boxes" : [ 											{
												"box" : 												{
													"maxclass" : "newobj",
													"text" : "t l l",
													"numinlets" : 1,
													"numoutlets" : 2,
													"patching_rect" : [ 90.0, 75.0, 32.5, 20.0 ],
													"outlettype" : [ "", "" ],
													"id" : "obj-6",
													"fontname" : "Arial",
													"fontsize" : 12.0
												}

											}
, 											{
												"box" : 												{
													"maxclass" : "newobj",
													"text" : "prepend set",
													"numinlets" : 1,
													"numoutlets" : 1,
													"patching_rect" : [ 135.0, 105.0, 76.0, 20.0 ],
													"outlettype" : [ "" ],
													"id" : "obj-3",
													"fontname" : "Arial",
													"fontsize" : 12.0
												}

											}
, 											{
												"box" : 												{
													"maxclass" : "outlet",
													"numinlets" : 1,
													"numoutlets" : 0,
													"patching_rect" : [ 90.0, 150.0, 25.0, 25.0 ],
													"id" : "obj-2",
													"comment" : ""
												}

											}
, 											{
												"box" : 												{
													"maxclass" : "inlet",
													"numinlets" : 0,
													"numoutlets" : 1,
													"patching_rect" : [ 90.0, 45.0, 25.0, 25.0 ],
													"outlettype" : [ "" ],
													"id" : "obj-1",
													"comment" : ""
												}

											}
 ],
										"lines" : [ 											{
												"patchline" : 												{
													"source" : [ "obj-6", 0 ],
													"destination" : [ "obj-2", 0 ],
													"hidden" : 0,
													"midpoints" : [  ]
												}

											}
, 											{
												"patchline" : 												{
													"source" : [ "obj-3", 0 ],
													"destination" : [ "obj-2", 0 ],
													"hidden" : 0,
													"midpoints" : [  ]
												}

											}
, 											{
												"patchline" : 												{
													"source" : [ "obj-6", 1 ],
													"destination" : [ "obj-3", 0 ],
													"hidden" : 0,
													"midpoints" : [  ]
												}

											}
, 											{
												"patchline" : 												{
													"source" : [ "obj-1", 0 ],
													"destination" : [ "obj-6", 0 ],
													"hidden" : 0,
													"midpoints" : [  ]
												}

											}
 ]
									}
,
									"saved_object_attributes" : 									{
										"default_fontname" : "Arial",
										"default_fontsize" : 12.0,
										"fontname" : "Arial",
										"globalpatchername" : "",
										"fontface" : 0,
										"fontsize" : 12.0,
										"default_fontface" : 0
									}

								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "Last readbuf message sent to csound~:",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 255.0, 460.0, 237.0, 20.0 ],
									"id" : "obj-71",
									"fontname" : "Arial",
									"fontface" : 2,
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "button",
									"numinlets" : 1,
									"numoutlets" : 1,
									"patching_rect" : [ 30.0, 390.0, 20.0, 20.0 ],
									"outlettype" : [ "bang" ],
									"id" : "obj-70"
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "event length (sec)",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 470.0, 375.0, 107.0, 20.0 ],
									"id" : "obj-57",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "number",
									"varname" : "amp",
									"numinlets" : 1,
									"numoutlets" : 2,
									"patching_rect" : [ 345.0, 75.0, 50.0, 20.0 ],
									"outlettype" : [ "int", "bang" ],
									"id" : "obj-55",
									"fontname" : "Arial",
									"maximum" : 12,
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "pack_it",
									"numinlets" : 1,
									"frgb" : [ 0.815686, 0.0, 0.0, 1.0 ],
									"textcolor" : [ 0.815686, 0.0, 0.0, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 22.0, 357.0, 52.0, 20.0 ],
									"id" : "obj-50",
									"fontname" : "Arial",
									"fontface" : 2,
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "p amp",
									"numinlets" : 3,
									"numoutlets" : 2,
									"patching_rect" : [ 285.0, 75.0, 56.0, 20.0 ],
									"outlettype" : [ "signal", "signal" ],
									"id" : "obj-49",
									"fontname" : "Arial",
									"fontsize" : 12.0,
									"patcher" : 									{
										"fileversion" : 1,
										"rect" : [ 20.0, 74.0, 350.0, 322.0 ],
										"bglocked" : 0,
										"defrect" : [ 20.0, 74.0, 350.0, 322.0 ],
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
										"boxes" : [ 											{
												"box" : 												{
													"maxclass" : "outlet",
													"numinlets" : 1,
													"numoutlets" : 0,
													"patching_rect" : [ 90.0, 148.0, 21.0, 21.0 ],
													"id" : "obj-1",
													"comment" : ""
												}

											}
, 											{
												"box" : 												{
													"maxclass" : "outlet",
													"numinlets" : 1,
													"numoutlets" : 0,
													"patching_rect" : [ 33.0, 148.0, 21.0, 21.0 ],
													"id" : "obj-2",
													"comment" : ""
												}

											}
, 											{
												"box" : 												{
													"maxclass" : "newobj",
													"text" : "dbtoa",
													"numinlets" : 1,
													"numoutlets" : 1,
													"patching_rect" : [ 129.0, 70.0, 48.0, 20.0 ],
													"outlettype" : [ "" ],
													"id" : "obj-3",
													"fontname" : "Arial",
													"fontsize" : 12.0
												}

											}
, 											{
												"box" : 												{
													"maxclass" : "newobj",
													"text" : "*~ 1.",
													"numinlets" : 2,
													"numoutlets" : 1,
													"patching_rect" : [ 90.0, 105.0, 49.0, 20.0 ],
													"outlettype" : [ "signal" ],
													"id" : "obj-4",
													"fontname" : "Arial",
													"fontsize" : 12.0
												}

											}
, 											{
												"box" : 												{
													"maxclass" : "newobj",
													"text" : "*~ 1.",
													"numinlets" : 2,
													"numoutlets" : 1,
													"patching_rect" : [ 33.0, 105.0, 49.0, 20.0 ],
													"outlettype" : [ "signal" ],
													"id" : "obj-5",
													"fontname" : "Arial",
													"fontsize" : 12.0
												}

											}
, 											{
												"box" : 												{
													"maxclass" : "inlet",
													"numinlets" : 0,
													"numoutlets" : 1,
													"patching_rect" : [ 129.0, 41.0, 21.0, 21.0 ],
													"outlettype" : [ "int" ],
													"id" : "obj-6",
													"comment" : ""
												}

											}
, 											{
												"box" : 												{
													"maxclass" : "inlet",
													"numinlets" : 0,
													"numoutlets" : 1,
													"patching_rect" : [ 90.0, 41.0, 21.0, 21.0 ],
													"outlettype" : [ "signal" ],
													"id" : "obj-7",
													"comment" : ""
												}

											}
, 											{
												"box" : 												{
													"maxclass" : "inlet",
													"numinlets" : 0,
													"numoutlets" : 1,
													"patching_rect" : [ 33.0, 41.0, 21.0, 21.0 ],
													"outlettype" : [ "signal" ],
													"id" : "obj-8",
													"comment" : ""
												}

											}
 ],
										"lines" : [ 											{
												"patchline" : 												{
													"source" : [ "obj-3", 0 ],
													"destination" : [ "obj-4", 1 ],
													"hidden" : 0,
													"midpoints" : [  ]
												}

											}
, 											{
												"patchline" : 												{
													"source" : [ "obj-6", 0 ],
													"destination" : [ "obj-3", 0 ],
													"hidden" : 0,
													"midpoints" : [  ]
												}

											}
, 											{
												"patchline" : 												{
													"source" : [ "obj-4", 0 ],
													"destination" : [ "obj-1", 0 ],
													"hidden" : 0,
													"midpoints" : [  ]
												}

											}
, 											{
												"patchline" : 												{
													"source" : [ "obj-7", 0 ],
													"destination" : [ "obj-4", 0 ],
													"hidden" : 0,
													"midpoints" : [  ]
												}

											}
, 											{
												"patchline" : 												{
													"source" : [ "obj-3", 0 ],
													"destination" : [ "obj-5", 1 ],
													"hidden" : 0,
													"midpoints" : [  ]
												}

											}
, 											{
												"patchline" : 												{
													"source" : [ "obj-5", 0 ],
													"destination" : [ "obj-2", 0 ],
													"hidden" : 0,
													"midpoints" : [  ]
												}

											}
, 											{
												"patchline" : 												{
													"source" : [ "obj-8", 0 ],
													"destination" : [ "obj-5", 0 ],
													"hidden" : 0,
													"midpoints" : [  ]
												}

											}
 ]
									}
,
									"saved_object_attributes" : 									{
										"default_fontname" : "Arial",
										"default_fontsize" : 12.0,
										"fontname" : "Arial",
										"globalpatchername" : "",
										"fontface" : 0,
										"fontsize" : 12.0,
										"default_fontface" : 0
									}

								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "readbuf -1 2 mybuf 2. 16.",
									"numinlets" : 2,
									"bgcolor" : [ 0.921569, 1.0, 0.482353, 1.0 ],
									"numoutlets" : 1,
									"patching_rect" : [ 30.0, 480.0, 761.0, 21.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-40",
									"fontname" : "Arial",
									"fontface" : 1,
									"fontsize" : 14.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "pattrforward csound~",
									"numinlets" : 1,
									"numoutlets" : 1,
									"patching_rect" : [ 30.0, 510.0, 126.0, 20.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-29",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"varname" : "pack_it",
									"text" : "unpack 0 0 s 0. 0.",
									"numinlets" : 1,
									"numoutlets" : 5,
									"patching_rect" : [ 72.0, 358.0, 187.0, 20.0 ],
									"outlettype" : [ "int", "int", "", "float", "float" ],
									"id" : "obj-13",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "pack e i1 0 1.",
									"numinlets" : 4,
									"numoutlets" : 1,
									"patching_rect" : [ 345.0, 405.0, 94.0, 20.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-22",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "pattrforward csound~",
									"numinlets" : 1,
									"numoutlets" : 1,
									"patching_rect" : [ 345.0, 429.0, 126.0, 20.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-24",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "button",
									"numinlets" : 1,
									"numoutlets" : 1,
									"patching_rect" : [ 345.0, 345.0, 45.0, 45.0 ],
									"outlettype" : [ "bang" ],
									"id" : "obj-30",
									"fgcolor" : [ 0.74902, 1.0, 0.0, 1.0 ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "pattrforward pack_it",
									"numinlets" : 1,
									"hidden" : 1,
									"numoutlets" : 1,
									"patching_rect" : [ 45.0, 331.0, 117.0, 20.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-68",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "1 0 mybuf 0. -44100",
									"numinlets" : 2,
									"bgcolor" : [ 0.941176, 0.941176, 0.941176, 1.0 ],
									"numoutlets" : 1,
									"patching_rect" : [ 45.0, 291.0, 121.0, 18.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-63",
									"fontname" : "Arial",
									"fontface" : 1,
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "1 1 mybuf 0. 3.",
									"numinlets" : 2,
									"bgcolor" : [ 0.941176, 0.941176, 0.941176, 1.0 ],
									"numoutlets" : 1,
									"patching_rect" : [ 45.0, 188.0, 92.0, 18.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-61",
									"fontname" : "Arial",
									"fontface" : 1,
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "-1 2 mybuf 2. 16.",
									"numinlets" : 2,
									"bgcolor" : [ 0.941176, 0.941176, 0.941176, 1.0 ],
									"numoutlets" : 1,
									"patching_rect" : [ 45.0, 239.0, 103.0, 18.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-59",
									"fontname" : "Arial",
									"fontface" : 1,
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "1",
									"numinlets" : 1,
									"frgb" : [ 0.176471, 0.301961, 1.0, 1.0 ],
									"textcolor" : [ 0.176471, 0.301961, 1.0, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 256.0, 102.0, 38.0, 48.0 ],
									"id" : "obj-65",
									"fontname" : "Arial",
									"fontface" : 1,
									"fontsize" : 36.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "scope~",
									"numinlets" : 2,
									"bgcolor" : [ 0.0, 0.0, 0.0, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 330.0, 106.0, 124.0, 60.0 ],
									"id" : "obj-53",
									"bufsize" : 64,
									"calccount" : 128
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "number",
									"numinlets" : 1,
									"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
									"numoutlets" : 2,
									"patching_rect" : [ 72.0, 390.0, 38.0, 20.0 ],
									"triscale" : 0.9,
									"outlettype" : [ "int", "bang" ],
									"id" : "obj-3",
									"fontname" : "Arial",
									"fontsize" : 12.0,
									"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "flonum",
									"numinlets" : 1,
									"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
									"numoutlets" : 2,
									"patching_rect" : [ 240.0, 390.0, 40.0, 20.0 ],
									"triscale" : 0.9,
									"outlettype" : [ "float", "bang" ],
									"id" : "obj-17",
									"fontname" : "Arial",
									"fontsize" : 12.0,
									"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "flonum",
									"numinlets" : 1,
									"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
									"numoutlets" : 2,
									"patching_rect" : [ 198.0, 390.0, 40.0, 20.0 ],
									"triscale" : 0.9,
									"outlettype" : [ "float", "bang" ],
									"id" : "obj-19",
									"fontname" : "Arial",
									"fontsize" : 12.0,
									"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "number",
									"numinlets" : 1,
									"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
									"numoutlets" : 2,
									"patching_rect" : [ 114.0, 390.0, 37.0, 20.0 ],
									"triscale" : 0.9,
									"minimum" : 0,
									"outlettype" : [ "int", "bang" ],
									"id" : "obj-23",
									"fontname" : "Arial",
									"fontsize" : 12.0,
									"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "pack readbuf 1 1 mybuf 0. 0.",
									"numinlets" : 6,
									"numoutlets" : 1,
									"patching_rect" : [ 30.0, 420.0, 229.0, 20.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-32",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "stop",
									"numinlets" : 2,
									"bgcolor" : [ 0.941176, 0.941176, 0.941176, 1.0 ],
									"numoutlets" : 1,
									"patching_rect" : [ 285.0, 15.0, 35.0, 18.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-43",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"varname" : "csound~",
									"text" : "csound~ @args readbuf.csd",
									"numinlets" : 2,
									"numoutlets" : 6,
									"patching_rect" : [ 285.0, 45.0, 166.0, 20.0 ],
									"outlettype" : [ "signal", "signal", "list", "int", "bang", "bang" ],
									"id" : "obj-45",
									"fontname" : "Arial",
									"fontsize" : 12.0,
									"saved_object_attributes" : 									{
										"matchsr" : 1,
										"bypass" : 0,
										"overdrive" : 0,
										"output" : 1,
										"input" : 1,
										"interval" : 10,
										"message" : 1,
										"autostart" : 0,
										"args" : "readbuf.csd"
									}

								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "csound~",
									"numinlets" : 1,
									"frgb" : [ 0.815686, 0.0, 0.0, 1.0 ],
									"textcolor" : [ 0.815686, 0.0, 0.0, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 228.0, 45.0, 58.0, 20.0 ],
									"id" : "obj-46",
									"fontname" : "Arial",
									"fontface" : 2,
									"fontsize" : 12.0
								}

							}
 ],
						"lines" : [ 							{
								"patchline" : 								{
									"source" : [ "obj-48", 0 ],
									"destination" : [ "obj-11", 1 ],
									"hidden" : 0,
									"midpoints" : [ 474.5, 97.5, 534.0, 97.5 ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-2", 0 ],
									"destination" : [ "obj-40", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-40", 0 ],
									"destination" : [ "obj-29", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-13", 0 ],
									"destination" : [ "obj-70", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-61", 0 ],
									"destination" : [ "obj-68", 0 ],
									"hidden" : 1,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-55", 0 ],
									"destination" : [ "obj-49", 2 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-22", 0 ],
									"destination" : [ "obj-24", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-63", 0 ],
									"destination" : [ "obj-68", 0 ],
									"hidden" : 1,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-59", 0 ],
									"destination" : [ "obj-68", 0 ],
									"hidden" : 1,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-13", 4 ],
									"destination" : [ "obj-17", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-13", 3 ],
									"destination" : [ "obj-19", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-13", 1 ],
									"destination" : [ "obj-23", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-13", 0 ],
									"destination" : [ "obj-3", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-30", 0 ],
									"destination" : [ "obj-22", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-11", 0 ],
									"destination" : [ "obj-31", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-9", 0 ],
									"destination" : [ "obj-11", 1 ],
									"hidden" : 0,
									"midpoints" : [ 676.5, 97.0, 534.0, 97.0 ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-9", 0 ],
									"destination" : [ "obj-8", 0 ],
									"hidden" : 0,
									"midpoints" : [ 676.5, 97.0, 669.5, 97.0 ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-26", 0 ],
									"destination" : [ "obj-9", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-70", 0 ],
									"destination" : [ "obj-32", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-13", 2 ],
									"destination" : [ "obj-32", 3 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-17", 0 ],
									"destination" : [ "obj-32", 5 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-19", 0 ],
									"destination" : [ "obj-32", 4 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-23", 0 ],
									"destination" : [ "obj-32", 2 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-3", 0 ],
									"destination" : [ "obj-32", 1 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-32", 0 ],
									"destination" : [ "obj-2", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-8", 0 ],
									"destination" : [ "obj-14", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-49", 1 ],
									"destination" : [ "obj-118", 1 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-49", 0 ],
									"destination" : [ "obj-118", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-79", 0 ],
									"destination" : [ "obj-22", 3 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-41", 0 ],
									"destination" : [ "obj-79", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-27", 0 ],
									"destination" : [ "obj-45", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-45", 1 ],
									"destination" : [ "obj-49", 1 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-43", 0 ],
									"destination" : [ "obj-45", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-45", 0 ],
									"destination" : [ "obj-49", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-45", 0 ],
									"destination" : [ "obj-53", 0 ],
									"hidden" : 1,
									"midpoints" : [  ]
								}

							}
 ]
					}
,
					"saved_object_attributes" : 					{
						"default_fontname" : "Arial",
						"default_fontsize" : 12.0,
						"fontname" : "Arial",
						"globalpatchername" : "",
						"fontface" : 0,
						"fontsize" : 12.0,
						"default_fontface" : 0
					}

				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "p \"rsidx wsidx messages\"",
					"numinlets" : 0,
					"numoutlets" : 0,
					"patching_rect" : [ 790.0, 212.0, 149.0, 20.0 ],
					"id" : "obj-4",
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"patcher" : 					{
						"fileversion" : 1,
						"rect" : [ 466.0, 340.0, 757.0, 380.0 ],
						"bglocked" : 0,
						"defrect" : [ 466.0, 340.0, 757.0, 380.0 ],
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
									"maxclass" : "comment",
									"text" : "csound~",
									"numinlets" : 1,
									"frgb" : [ 0.815686, 0.0, 0.0, 1.0 ],
									"textcolor" : [ 0.815686, 0.0, 0.0, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 121.0, 147.0, 63.0, 20.0 ],
									"id" : "obj-44",
									"fontname" : "Arial",
									"fontface" : 2,
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "pattrforward csound~",
									"numinlets" : 1,
									"numoutlets" : 1,
									"patching_rect" : [ 260.0, 329.0, 126.0, 20.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-43",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "pattrforward csound~",
									"numinlets" : 1,
									"numoutlets" : 1,
									"patching_rect" : [ 255.0, 177.0, 126.0, 20.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-42",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "pattrforward csound~",
									"numinlets" : 1,
									"numoutlets" : 1,
									"patching_rect" : [ 405.0, 90.0, 126.0, 20.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-1",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "button",
									"numinlets" : 1,
									"numoutlets" : 1,
									"patching_rect" : [ 405.0, 15.0, 33.0, 33.0 ],
									"outlettype" : [ "bang" ],
									"id" : "obj-2"
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "wsidx 1 3 8.3333",
									"numinlets" : 2,
									"numoutlets" : 1,
									"patching_rect" : [ 638.0, 60.0, 102.0, 18.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-12",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "wsidx 1 2 4.",
									"numinlets" : 2,
									"numoutlets" : 1,
									"patching_rect" : [ 563.0, 60.0, 74.0, 18.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-11",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "wsidx 1 1 2",
									"numinlets" : 2,
									"numoutlets" : 1,
									"patching_rect" : [ 488.0, 60.0, 71.0, 18.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-10",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "wsidx 1 0 42.",
									"numinlets" : 2,
									"numoutlets" : 1,
									"patching_rect" : [ 405.0, 60.0, 81.0, 18.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-8",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "3",
									"numinlets" : 1,
									"frgb" : [ 0.176471, 0.301961, 1.0, 1.0 ],
									"textcolor" : [ 0.176471, 0.301961, 1.0, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 285.0, 225.0, 38.0, 48.0 ],
									"id" : "obj-4",
									"fontname" : "Arial",
									"fontface" : 1,
									"fontsize" : 36.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "2",
									"numinlets" : 1,
									"frgb" : [ 0.176471, 0.301961, 1.0, 1.0 ],
									"textcolor" : [ 0.176471, 0.301961, 1.0, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 370.0, 10.0, 38.0, 48.0 ],
									"id" : "obj-9",
									"fontname" : "Arial",
									"fontface" : 1,
									"fontsize" : 36.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "1",
									"numinlets" : 1,
									"frgb" : [ 0.176471, 0.301961, 1.0, 1.0 ],
									"textcolor" : [ 0.176471, 0.301961, 1.0, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 15.0, 15.0, 38.0, 48.0 ],
									"id" : "obj-54",
									"fontname" : "Arial",
									"fontface" : 1,
									"fontsize" : 36.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "Click this button to start [csound~].",
									"linecount" : 2,
									"numinlets" : 1,
									"frgb" : [ 0.337255, 0.254902, 0.113725, 1.0 ],
									"textcolor" : [ 0.337255, 0.254902, 0.113725, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 147.0, 43.0, 130.0, 39.0 ],
									"id" : "obj-52",
									"fontname" : "Arial",
									"fontsize" : 14.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "button",
									"numinlets" : 1,
									"numoutlets" : 1,
									"patching_rect" : [ 60.0, 15.0, 87.0, 87.0 ],
									"outlettype" : [ "bang" ],
									"id" : "obj-41",
									"fgcolor" : [ 0.843137, 1.0, 0.372549, 1.0 ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "button",
									"numinlets" : 1,
									"numoutlets" : 1,
									"patching_rect" : [ 21.0, 309.0, 33.0, 33.0 ],
									"outlettype" : [ "bang" ],
									"id" : "obj-14",
									"blinkcolor" : [ 1.0, 0.090196, 0.090196, 1.0 ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "rsidx 1 3 8.3333",
									"numinlets" : 2,
									"bgcolor" : [ 0.984314, 1.0, 0.662745, 1.0 ],
									"numoutlets" : 1,
									"patching_rect" : [ 60.0, 309.0, 179.0, 32.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-16",
									"fontname" : "Arial",
									"fontsize" : 24.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "stop",
									"numinlets" : 2,
									"numoutlets" : 1,
									"patching_rect" : [ 15.0, 75.0, 35.0, 18.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-17",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "When csound~ receives an rsidx message, it responds with an rsidx message with the same format as wsidx.",
									"linecount" : 4,
									"numinlets" : 1,
									"frgb" : [ 0.219608, 0.176471, 0.235294, 1.0 ],
									"textcolor" : [ 0.219608, 0.176471, 0.235294, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 75.0, 210.0, 166.0, 62.0 ],
									"id" : "obj-18",
									"fontname" : "Arial",
									"fontface" : 2,
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "Arg #1: table #\nArg #2: table index",
									"linecount" : 2,
									"numinlets" : 1,
									"frgb" : [ 0.219608, 0.176471, 0.235294, 1.0 ],
									"textcolor" : [ 0.219608, 0.176471, 0.235294, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 274.0, 293.0, 115.0, 34.0 ],
									"id" : "obj-20",
									"fontname" : "Arial",
									"fontface" : 2,
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "Arg #1: table #\nArg #2: table index\nArg #3: value",
									"linecount" : 3,
									"numinlets" : 1,
									"frgb" : [ 0.219608, 0.176471, 0.235294, 1.0 ],
									"textcolor" : [ 0.219608, 0.176471, 0.235294, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 269.0, 126.0, 121.0, 48.0 ],
									"id" : "obj-25",
									"fontname" : "Arial",
									"fontface" : 2,
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "value",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 350.0, 55.0, 40.0, 20.0 ],
									"id" : "obj-26",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "index",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 307.0, 55.0, 41.0, 20.0 ],
									"id" : "obj-27",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "prepend set",
									"numinlets" : 1,
									"numoutlets" : 1,
									"patching_rect" : [ 60.0, 285.0, 80.0, 20.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-28",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "pak rsidx 1 0",
									"numinlets" : 3,
									"numoutlets" : 1,
									"patching_rect" : [ 260.0, 272.0, 90.0, 20.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-29",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "flonum",
									"numinlets" : 1,
									"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
									"numoutlets" : 2,
									"patching_rect" : [ 350.0, 75.0, 40.0, 20.0 ],
									"triscale" : 0.9,
									"minimum" : 0.0,
									"outlettype" : [ "float", "bang" ],
									"id" : "obj-30",
									"fontname" : "Arial",
									"maximum" : 127.0,
									"fontsize" : 12.0,
									"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "pak wsidx 1 0 0.",
									"numinlets" : 4,
									"numoutlets" : 1,
									"patching_rect" : [ 255.0, 105.0, 97.0, 20.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-31",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "number",
									"numinlets" : 1,
									"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
									"numoutlets" : 2,
									"patching_rect" : [ 307.0, 75.0, 37.0, 20.0 ],
									"triscale" : 0.9,
									"minimum" : 0,
									"outlettype" : [ "int", "bang" ],
									"id" : "obj-33",
									"fontname" : "Arial",
									"maximum" : 15,
									"fontsize" : 12.0,
									"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "number",
									"numinlets" : 1,
									"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
									"numoutlets" : 2,
									"patching_rect" : [ 331.0, 243.0, 37.0, 20.0 ],
									"triscale" : 0.9,
									"minimum" : 0,
									"outlettype" : [ "int", "bang" ],
									"id" : "obj-34",
									"fontname" : "Arial",
									"fontsize" : 12.0,
									"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"varname" : "csound~",
									"text" : "csound~ @args rsidx_wsidx.csd",
									"numinlets" : 2,
									"numoutlets" : 6,
									"patching_rect" : [ 60.0, 165.0, 188.0, 20.0 ],
									"outlettype" : [ "signal", "signal", "list", "int", "bang", "bang" ],
									"id" : "obj-35",
									"fontname" : "Arial",
									"fontsize" : 12.0,
									"saved_object_attributes" : 									{
										"matchsr" : 1,
										"bypass" : 0,
										"overdrive" : 0,
										"output" : 1,
										"input" : 1,
										"interval" : 10,
										"message" : 1,
										"autostart" : 0,
										"args" : "rsidx_wsidx.csd"
									}

								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "index to read",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 313.0, 223.0, 83.0, 20.0 ],
									"id" : "obj-36",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "3) Read some table values.",
									"numinlets" : 1,
									"frgb" : [ 0.0, 0.14902, 0.964706, 1.0 ],
									"textcolor" : [ 0.0, 0.14902, 0.964706, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 450.0, 350.0, 200.0, 23.0 ],
									"id" : "obj-37",
									"fontname" : "Arial",
									"fontsize" : 14.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "2) Change some table values.",
									"numinlets" : 1,
									"frgb" : [ 0.0, 0.14902, 0.964706, 1.0 ],
									"textcolor" : [ 0.0, 0.14902, 0.964706, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 450.0, 325.0, 200.0, 23.0 ],
									"id" : "obj-32",
									"fontname" : "Arial",
									"fontsize" : 14.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "1) Start [csound~].",
									"numinlets" : 1,
									"frgb" : [ 0.0, 0.14902, 0.964706, 1.0 ],
									"textcolor" : [ 0.0, 0.14902, 0.964706, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 450.0, 300.0, 139.0, 23.0 ],
									"id" : "obj-15",
									"fontname" : "Arial",
									"fontsize" : 14.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "All indexes are zero-based.  In other words, the first index is 0.  The maximum index for a table of size x is x-1.",
									"linecount" : 3,
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 458.0, 237.0, 234.0, 48.0 ],
									"id" : "obj-13",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "rsidx / wsidx",
									"numinlets" : 1,
									"frgb" : [ 0.337255, 0.254902, 0.113725, 1.0 ],
									"textcolor" : [ 0.337255, 0.254902, 0.113725, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 458.0, 145.0, 212.0, 48.0 ],
									"id" : "obj-40",
									"fontname" : "Arial",
									"fontsize" : 36.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "Use wsidx to write Csound table values.",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 458.0, 216.0, 238.0, 20.0 ],
									"id" : "obj-21",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "Use rsidx to read Csound table values.",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 458.0, 195.0, 232.0, 20.0 ],
									"id" : "obj-22",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "panel",
									"numinlets" : 1,
									"bgcolor" : [ 0.952941, 0.933333, 0.917647, 1.0 ],
									"numoutlets" : 0,
									"border" : 1,
									"patching_rect" : [ 450.0, 135.0, 249.0, 158.0 ],
									"id" : "obj-106",
									"rounded" : 0
								}

							}
 ],
						"lines" : [ 							{
								"patchline" : 								{
									"source" : [ "obj-35", 2 ],
									"destination" : [ "obj-28", 0 ],
									"hidden" : 0,
									"midpoints" : [ 137.100006, 197.0, 69.5, 197.0 ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-29", 0 ],
									"destination" : [ "obj-43", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-31", 0 ],
									"destination" : [ "obj-42", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-30", 0 ],
									"destination" : [ "obj-31", 3 ],
									"hidden" : 0,
									"midpoints" : [ 359.5, 98.0, 342.5, 98.0 ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-33", 0 ],
									"destination" : [ "obj-31", 2 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-41", 0 ],
									"destination" : [ "obj-35", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-17", 0 ],
									"destination" : [ "obj-35", 0 ],
									"hidden" : 1,
									"midpoints" : [ 24.5, 155.0, 69.5, 155.0 ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-34", 0 ],
									"destination" : [ "obj-29", 2 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-28", 0 ],
									"destination" : [ "obj-16", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-28", 0 ],
									"destination" : [ "obj-14", 0 ],
									"hidden" : 0,
									"midpoints" : [ 69.5, 305.0, 30.5, 305.0 ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-2", 0 ],
									"destination" : [ "obj-8", 0 ],
									"hidden" : 0,
									"midpoints" : [ 414.5, 53.5, 414.5, 53.5 ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-2", 0 ],
									"destination" : [ "obj-10", 0 ],
									"hidden" : 0,
									"midpoints" : [ 414.5, 53.5, 497.5, 53.5 ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-2", 0 ],
									"destination" : [ "obj-11", 0 ],
									"hidden" : 0,
									"midpoints" : [ 414.5, 53.5, 572.5, 53.5 ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-2", 0 ],
									"destination" : [ "obj-12", 0 ],
									"hidden" : 0,
									"midpoints" : [ 414.5, 53.5, 647.5, 53.5 ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-8", 0 ],
									"destination" : [ "obj-1", 0 ],
									"hidden" : 0,
									"midpoints" : [ 414.5, 83.5, 414.5, 83.5 ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-10", 0 ],
									"destination" : [ "obj-1", 0 ],
									"hidden" : 0,
									"midpoints" : [ 497.5, 83.5, 414.5, 83.5 ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-11", 0 ],
									"destination" : [ "obj-1", 0 ],
									"hidden" : 0,
									"midpoints" : [ 572.5, 83.5, 414.5, 83.5 ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-12", 0 ],
									"destination" : [ "obj-1", 0 ],
									"hidden" : 0,
									"midpoints" : [ 647.5, 83.5, 414.5, 83.5 ]
								}

							}
 ]
					}
,
					"saved_object_attributes" : 					{
						"default_fontname" : "Arial",
						"default_fontsize" : 12.0,
						"fontname" : "Arial",
						"globalpatchername" : "",
						"fontface" : 0,
						"fontsize" : 12.0,
						"default_fontface" : 0
					}

				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "p \"invalue outvalue chnget chnset\"",
					"numinlets" : 0,
					"numoutlets" : 0,
					"patching_rect" : [ 790.0, 170.0, 194.0, 20.0 ],
					"id" : "obj-8",
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"patcher" : 					{
						"fileversion" : 1,
						"rect" : [ 689.0, 208.0, 490.0, 529.0 ],
						"bglocked" : 0,
						"defrect" : [ 689.0, 208.0, 490.0, 529.0 ],
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
									"maxclass" : "comment",
									"text" : "Lines 6 and 7 are not required because \"frq\" and \"reso\" are input channels, but they don't hurt.",
									"linecount" : 3,
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 201.0, 299.0, 196.0, 48.0 ],
									"id" : "obj-1",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "Line 18 will always work because the invalue/outvalue opcodes don't use channels.",
									"linecount" : 2,
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 220.0, 465.0, 266.0, 34.0 ],
									"id" : "obj-2",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "If line 08 is left out, [csound~] will not output value pairs for channel \"frq2\".",
									"linecount" : 2,
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 201.0, 345.0, 215.0, 34.0 ],
									"id" : "obj-3",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "01  sr = 44100\r02  kr = 4410 \r03  ksmps = 10 \r04  nchnls = 2 \r05 \r06  chn_k \"frq\", 1\r07  chn_k \"reso\", 1\r08  chn_k \"frq2\", 2\r09   \r10  instr 1\r11      kFrq   chnget \"frq\"\r12      kReso chnget \"reso\"\r13      kTime invalue \"time\"\r14      \r15      kFrq2 = kFrq * 2\r16      kTime2 = kTime * 2\r17\r18      outvalue \"time2\", kTime2\r19      chnset kFrq, \"frq2\"\r20  endin",
									"linecount" : 20,
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 30.0, 244.0, 209.0, 282.0 ],
									"id" : "obj-4",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "outvalue \"filt\", kFilt\n\rchnset kReso, \"reso\"",
									"linecount" : 3,
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 30.0, 114.0, 136.0, 48.0 ],
									"id" : "obj-5",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "kFilt invalue \"filt\"\n\rkReso chnget \"reso\" ",
									"linecount" : 3,
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 30.0, 36.0, 136.0, 48.0 ],
									"id" : "obj-7",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "I recommend using chnget/chnset instead of invalue/outvalue.",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 6.0, 170.0, 452.0, 20.0 ],
									"id" : "obj-8",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "To output values from [csound~], try something like:",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 6.0, 89.0, 347.0, 20.0 ],
									"id" : "obj-9",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "IMPORTANT:  In order to use the chnset opcode, you must declare all output channels in the header section of your orchestra with the chn_k opcode. The 2nd argument of chn_k can be 1 (input), 2 (output), or 3 (input + output).  For example:",
									"linecount" : 3,
									"numinlets" : 1,
									"frgb" : [ 0.65098, 0.0, 0.0, 1.0 ],
									"textcolor" : [ 0.65098, 0.0, 0.0, 1.0 ],
									"numoutlets" : 0,
									"patching_rect" : [ 5.0, 189.0, 454.0, 48.0 ],
									"id" : "obj-10",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "To access values sent to [csound~] via \"control\" messages, try something like:",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 6.0, 9.0, 472.0, 20.0 ],
									"id" : "obj-11",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
 ],
						"lines" : [  ]
					}
,
					"saved_object_attributes" : 					{
						"default_fontname" : "Arial",
						"default_fontsize" : 12.0,
						"fontname" : "Arial",
						"globalpatchername" : "",
						"fontface" : 0,
						"fontsize" : 12.0,
						"default_fontface" : 0
					}

				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "p \"csound message\"",
					"numinlets" : 0,
					"numoutlets" : 0,
					"patching_rect" : [ 790.0, 149.0, 123.0, 20.0 ],
					"id" : "obj-9",
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"patcher" : 					{
						"fileversion" : 1,
						"rect" : [ 678.0, 304.0, 462.0, 436.0 ],
						"bglocked" : 0,
						"defrect" : [ 678.0, 304.0, 462.0, 436.0 ],
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
									"maxclass" : "comment",
									"text" : "You can send the csound command arguments using a \"csound\" message or by using the @args attribute.",
									"linecount" : 2,
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 15.0, 0.0, 426.0, 34.0 ],
									"id" : "obj-17",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "Macintosh HD:/Users/George/file.csd",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 44.0, 401.0, 226.0, 20.0 ],
									"id" : "obj-16",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "A Max style path:",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 15.0, 381.0, 226.0, 20.0 ],
									"id" : "obj-15",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "/Users/George/file.csd",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 44.0, 361.0, 226.0, 20.0 ],
									"id" : "obj-14",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "On All Platforms, use forward slashes (/).",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 15.0, 301.0, 339.0, 20.0 ],
									"id" : "obj-13",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "On Windows, use Max style paths (e.g. C:/directory/file.csd).",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 15.0, 321.0, 339.0, 20.0 ],
									"id" : "obj-12",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "csound -o dog.aif doggystyle.csd",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 32.0, 206.0, 189.0, 20.0 ],
									"id" : "obj-1",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "csound -m0 badkitty.csd",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 32.0, 188.0, 160.0, 20.0 ],
									"id" : "obj-2",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "On Mac OSX, use Max style or POSIX style paths.  A POSIX path:",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 15.0, 341.0, 366.0, 20.0 ],
									"id" : "obj-3",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "csound hello.csd",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 32.0, 116.0, 148.0, 20.0 ],
									"id" : "obj-4",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "csound /Volumes/projects/acid.csd",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 32.0, 170.0, 203.0, 20.0 ],
									"id" : "obj-5",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "csound projects/nokittythatsmypotpie.csd",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 32.0, 152.0, 235.0, 20.0 ],
									"id" : "obj-6",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "If your patch is within Max's search path, the current directory may not be the same as the directory containing your patch. In this case, simply specify the csd/orc/sco file by name (i.e. no slashes), or use an absolute path name.",
									"linecount" : 3,
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 15.0, 238.0, 426.0, 48.0 ],
									"id" : "obj-7",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "If your csd/orc/sco files are in the same directory as this patch, there's no need to use absolute pathnames. You can specify the file using a relative pathname instead.",
									"linecount" : 3,
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 15.0, 45.0, 429.0, 48.0 ],
									"id" : "obj-8",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "csound fun.orc fun.sco",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 32.0, 134.0, 150.0, 20.0 ],
									"id" : "obj-9",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "panel",
									"numinlets" : 1,
									"bgcolor" : [ 0.901961, 0.901961, 0.901961, 1.0 ],
									"numoutlets" : 0,
									"border" : 2,
									"bordercolor" : [ 1.0, 1.0, 1.0, 1.0 ],
									"patching_rect" : [ 30.0, 112.0, 248.0, 119.0 ],
									"id" : "obj-10",
									"rounded" : 0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "Examples:",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 30.0, 95.0, 128.0, 20.0 ],
									"id" : "obj-11",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
 ],
						"lines" : [  ]
					}
,
					"saved_object_attributes" : 					{
						"default_fontname" : "Arial",
						"default_fontsize" : 12.0,
						"fontname" : "Arial",
						"globalpatchername" : "",
						"fontface" : 0,
						"fontsize" : 12.0,
						"default_fontface" : 0
					}

				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "Written by Davis Pyon. Inspired by Matt Ingalls' original csound~ and Victor Lazzarini's csoundapi~.",
					"linecount" : 3,
					"numinlets" : 1,
					"frgb" : [ 0.45098, 0.447059, 0.517647, 1.0 ],
					"textcolor" : [ 0.45098, 0.447059, 0.517647, 1.0 ],
					"numoutlets" : 0,
					"patching_rect" : [ 791.0, 0.0, 209.0, 48.0 ],
					"id" : "obj-13",
					"fontname" : "Arial",
					"fontface" : 3,
					"fontsize" : 12.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "c portTime $1",
					"numinlets" : 2,
					"bgcolor" : [ 1.0, 0.992157, 0.94902, 1.0 ],
					"numoutlets" : 1,
					"patching_rect" : [ 210.0, 184.0, 86.0, 18.0 ],
					"outlettype" : [ "" ],
					"id" : "obj-15",
					"fontname" : "Arial",
					"fontsize" : 12.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "flonum",
					"numinlets" : 1,
					"bgcolor" : [ 0.909804, 0.909804, 0.909804, 1.0 ],
					"numoutlets" : 2,
					"patching_rect" : [ 210.0, 163.0, 51.0, 20.0 ],
					"triscale" : 0.9,
					"minimum" : 0.0,
					"outlettype" : [ "float", "bang" ],
					"id" : "obj-16",
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "flonum",
					"numinlets" : 1,
					"bgcolor" : [ 0.909804, 0.909804, 0.909804, 1.0 ],
					"numoutlets" : 2,
					"patching_rect" : [ 149.0, 283.0, 47.0, 20.0 ],
					"triscale" : 0.9,
					"outlettype" : [ "float", "bang" ],
					"id" : "obj-17",
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "Output Control Messages",
					"numinlets" : 1,
					"frgb" : [ 0.301961, 0.294118, 0.376471, 1.0 ],
					"textcolor" : [ 0.301961, 0.294118, 0.376471, 1.0 ],
					"numoutlets" : 0,
					"patching_rect" : [ 15.0, 233.0, 189.0, 23.0 ],
					"id" : "obj-18",
					"fontname" : "Arial",
					"fontface" : 3,
					"fontsize" : 14.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "Input Control Messages",
					"numinlets" : 1,
					"frgb" : [ 0.301961, 0.294118, 0.376471, 1.0 ],
					"textcolor" : [ 0.301961, 0.294118, 0.376471, 1.0 ],
					"numoutlets" : 0,
					"patching_rect" : [ 15.0, 139.0, 177.0, 23.0 ],
					"id" : "obj-19",
					"fontname" : "Arial",
					"fontface" : 3,
					"fontsize" : 14.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "e i3 0 8 6.08 32768 2 4 4 6 5 1",
					"numinlets" : 2,
					"bgcolor" : [ 1.0, 0.992157, 0.94902, 1.0 ],
					"numoutlets" : 1,
					"patching_rect" : [ 15.0, 97.0, 179.0, 18.0 ],
					"outlettype" : [ "" ],
					"id" : "obj-36",
					"fontname" : "Arial",
					"fontsize" : 12.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "overdrive $1",
					"numinlets" : 2,
					"bgcolor" : [ 1.0, 0.992157, 0.94902, 1.0 ],
					"numoutlets" : 1,
					"patching_rect" : [ 398.0, 250.0, 78.0, 18.0 ],
					"outlettype" : [ "" ],
					"id" : "obj-37",
					"fontname" : "Arial",
					"fontsize" : 12.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "toggle",
					"numinlets" : 1,
					"numoutlets" : 1,
					"patching_rect" : [ 382.0, 250.0, 15.0, 15.0 ],
					"outlettype" : [ "int" ],
					"id" : "obj-38"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "Output overdrive (disabled by default). When disabled, outvalue pairs are output at 10ms intervals. When enabled, outvalue pairs are output immediately.",
					"linecount" : 3,
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 477.0, 249.0, 308.0, 48.0 ],
					"id" : "obj-39",
					"fontname" : "Arial",
					"fontsize" : 12.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "message $1",
					"numinlets" : 2,
					"bgcolor" : [ 1.0, 0.992157, 0.94902, 1.0 ],
					"numoutlets" : 1,
					"patching_rect" : [ 398.0, 228.0, 78.0, 18.0 ],
					"outlettype" : [ "" ],
					"id" : "obj-40",
					"fontname" : "Arial",
					"fontsize" : 12.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "toggle",
					"numinlets" : 1,
					"numoutlets" : 1,
					"patching_rect" : [ 382.0, 228.0, 15.0, 15.0 ],
					"outlettype" : [ "int" ],
					"id" : "obj-41"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "Message output to Max window (enabled by default).",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 476.0, 228.0, 295.0, 20.0 ],
					"id" : "obj-42",
					"fontname" : "Arial",
					"fontsize" : 12.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "output $1",
					"numinlets" : 2,
					"bgcolor" : [ 1.0, 0.992157, 0.94902, 1.0 ],
					"numoutlets" : 1,
					"patching_rect" : [ 398.0, 205.0, 71.0, 18.0 ],
					"outlettype" : [ "" ],
					"id" : "obj-43",
					"fontname" : "Arial",
					"fontsize" : 12.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "toggle",
					"numinlets" : 1,
					"numoutlets" : 1,
					"patching_rect" : [ 382.0, 205.0, 15.0, 15.0 ],
					"outlettype" : [ "int" ],
					"id" : "obj-44"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "Output processing (enabled by default).",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 469.0, 205.0, 241.0, 20.0 ],
					"id" : "obj-45",
					"fontname" : "Arial",
					"fontsize" : 12.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "input $1",
					"numinlets" : 2,
					"bgcolor" : [ 1.0, 0.992157, 0.94902, 1.0 ],
					"numoutlets" : 1,
					"patching_rect" : [ 398.0, 183.0, 61.0, 18.0 ],
					"outlettype" : [ "" ],
					"id" : "obj-46",
					"fontname" : "Arial",
					"fontsize" : 12.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "toggle",
					"numinlets" : 1,
					"numoutlets" : 1,
					"patching_rect" : [ 382.0, 183.0, 15.0, 15.0 ],
					"outlettype" : [ "int" ],
					"id" : "obj-47"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "p note",
					"numinlets" : 2,
					"hidden" : 1,
					"numoutlets" : 1,
					"patching_rect" : [ 15.0, 390.0, 46.0, 20.0 ],
					"outlettype" : [ "" ],
					"id" : "obj-48",
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"patcher" : 					{
						"fileversion" : 1,
						"rect" : [ 334.0, 523.0, 199.0, 216.0 ],
						"bglocked" : 0,
						"defrect" : [ 334.0, 523.0, 199.0, 216.0 ],
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
									"numinlets" : 0,
									"numoutlets" : 1,
									"patching_rect" : [ 96.0, 70.0, 15.0, 15.0 ],
									"outlettype" : [ "int" ],
									"id" : "obj-1",
									"comment" : ""
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "inlet",
									"numinlets" : 0,
									"numoutlets" : 1,
									"patching_rect" : [ 57.0, 70.0, 15.0, 15.0 ],
									"outlettype" : [ "int" ],
									"id" : "obj-2",
									"comment" : ""
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "pack 0 0",
									"numinlets" : 2,
									"numoutlets" : 1,
									"patching_rect" : [ 57.0, 115.0, 48.0, 17.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-3",
									"fontname" : "Arial",
									"fontsize" : 9.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "makenote 60 500",
									"numinlets" : 3,
									"numoutlets" : 2,
									"patching_rect" : [ 57.0, 96.0, 88.0, 17.0 ],
									"outlettype" : [ "float", "float" ],
									"id" : "obj-4",
									"fontname" : "Arial",
									"fontsize" : 9.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "outlet",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 57.0, 135.0, 15.0, 15.0 ],
									"id" : "obj-5",
									"comment" : ""
								}

							}
 ],
						"lines" : [ 							{
								"patchline" : 								{
									"source" : [ "obj-1", 0 ],
									"destination" : [ "obj-4", 1 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-4", 1 ],
									"destination" : [ "obj-3", 1 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-3", 0 ],
									"destination" : [ "obj-5", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-4", 0 ],
									"destination" : [ "obj-3", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-2", 0 ],
									"destination" : [ "obj-4", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
 ]
					}
,
					"saved_object_attributes" : 					{
						"default_fontname" : "Arial",
						"default_fontsize" : 12.0,
						"fontname" : "Arial",
						"globalpatchername" : "",
						"fontface" : 0,
						"fontsize" : 12.0,
						"default_fontface" : 0
					}

				}

			}
, 			{
				"box" : 				{
					"maxclass" : "button",
					"numinlets" : 1,
					"numoutlets" : 1,
					"patching_rect" : [ 363.0, 76.0, 17.0, 17.0 ],
					"outlettype" : [ "bang" ],
					"id" : "obj-50",
					"fgcolor" : [ 1.0, 1.0, 0.003922, 1.0 ],
					"blinkcolor" : [ 1.0, 0.090196, 0.090196, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "kslider",
					"numinlets" : 2,
					"presentation_rect" : [ 0.0, 0.0, 312.0, 53.0 ],
					"numoutlets" : 2,
					"patching_rect" : [ 15.0, 330.0, 312.0, 53.0 ],
					"outlettype" : [ "int", "int" ],
					"id" : "obj-51",
					"hkeycolor" : [ 0.501961, 0.501961, 0.501961, 1.0 ],
					"offset" : 24,
					"range" : 44
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "e i3 0 8 8.01 32768 1 1.1 2 0.6 1 20",
					"numinlets" : 2,
					"bgcolor" : [ 1.0, 0.992157, 0.94902, 1.0 ],
					"numoutlets" : 1,
					"patching_rect" : [ 15.0, 75.0, 202.0, 18.0 ],
					"outlettype" : [ "" ],
					"id" : "obj-52",
					"fontname" : "Arial",
					"fontsize" : 12.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "preset",
					"numinlets" : 1,
					"numoutlets" : 4,
					"patching_rect" : [ 270.0, 75.0, 79.0, 22.0 ],
					"margin" : 4,
					"outlettype" : [ "preset", "int", "preset", "int" ],
					"bubblesize" : 12,
					"id" : "obj-53",
					"spacing" : 2,
					"preset_data" : [ 						{
							"number" : 1,
							"data" : [ 5, "obj-89", "flonum", "float", 2.0, 5, "obj-88", "flonum", "float", 3.0, 5, "obj-86", "flonum", "float", 2.0, 5, "obj-85", "flonum", "float", 4.0, 5, "obj-84", "flonum", "float", 3.0, 5, "obj-78", "flonum", "float", 0.05, 5, "obj-70", "flonum", "float", 290.0, 5, "obj-66", "dial", "float", 0.0, 5, "obj-65", "dial", "float", 29.0, 5, "obj-64", "dial", "float", 44.0, 5, "obj-63", "dial", "float", 102.0, 5, "obj-51", "kslider", "int", 57, 5, "obj-47", "toggle", "int", 1, 5, "obj-44", "toggle", "int", 1, 5, "obj-41", "toggle", "int", 1, 5, "obj-38", "toggle", "int", 0, 5, "obj-17", "flonum", "float", 0.05, 5, "obj-16", "flonum", "float", 0.1, 5, "obj-23", "multislider", "list", 22.0, 5, "obj-14", "number", "int", 0, 5, "obj-35", "toggle", "int", 0, 5, "obj-79", "number", "int", 22 ]
						}
, 						{
							"number" : 2,
							"data" : [ 6, "<invalid>", "gain~", "list", 123, 10.0, 6, "<invalid>", "gain~", "list", 123, 10.0, 5, "obj-89", "flonum", "float", 3.75, 5, "obj-88", "flonum", "float", 0.02, 5, "obj-86", "flonum", "float", 3.75, 5, "obj-85", "flonum", "float", 7.5, 5, "obj-84", "flonum", "float", 0.02, 5, "obj-78", "flonum", "float", 25.0, 5, "obj-70", "flonum", "float", 500.0, 5, "obj-66", "dial", "int", 13, 5, "obj-65", "dial", "int", 14, 5, "obj-64", "dial", "int", 14, 5, "obj-63", "dial", "int", 0, 6, "<invalid>", "gain~", "list", 107, 10.0, 6, "<invalid>", "gain~", "list", 107, 10.0, 5, "obj-47", "toggle", "int", 1, 5, "obj-44", "toggle", "int", 1, 5, "obj-41", "toggle", "int", 1, 5, "obj-38", "toggle", "int", 0, 5, "obj-17", "flonum", "float", 25.0, 5, "obj-16", "flonum", "float", 0.1 ]
						}
, 						{
							"number" : 3,
							"data" : [ 6, "<invalid>", "gain~", "list", 126, 10.0, 6, "<invalid>", "gain~", "list", 126, 10.0, 5, "obj-89", "flonum", "float", 4.52, 5, "obj-88", "flonum", "float", 5.2, 5, "obj-86", "flonum", "float", 4.52, 5, "obj-85", "flonum", "float", 9.039584, 5, "obj-84", "flonum", "float", 5.2, 5, "obj-78", "flonum", "float", 0.16, 5, "obj-70", "flonum", "float", 250.0, 5, "obj-66", "dial", "int", 1, 5, "obj-65", "dial", "int", 19, 5, "obj-64", "dial", "int", 39, 5, "obj-63", "dial", "int", 127, 6, "<invalid>", "gain~", "list", 107, 10.0, 6, "<invalid>", "gain~", "list", 107, 10.0, 5, "obj-47", "toggle", "int", 1, 5, "obj-44", "toggle", "int", 1, 5, "obj-41", "toggle", "int", 1, 5, "obj-38", "toggle", "int", 0, 5, "obj-17", "flonum", "float", 0.16, 5, "obj-16", "flonum", "float", 0.1 ]
						}
, 						{
							"number" : 4,
							"data" : [ 6, "<invalid>", "gain~", "list", 129, 10.0, 6, "<invalid>", "gain~", "list", 129, 10.0, 5, "obj-89", "flonum", "float", 2.0, 5, "obj-88", "flonum", "float", 0.03, 5, "obj-86", "flonum", "float", 2.0, 5, "obj-85", "flonum", "float", 4.001516, 5, "obj-84", "flonum", "float", 0.03, 5, "obj-78", "flonum", "float", 0.77, 5, "obj-70", "flonum", "float", 200.0, 5, "obj-66", "dial", "int", 44, 5, "obj-65", "dial", "int", 86, 5, "obj-64", "dial", "int", 39, 5, "obj-63", "dial", "int", 127, 6, "<invalid>", "gain~", "list", 107, 10.0, 6, "<invalid>", "gain~", "list", 107, 10.0, 5, "obj-47", "toggle", "int", 1, 5, "obj-44", "toggle", "int", 1, 5, "obj-41", "toggle", "int", 1, 5, "obj-38", "toggle", "int", 0, 5, "obj-17", "flonum", "float", 0.77, 5, "obj-16", "flonum", "float", 0.5 ]
						}
 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "loadmess 1",
					"numinlets" : 1,
					"hidden" : 1,
					"numoutlets" : 1,
					"patching_rect" : [ 363.0, 45.0, 74.0, 20.0 ],
					"outlettype" : [ "" ],
					"id" : "obj-55",
					"fontname" : "Arial",
					"fontsize" : 12.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "rel",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 287.0, 422.0, 25.0, 20.0 ],
					"id" : "obj-59",
					"fontname" : "Arial",
					"fontsize" : 12.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "sus",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 258.0, 422.0, 29.0, 20.0 ],
					"id" : "obj-60",
					"fontname" : "Arial",
					"fontsize" : 12.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "dec",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 230.0, 422.0, 28.0, 20.0 ],
					"id" : "obj-61",
					"fontname" : "Arial",
					"fontsize" : 12.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "p MIDI controls",
					"numinlets" : 4,
					"numoutlets" : 1,
					"patching_rect" : [ 203.0, 467.0, 102.0, 20.0 ],
					"outlettype" : [ "" ],
					"id" : "obj-62",
					"fontname" : "Arial",
					"color" : [ 1.0, 0.741176, 0.611765, 1.0 ],
					"fontsize" : 12.0,
					"patcher" : 					{
						"fileversion" : 1,
						"rect" : [ 10.0, 59.0, 303.0, 223.0 ],
						"bglocked" : 0,
						"defrect" : [ 10.0, 59.0, 303.0, 223.0 ],
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
									"maxclass" : "outlet",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 40.0, 119.0, 15.0, 15.0 ],
									"id" : "obj-1",
									"comment" : ""
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "pak 4 0",
									"numinlets" : 2,
									"numoutlets" : 1,
									"patching_rect" : [ 178.0, 86.0, 43.0, 17.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-2",
									"fontname" : "Arial",
									"fontsize" : 9.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "pak 3 0",
									"numinlets" : 2,
									"numoutlets" : 1,
									"patching_rect" : [ 132.0, 86.0, 43.0, 17.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-3",
									"fontname" : "Arial",
									"fontsize" : 9.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "pak 2 0",
									"numinlets" : 2,
									"numoutlets" : 1,
									"patching_rect" : [ 86.0, 86.0, 43.0, 17.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-4",
									"fontname" : "Arial",
									"fontsize" : 9.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "pak 1 0",
									"numinlets" : 2,
									"numoutlets" : 1,
									"patching_rect" : [ 40.0, 86.0, 43.0, 17.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-5",
									"fontname" : "Arial",
									"fontsize" : 9.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "inlet",
									"numinlets" : 0,
									"numoutlets" : 1,
									"patching_rect" : [ 211.0, 56.0, 15.0, 15.0 ],
									"outlettype" : [ "float" ],
									"id" : "obj-6",
									"comment" : ""
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "inlet",
									"numinlets" : 0,
									"numoutlets" : 1,
									"patching_rect" : [ 165.0, 56.0, 15.0, 15.0 ],
									"outlettype" : [ "float" ],
									"id" : "obj-7",
									"comment" : ""
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "inlet",
									"numinlets" : 0,
									"numoutlets" : 1,
									"patching_rect" : [ 119.0, 56.0, 15.0, 15.0 ],
									"outlettype" : [ "float" ],
									"id" : "obj-8",
									"comment" : ""
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "inlet",
									"numinlets" : 0,
									"numoutlets" : 1,
									"patching_rect" : [ 73.0, 56.0, 15.0, 15.0 ],
									"outlettype" : [ "float" ],
									"id" : "obj-9",
									"comment" : ""
								}

							}
 ],
						"lines" : [ 							{
								"patchline" : 								{
									"source" : [ "obj-6", 0 ],
									"destination" : [ "obj-2", 1 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-7", 0 ],
									"destination" : [ "obj-3", 1 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-8", 0 ],
									"destination" : [ "obj-4", 1 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-9", 0 ],
									"destination" : [ "obj-5", 1 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-2", 0 ],
									"destination" : [ "obj-1", 0 ],
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
									"source" : [ "obj-4", 0 ],
									"destination" : [ "obj-1", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-5", 0 ],
									"destination" : [ "obj-1", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
 ]
					}
,
					"saved_object_attributes" : 					{
						"default_fontname" : "Arial",
						"default_fontsize" : 12.0,
						"fontname" : "Arial",
						"globalpatchername" : "",
						"fontface" : 0,
						"fontsize" : 12.0,
						"default_fontface" : 0
					}

				}

			}
, 			{
				"box" : 				{
					"maxclass" : "dial",
					"numinlets" : 1,
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"numoutlets" : 1,
					"patching_rect" : [ 286.0, 439.0, 26.0, 26.0 ],
					"outlettype" : [ "float" ],
					"id" : "obj-63",
					"fgcolor" : [ 0.666667, 0.666667, 0.666667, 1.0 ],
					"outlinecolor" : [ 0.882353, 0.882353, 0.882353, 1.0 ],
					"needlecolor" : [ 0.0, 0.0, 0.0, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "dial",
					"numinlets" : 1,
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"numoutlets" : 1,
					"patching_rect" : [ 258.0, 439.0, 26.0, 26.0 ],
					"outlettype" : [ "float" ],
					"id" : "obj-64",
					"fgcolor" : [ 0.666667, 0.666667, 0.666667, 1.0 ],
					"outlinecolor" : [ 0.882353, 0.882353, 0.882353, 1.0 ],
					"needlecolor" : [ 0.0, 0.0, 0.0, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "dial",
					"numinlets" : 1,
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"numoutlets" : 1,
					"patching_rect" : [ 231.0, 439.0, 26.0, 26.0 ],
					"outlettype" : [ "float" ],
					"id" : "obj-65",
					"fgcolor" : [ 0.666667, 0.666667, 0.666667, 1.0 ],
					"outlinecolor" : [ 0.882353, 0.882353, 0.882353, 1.0 ],
					"needlecolor" : [ 0.0, 0.0, 0.0, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "dial",
					"numinlets" : 1,
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"numoutlets" : 1,
					"patching_rect" : [ 203.0, 439.0, 26.0, 26.0 ],
					"outlettype" : [ "float" ],
					"id" : "obj-66",
					"fgcolor" : [ 0.666667, 0.666667, 0.666667, 1.0 ],
					"outlinecolor" : [ 0.882353, 0.882353, 0.882353, 1.0 ],
					"needlecolor" : [ 0.0, 0.0, 0.0, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "start",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 23.0, 422.0, 34.0, 20.0 ],
					"id" : "obj-67",
					"fontname" : "Arial",
					"fontsize" : 12.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "button",
					"numinlets" : 1,
					"numoutlets" : 1,
					"patching_rect" : [ 29.0, 439.0, 18.0, 18.0 ],
					"outlettype" : [ "bang" ],
					"id" : "obj-68",
					"fgcolor" : [ 0.101961, 0.541176, 0.054902, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "stop",
					"numinlets" : 2,
					"numoutlets" : 1,
					"patching_rect" : [ 50.0, 439.0, 34.0, 18.0 ],
					"outlettype" : [ "" ],
					"id" : "obj-69",
					"fontname" : "Arial",
					"fontsize" : 12.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "flonum",
					"numinlets" : 1,
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ],
					"numoutlets" : 2,
					"patching_rect" : [ 87.0, 439.0, 42.0, 20.0 ],
					"triscale" : 0.9,
					"minimum" : 0.0,
					"outlettype" : [ "float", "bang" ],
					"id" : "obj-70",
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "BPM",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 91.0, 422.0, 35.0, 20.0 ],
					"id" : "obj-71",
					"fontname" : "Arial",
					"fontsize" : 12.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "p Random Note Generator",
					"numinlets" : 2,
					"numoutlets" : 1,
					"patching_rect" : [ 29.0, 467.0, 158.0, 20.0 ],
					"outlettype" : [ "" ],
					"id" : "obj-72",
					"fontname" : "Arial",
					"color" : [ 1.0, 0.741176, 0.611765, 1.0 ],
					"fontsize" : 12.0,
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
									"numinlets" : 0,
									"numoutlets" : 1,
									"patching_rect" : [ 150.0, 15.0, 25.0, 25.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-1",
									"comment" : ""
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "inlet",
									"numinlets" : 0,
									"numoutlets" : 1,
									"patching_rect" : [ 210.0, 15.0, 25.0, 25.0 ],
									"outlettype" : [ "float" ],
									"id" : "obj-2",
									"comment" : "BPM"
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "makenote 64 500",
									"numinlets" : 3,
									"numoutlets" : 2,
									"patching_rect" : [ 150.0, 240.0, 106.0, 20.0 ],
									"outlettype" : [ "float", "float" ],
									"id" : "obj-3",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "+ 30",
									"numinlets" : 2,
									"numoutlets" : 1,
									"patching_rect" : [ 150.0, 210.0, 39.0, 20.0 ],
									"outlettype" : [ "int" ],
									"id" : "obj-4",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "random 60",
									"numinlets" : 2,
									"numoutlets" : 1,
									"patching_rect" : [ 150.0, 180.0, 72.0, 20.0 ],
									"outlettype" : [ "int" ],
									"id" : "obj-5",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "* 1.",
									"numinlets" : 2,
									"numoutlets" : 1,
									"patching_rect" : [ 210.0, 75.0, 36.0, 20.0 ],
									"outlettype" : [ "float" ],
									"id" : "obj-7",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "* 60000.",
									"numinlets" : 2,
									"numoutlets" : 1,
									"patching_rect" : [ 210.0, 105.0, 66.0, 20.0 ],
									"outlettype" : [ "float" ],
									"id" : "obj-8",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "pow -1.",
									"numinlets" : 2,
									"numoutlets" : 1,
									"patching_rect" : [ 210.0, 45.0, 56.0, 20.0 ],
									"outlettype" : [ "float" ],
									"id" : "obj-9",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "metro 500",
									"numinlets" : 2,
									"numoutlets" : 1,
									"patching_rect" : [ 150.0, 150.0, 72.0, 20.0 ],
									"outlettype" : [ "bang" ],
									"id" : "obj-10",
									"fontname" : "Arial",
									"color" : [ 1.0, 0.890196, 0.090196, 1.0 ],
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "outlet",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 150.0, 315.0, 25.0, 25.0 ],
									"id" : "obj-11",
									"comment" : ""
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "pack 0 0",
									"numinlets" : 2,
									"numoutlets" : 1,
									"patching_rect" : [ 150.0, 285.0, 58.0, 20.0 ],
									"outlettype" : [ "" ],
									"id" : "obj-13",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "BPM",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 240.0, 15.0, 40.0, 20.0 ],
									"id" : "obj-14",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "metro control",
									"numinlets" : 1,
									"numoutlets" : 0,
									"patching_rect" : [ 60.0, 15.0, 91.0, 20.0 ],
									"id" : "obj-15",
									"fontname" : "Arial",
									"fontsize" : 12.0
								}

							}
 ],
						"lines" : [ 							{
								"patchline" : 								{
									"source" : [ "obj-8", 0 ],
									"destination" : [ "obj-10", 1 ],
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
									"source" : [ "obj-9", 0 ],
									"destination" : [ "obj-7", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-2", 0 ],
									"destination" : [ "obj-9", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-13", 0 ],
									"destination" : [ "obj-11", 0 ],
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
									"source" : [ "obj-10", 0 ],
									"destination" : [ "obj-5", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-1", 0 ],
									"destination" : [ "obj-10", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-8", 0 ],
									"destination" : [ "obj-3", 2 ],
									"hidden" : 0,
									"midpoints" : [ 219.5, 139.0, 246.5, 139.0 ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-3", 1 ],
									"destination" : [ "obj-13", 1 ],
									"hidden" : 0,
									"midpoints" : [ 246.5, 268.0, 198.5, 268.0 ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-3", 0 ],
									"destination" : [ "obj-13", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-4", 0 ],
									"destination" : [ "obj-3", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
 ]
					}
,
					"saved_object_attributes" : 					{
						"default_fontname" : "Arial",
						"default_fontsize" : 12.0,
						"fontname" : "Arial",
						"globalpatchername" : "",
						"fontface" : 0,
						"fontsize" : 12.0,
						"default_fontface" : 0
					}

				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "c car $1",
					"numinlets" : 2,
					"bgcolor" : [ 1.0, 0.992157, 0.94902, 1.0 ],
					"numoutlets" : 1,
					"patching_rect" : [ 15.0, 184.0, 53.0, 18.0 ],
					"outlettype" : [ "" ],
					"id" : "obj-73",
					"fontname" : "Arial",
					"fontsize" : 12.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "c mod $1",
					"numinlets" : 2,
					"bgcolor" : [ 1.0, 0.992157, 0.94902, 1.0 ],
					"numoutlets" : 1,
					"patching_rect" : [ 77.0, 184.0, 65.0, 18.0 ],
					"outlettype" : [ "" ],
					"id" : "obj-74",
					"fontname" : "Arial",
					"fontsize" : 12.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "c ndx $1",
					"numinlets" : 2,
					"bgcolor" : [ 1.0, 0.992157, 0.94902, 1.0 ],
					"numoutlets" : 1,
					"patching_rect" : [ 146.0, 184.0, 62.0, 18.0 ],
					"outlettype" : [ "" ],
					"id" : "obj-75",
					"fontname" : "Arial",
					"fontsize" : 12.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "scope~",
					"numinlets" : 2,
					"bgcolor" : [ 0.082353, 0.082353, 0.082353, 1.0 ],
					"numoutlets" : 0,
					"patching_rect" : [ 427.0, 465.0, 111.0, 101.0 ],
					"id" : "obj-76",
					"gridcolor" : [ 0.141176, 0.141176, 0.141176, 1.0 ],
					"rounded" : 0,
					"fgcolor" : [ 0.682353, 1.0, 0.298039, 1.0 ],
					"calccount" : 16
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "midiformat 1",
					"numinlets" : 7,
					"hidden" : 1,
					"numoutlets" : 1,
					"patching_rect" : [ 15.0, 505.0, 119.0, 20.0 ],
					"outlettype" : [ "int" ],
					"id" : "obj-77",
					"fontname" : "Arial",
					"fontsize" : 12.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "flonum",
					"numinlets" : 1,
					"bgcolor" : [ 0.909804, 0.909804, 0.909804, 1.0 ],
					"numoutlets" : 2,
					"patching_rect" : [ 146.0, 163.0, 52.0, 20.0 ],
					"triscale" : 0.9,
					"minimum" : 0.0,
					"outlettype" : [ "float", "bang" ],
					"id" : "obj-78",
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "stop",
					"numinlets" : 2,
					"bgcolor" : [ 1.0, 0.992157, 0.94902, 1.0 ],
					"numoutlets" : 1,
					"patching_rect" : [ 382.0, 98.0, 36.0, 18.0 ],
					"outlettype" : [ "" ],
					"id" : "obj-80",
					"fontname" : "Arial",
					"fontsize" : 12.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "start",
					"numinlets" : 2,
					"bgcolor" : [ 1.0, 1.0, 0.090196, 1.0 ],
					"numoutlets" : 1,
					"patching_rect" : [ 382.0, 76.0, 39.0, 18.0 ],
					"outlettype" : [ "" ],
					"id" : "obj-81",
					"fontname" : "Arial",
					"fontsize" : 12.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "rewind",
					"numinlets" : 2,
					"bgcolor" : [ 1.0, 0.992157, 0.94902, 1.0 ],
					"numoutlets" : 1,
					"patching_rect" : [ 382.0, 120.0, 55.0, 18.0 ],
					"outlettype" : [ "" ],
					"id" : "obj-82",
					"fontname" : "Arial",
					"fontsize" : 12.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "reset",
					"numinlets" : 2,
					"bgcolor" : [ 1.0, 0.992157, 0.94902, 1.0 ],
					"numoutlets" : 1,
					"patching_rect" : [ 382.0, 141.0, 44.0, 18.0 ],
					"outlettype" : [ "" ],
					"id" : "obj-83",
					"fontname" : "Arial",
					"fontsize" : 12.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "flonum",
					"numinlets" : 1,
					"bgcolor" : [ 0.909804, 0.909804, 0.909804, 1.0 ],
					"numoutlets" : 2,
					"patching_rect" : [ 104.0, 283.0, 47.0, 20.0 ],
					"triscale" : 0.9,
					"outlettype" : [ "float", "bang" ],
					"id" : "obj-84",
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "flonum",
					"numinlets" : 1,
					"bgcolor" : [ 0.909804, 0.909804, 0.909804, 1.0 ],
					"numoutlets" : 2,
					"patching_rect" : [ 60.0, 283.0, 45.0, 20.0 ],
					"triscale" : 0.9,
					"outlettype" : [ "float", "bang" ],
					"id" : "obj-85",
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "flonum",
					"numinlets" : 1,
					"bgcolor" : [ 0.909804, 0.909804, 0.909804, 1.0 ],
					"numoutlets" : 2,
					"patching_rect" : [ 15.0, 283.0, 47.0, 20.0 ],
					"triscale" : 0.9,
					"outlettype" : [ "float", "bang" ],
					"id" : "obj-86",
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"varname" : "messages",
					"text" : "route car carX2 mod ndx",
					"numinlets" : 1,
					"numoutlets" : 5,
					"patching_rect" : [ 15.0, 257.0, 197.0, 20.0 ],
					"outlettype" : [ "", "", "", "", "" ],
					"id" : "obj-87",
					"fontname" : "Arial",
					"fontsize" : 12.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "flonum",
					"numinlets" : 1,
					"bgcolor" : [ 0.909804, 0.909804, 0.909804, 1.0 ],
					"numoutlets" : 2,
					"patching_rect" : [ 77.0, 163.0, 52.0, 20.0 ],
					"triscale" : 0.9,
					"minimum" : 0.0,
					"outlettype" : [ "float", "bang" ],
					"id" : "obj-88",
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "flonum",
					"numinlets" : 1,
					"bgcolor" : [ 0.909804, 0.909804, 0.909804, 1.0 ],
					"numoutlets" : 2,
					"patching_rect" : [ 15.0, 163.0, 52.0, 20.0 ],
					"triscale" : 0.9,
					"minimum" : 0.0,
					"outlettype" : [ "float", "bang" ],
					"id" : "obj-89",
					"fontname" : "Arial",
					"fontsize" : 12.0,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"varname" : "csound~",
					"text" : "csound~ @args csound~.csd @io 4",
					"numinlets" : 4,
					"textcolor" : [ 0.301961, 0.294118, 0.376471, 1.0 ],
					"numoutlets" : 8,
					"patching_rect" : [ 360.0, 360.0, 245.0, 23.0 ],
					"outlettype" : [ "signal", "signal", "signal", "signal", "list", "int", "bang", "bang" ],
					"id" : "obj-94",
					"fontname" : "Arial",
					"fontface" : 3,
					"fontsize" : 14.0,
					"saved_object_attributes" : 					{
						"matchsr" : 1,
						"bypass" : 0,
						"overdrive" : 0,
						"output" : 1,
						"input" : 1,
						"interval" : 10,
						"message" : 1,
						"autostart" : 0,
						"args" : "csound~.csd"
					}

				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "Start csound performance (from beginning).",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 426.0, 76.0, 255.0, 20.0 ],
					"id" : "obj-95",
					"fontname" : "Arial",
					"fontsize" : 12.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "Stop csound performance.",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 419.0, 98.0, 158.0, 20.0 ],
					"id" : "obj-96",
					"fontname" : "Arial",
					"fontsize" : 12.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "Rewind csound score to beginning.",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 437.0, 120.0, 207.0, 20.0 ],
					"id" : "obj-97",
					"fontname" : "Arial",
					"fontsize" : 12.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "Restart csound performance.",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 427.0, 141.0, 178.0, 20.0 ],
					"id" : "obj-98",
					"fontname" : "Arial",
					"fontsize" : 12.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "Input processing (enabled by default).",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 459.0, 183.0, 245.0, 20.0 ],
					"id" : "obj-99",
					"fontname" : "Arial",
					"fontsize" : 12.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "atk",
					"numinlets" : 1,
					"numoutlets" : 0,
					"patching_rect" : [ 203.0, 422.0, 25.0, 20.0 ],
					"id" : "obj-102",
					"fontname" : "Arial",
					"fontsize" : 12.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "Send score events.",
					"numinlets" : 1,
					"frgb" : [ 0.301961, 0.294118, 0.376471, 1.0 ],
					"textcolor" : [ 0.301961, 0.294118, 0.376471, 1.0 ],
					"numoutlets" : 0,
					"patching_rect" : [ 15.0, 54.0, 151.0, 23.0 ],
					"id" : "obj-103",
					"fontname" : "Arial",
					"fontface" : 3,
					"fontsize" : 14.0
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "panel",
					"numinlets" : 1,
					"bgcolor" : [ 1.0, 1.0, 1.0, 1.0 ],
					"numoutlets" : 0,
					"border" : 2,
					"bordercolor" : [ 0.666667, 0.666667, 0.666667, 1.0 ],
					"patching_rect" : [ 784.0, 53.0, 205.0, 293.0 ],
					"id" : "obj-104",
					"rounded" : 12
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "panel",
					"varname" : "autohelp_top_panel",
					"numinlets" : 1,
					"presentation_rect" : [ 45.0, 30.0, 539.0, 45.0 ],
					"numoutlets" : 0,
					"mode" : 1,
					"patching_rect" : [ 15.0, 0.0, 953.0, 48.0 ],
					"presentation" : 1,
					"id" : "obj-32",
					"grad1" : [ 0.101961, 0.101961, 0.101961, 1.0 ],
					"background" : 1,
					"grad2" : [ 1.0, 1.0, 1.0, 1.0 ]
				}

			}
 ],
		"lines" : [ 			{
				"patchline" : 				{
					"source" : [ "obj-20", 0 ],
					"destination" : [ "obj-63", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-20", 0 ],
					"destination" : [ "obj-64", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-20", 0 ],
					"destination" : [ "obj-65", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-20", 0 ],
					"destination" : [ "obj-66", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-94", 6 ],
					"destination" : [ "obj-31", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-7", 0 ],
					"destination" : [ "obj-79", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-73", 0 ],
					"destination" : [ "obj-113", 0 ],
					"hidden" : 1,
					"midpoints" : [ 24.5, 206.0, 9.5, 206.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-74", 0 ],
					"destination" : [ "obj-113", 0 ],
					"hidden" : 1,
					"midpoints" : [ 86.5, 206.0, 9.5, 206.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-75", 0 ],
					"destination" : [ "obj-113", 0 ],
					"hidden" : 1,
					"midpoints" : [ 155.5, 206.0, 9.5, 206.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-15", 0 ],
					"destination" : [ "obj-113", 0 ],
					"hidden" : 1,
					"midpoints" : [ 219.5, 206.0, 9.5, 206.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-55", 0 ],
					"destination" : [ "obj-50", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-77", 0 ],
					"destination" : [ "obj-108", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-25", 0 ],
					"destination" : [ "obj-94", 0 ],
					"hidden" : 1,
					"midpoints" : [ 369.5, 354.5, 369.5, 354.5 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-7", 0 ],
					"destination" : [ "obj-23", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-55", 0 ],
					"destination" : [ "obj-53", 0 ],
					"hidden" : 1,
					"midpoints" : [ 372.5, 67.0, 279.5, 67.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-87", 3 ],
					"destination" : [ "obj-17", 0 ],
					"hidden" : 0,
					"midpoints" : [ 158.0, 278.0, 158.5, 278.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-87", 2 ],
					"destination" : [ "obj-84", 0 ],
					"hidden" : 0,
					"midpoints" : [ 113.5, 278.0, 113.5, 278.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-87", 1 ],
					"destination" : [ "obj-85", 0 ],
					"hidden" : 0,
					"midpoints" : [ 69.0, 278.0, 69.5, 278.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-87", 0 ],
					"destination" : [ "obj-86", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-38", 0 ],
					"destination" : [ "obj-37", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-41", 0 ],
					"destination" : [ "obj-40", 0 ],
					"hidden" : 1,
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
					"source" : [ "obj-47", 0 ],
					"destination" : [ "obj-46", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-16", 0 ],
					"destination" : [ "obj-15", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-63", 0 ],
					"destination" : [ "obj-62", 3 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-78", 0 ],
					"destination" : [ "obj-75", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-64", 0 ],
					"destination" : [ "obj-62", 2 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-65", 0 ],
					"destination" : [ "obj-62", 1 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-88", 0 ],
					"destination" : [ "obj-74", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-66", 0 ],
					"destination" : [ "obj-62", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-70", 0 ],
					"destination" : [ "obj-72", 1 ],
					"hidden" : 0,
					"midpoints" : [ 96.5, 461.0, 177.5, 461.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-89", 0 ],
					"destination" : [ "obj-73", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-51", 1 ],
					"destination" : [ "obj-48", 1 ],
					"hidden" : 1,
					"midpoints" : [ 317.5, 387.0, 51.5, 387.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-62", 0 ],
					"destination" : [ "obj-77", 2 ],
					"hidden" : 1,
					"midpoints" : [ 212.5, 498.0, 57.833332, 498.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-69", 0 ],
					"destination" : [ "obj-72", 0 ],
					"hidden" : 0,
					"midpoints" : [ 59.5, 462.0, 38.5, 462.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-68", 0 ],
					"destination" : [ "obj-72", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-48", 0 ],
					"destination" : [ "obj-77", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-72", 0 ],
					"destination" : [ "obj-77", 0 ],
					"hidden" : 1,
					"midpoints" : [ 38.5, 499.0, 24.5, 499.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-51", 0 ],
					"destination" : [ "obj-48", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-94", 4 ],
					"destination" : [ "obj-11", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-14", 0 ],
					"destination" : [ "obj-21", 2 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-94", 0 ],
					"destination" : [ "obj-21", 0 ],
					"hidden" : 0,
					"midpoints" : [ 369.5, 383.5, 369.5, 383.5 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-94", 1 ],
					"destination" : [ "obj-21", 1 ],
					"hidden" : 0,
					"midpoints" : [ 401.785706, 403.5, 388.0, 403.5 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-94", 2 ],
					"destination" : [ "obj-21", 0 ],
					"hidden" : 0,
					"midpoints" : [ 434.071442, 391.5, 369.5, 391.5 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-94", 3 ],
					"destination" : [ "obj-21", 1 ],
					"hidden" : 0,
					"midpoints" : [ 466.357147, 403.5, 388.0, 403.5 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-21", 0 ],
					"destination" : [ "obj-118", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-21", 1 ],
					"destination" : [ "obj-118", 1 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-21", 1 ],
					"destination" : [ "obj-76", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-36", 0 ],
					"destination" : [ "obj-113", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-52", 0 ],
					"destination" : [ "obj-113", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-37", 0 ],
					"destination" : [ "obj-33", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-40", 0 ],
					"destination" : [ "obj-33", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-43", 0 ],
					"destination" : [ "obj-33", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-46", 0 ],
					"destination" : [ "obj-33", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-83", 0 ],
					"destination" : [ "obj-33", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-82", 0 ],
					"destination" : [ "obj-33", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-80", 0 ],
					"destination" : [ "obj-33", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-81", 0 ],
					"destination" : [ "obj-33", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-50", 0 ],
					"destination" : [ "obj-33", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-35", 0 ],
					"destination" : [ "obj-34", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-34", 0 ],
					"destination" : [ "obj-33", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-53", 3 ],
					"destination" : [ "obj-58", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-58", 0 ],
					"destination" : [ "obj-91", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
 ],
		"parameters" : 		{

		}

	}

}
