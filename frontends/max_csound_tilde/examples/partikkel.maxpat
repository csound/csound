{
	"patcher" : 	{
		"fileversion" : 1,
		"rect" : [ 190.0, 132.0, 1027.0, 556.0 ],
		"bglocked" : 0,
		"defrect" : [ 190.0, 132.0, 1027.0, 556.0 ],
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
					"text" : "2",
					"numoutlets" : 0,
					"id" : "obj-78",
					"textcolor" : [ 0.176471, 0.301961, 1.0, 1.0 ],
					"fontface" : 1,
					"fontsize" : 36.0,
					"patching_rect" : [ 46.0, 345.0, 38.0, 48.0 ],
					"numinlets" : 1,
					"fontname" : "Arial",
					"frgb" : [ 0.176471, 0.301961, 1.0, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "1",
					"numoutlets" : 0,
					"id" : "obj-77",
					"textcolor" : [ 0.176471, 0.301961, 1.0, 1.0 ],
					"fontface" : 1,
					"fontsize" : 36.0,
					"patching_rect" : [ 746.0, 316.0, 38.0, 48.0 ],
					"numinlets" : 1,
					"fontname" : "Arial",
					"frgb" : [ 0.176471, 0.301961, 1.0, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "1) Start DSP.",
					"numoutlets" : 0,
					"id" : "obj-47",
					"textcolor" : [ 0.0, 0.14902, 0.964706, 1.0 ],
					"fontsize" : 14.0,
					"patching_rect" : [ 824.0, 314.0, 139.0, 23.0 ],
					"numinlets" : 1,
					"fontname" : "Arial",
					"frgb" : [ 0.0, 0.14902, 0.964706, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "2) Load an audio file.  The partikkel instrument (instr #1) will be activated automatically.",
					"linecount" : 3,
					"numoutlets" : 0,
					"id" : "obj-76",
					"textcolor" : [ 0.0, 0.14902, 0.964706, 1.0 ],
					"fontsize" : 14.0,
					"patching_rect" : [ 824.0, 339.0, 199.0, 55.0 ],
					"numinlets" : 1,
					"fontname" : "Arial",
					"frgb" : [ 0.0, 0.14902, 0.964706, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "This patch is centered around partikkel, which is a granular Csound opcode.  It uses the \"loadsamp\" message to replace Csound audio tables. ",
					"linecount" : 5,
					"numoutlets" : 0,
					"id" : "obj-45",
					"textcolor" : [ 0.0, 0.2, 0.490196, 1.0 ],
					"fontsize" : 12.0,
					"patching_rect" : [ 824.0, 234.0, 202.0, 75.0 ],
					"numinlets" : 1,
					"fontname" : "Arial",
					"frgb" : [ 0.0, 0.2, 0.490196, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "flonum",
					"numoutlets" : 2,
					"id" : "obj-46",
					"outlettype" : [ "float", "bang" ],
					"fontface" : 1,
					"fontsize" : 14.0,
					"patching_rect" : [ 267.0, 289.0, 52.0, 23.0 ],
					"numinlets" : 1,
					"fontname" : "Arial"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "Opening an audio file automatically re-activates the partikkel instr.  If loading large audio files, increase activation delay time.",
					"linecount" : 4,
					"numoutlets" : 0,
					"id" : "obj-74",
					"textcolor" : [ 0.0, 0.2, 0.490196, 1.0 ],
					"fontsize" : 12.0,
					"patching_rect" : [ 211.0, 313.0, 203.0, 62.0 ],
					"numinlets" : 1,
					"fontname" : "Arial",
					"frgb" : [ 0.0, 0.2, 0.490196, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "Partikkel Instrument Activation Delay Time (sec)",
					"linecount" : 2,
					"numoutlets" : 0,
					"id" : "obj-55",
					"textcolor" : [ 0.0, 0.2, 0.490196, 1.0 ],
					"fontface" : 3,
					"fontsize" : 12.0,
					"patching_rect" : [ 220.0, 256.0, 165.0, 34.0 ],
					"numinlets" : 1,
					"fontname" : "Arial",
					"frgb" : [ 0.0, 0.2, 0.490196, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "grain pos randomness",
					"linecount" : 2,
					"numoutlets" : 0,
					"id" : "obj-43",
					"textcolor" : [ 0.0, 0.2, 0.490196, 1.0 ],
					"fontface" : 3,
					"fontsize" : 12.0,
					"patching_rect" : [ 395.0, 58.0, 90.0, 34.0 ],
					"numinlets" : 1,
					"fontname" : "Arial",
					"frgb" : [ 0.0, 0.2, 0.490196, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "Load Last File",
					"numoutlets" : 0,
					"id" : "obj-42",
					"textcolor" : [ 0.0, 0.2, 0.490196, 1.0 ],
					"fontface" : 3,
					"fontsize" : 12.0,
					"patching_rect" : [ 20.0, 255.0, 92.0, 20.0 ],
					"numinlets" : 1,
					"fontname" : "Arial",
					"frgb" : [ 0.0, 0.2, 0.490196, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "button",
					"numoutlets" : 1,
					"outlinecolor" : [ 0.0, 0.0, 0.0, 1.0 ],
					"id" : "obj-37",
					"outlettype" : [ "bang" ],
					"patching_rect" : [ 30.0, 273.0, 66.0, 66.0 ],
					"numinlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "Re-activate partikkel instrument after changing the grain window.",
					"linecount" : 4,
					"numoutlets" : 0,
					"id" : "obj-19",
					"textcolor" : [ 0.0, 0.196078, 0.490196, 1.0 ],
					"fontface" : 3,
					"fontsize" : 12.0,
					"patching_rect" : [ 104.0, 158.0, 130.0, 62.0 ],
					"numinlets" : 1,
					"fontname" : "Arial",
					"frgb" : [ 0.0, 0.196078, 0.490196, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "button",
					"numoutlets" : 1,
					"outlinecolor" : [ 0.0, 0.0, 0.0, 1.0 ],
					"id" : "obj-18",
					"outlettype" : [ "bang" ],
					"patching_rect" : [ 262.0, 138.0, 85.0, 85.0 ],
					"numinlets" : 1,
					"fgcolor" : [ 0.615686, 0.670588, 1.0, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "pattrforward cIN",
					"numoutlets" : 1,
					"id" : "obj-16",
					"outlettype" : [ "" ],
					"fontsize" : 12.0,
					"patching_rect" : [ 5.0, 224.0, 96.0, 20.0 ],
					"numinlets" : 1,
					"hidden" : 1,
					"fontname" : "Arial"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "i_win $1",
					"numoutlets" : 1,
					"id" : "obj-54",
					"outlettype" : [ "" ],
					"fontsize" : 12.0,
					"patching_rect" : [ 5.0, 202.0, 64.0, 18.0 ],
					"numinlets" : 2,
					"fontname" : "Arial"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "dB amp",
					"numoutlets" : 0,
					"id" : "obj-24",
					"fontsize" : 12.0,
					"patching_rect" : [ 723.0, 244.0, 52.0, 20.0 ],
					"numinlets" : 1,
					"fontname" : "Arial"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "flonum",
					"numoutlets" : 2,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ],
					"id" : "obj-36",
					"maximum" : 0.0,
					"outlettype" : [ "float", "bang" ],
					"fontsize" : 12.0,
					"patching_rect" : [ 727.0, 262.0, 40.0, 20.0 ],
					"triscale" : 0.9,
					"numinlets" : 1,
					"fontname" : "Arial",
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "p amp",
					"numoutlets" : 2,
					"id" : "obj-40",
					"outlettype" : [ "signal", "signal" ],
					"fontsize" : 12.0,
					"patching_rect" : [ 681.0, 284.0, 65.0, 20.0 ],
					"numinlets" : 3,
					"fontname" : "Arial",
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
									"numoutlets" : 0,
									"id" : "obj-1",
									"patching_rect" : [ 90.0, 148.0, 21.0, 21.0 ],
									"numinlets" : 1,
									"comment" : ""
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "outlet",
									"numoutlets" : 0,
									"id" : "obj-2",
									"patching_rect" : [ 33.0, 148.0, 21.0, 21.0 ],
									"numinlets" : 1,
									"comment" : ""
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "dbtoa",
									"numoutlets" : 1,
									"id" : "obj-3",
									"outlettype" : [ "" ],
									"fontsize" : 12.0,
									"patching_rect" : [ 129.0, 70.0, 48.0, 20.0 ],
									"numinlets" : 1,
									"fontname" : "Arial"
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "*~ 1.",
									"numoutlets" : 1,
									"id" : "obj-4",
									"outlettype" : [ "signal" ],
									"fontsize" : 12.0,
									"patching_rect" : [ 90.0, 105.0, 49.0, 20.0 ],
									"numinlets" : 2,
									"fontname" : "Arial"
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "*~ 1.",
									"numoutlets" : 1,
									"id" : "obj-5",
									"outlettype" : [ "signal" ],
									"fontsize" : 12.0,
									"patching_rect" : [ 33.0, 105.0, 49.0, 20.0 ],
									"numinlets" : 2,
									"fontname" : "Arial"
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "inlet",
									"numoutlets" : 1,
									"id" : "obj-6",
									"outlettype" : [ "float" ],
									"patching_rect" : [ 129.0, 41.0, 21.0, 21.0 ],
									"numinlets" : 0,
									"comment" : ""
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "inlet",
									"numoutlets" : 1,
									"id" : "obj-7",
									"outlettype" : [ "signal" ],
									"patching_rect" : [ 90.0, 41.0, 21.0, 21.0 ],
									"numinlets" : 0,
									"comment" : ""
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "inlet",
									"numoutlets" : 1,
									"id" : "obj-8",
									"outlettype" : [ "signal" ],
									"patching_rect" : [ 33.0, 41.0, 21.0, 21.0 ],
									"numinlets" : 0,
									"comment" : ""
								}

							}
 ],
						"lines" : [ 							{
								"patchline" : 								{
									"source" : [ "obj-8", 0 ],
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
									"source" : [ "obj-7", 0 ],
									"destination" : [ "obj-4", 0 ],
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
									"source" : [ "obj-6", 0 ],
									"destination" : [ "obj-3", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-3", 0 ],
									"destination" : [ "obj-4", 1 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
 ]
					}
,
					"saved_object_attributes" : 					{
						"default_fontface" : 0,
						"fontface" : 0,
						"fontsize" : 12.0,
						"default_fontname" : "Arial",
						"globalpatchername" : "",
						"default_fontsize" : 12.0,
						"fontname" : "Arial"
					}

				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "loadbang",
					"numoutlets" : 1,
					"id" : "obj-12",
					"outlettype" : [ "bang" ],
					"fontsize" : 12.0,
					"patching_rect" : [ 808.0, 13.0, 60.0, 20.0 ],
					"numinlets" : 1,
					"hidden" : 1,
					"fontname" : "Arial"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "p reactivate_partikkel_instr",
					"numoutlets" : 0,
					"id" : "obj-10",
					"fontface" : 3,
					"fontsize" : 14.0,
					"color" : [ 1.0, 1.0, 1.0, 1.0 ],
					"patching_rect" : [ 262.0, 225.0, 194.0, 23.0 ],
					"numinlets" : 1,
					"fontname" : "Arial",
					"bgcolor" : [ 0.760784, 0.866667, 1.0, 1.0 ],
					"patcher" : 					{
						"fileversion" : 1,
						"rect" : [ 676.0, 221.0, 405.0, 217.0 ],
						"bglocked" : 0,
						"defrect" : [ 676.0, 221.0, 405.0, 217.0 ],
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
									"text" : "pattrforward parent::csIN",
									"numoutlets" : 1,
									"id" : "obj-1",
									"outlettype" : [ "" ],
									"fontsize" : 12.0,
									"patching_rect" : [ 90.0, 180.0, 143.0, 20.0 ],
									"numinlets" : 1,
									"fontname" : "Arial"
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "Turn off instr #1, wait 200ms, then create an instance of instr #1.",
									"linecount" : 2,
									"numoutlets" : 0,
									"id" : "obj-36",
									"textcolor" : [ 0.0, 0.196078, 0.490196, 1.0 ],
									"fontface" : 3,
									"fontsize" : 12.0,
									"patching_rect" : [ 120.0, 15.0, 278.0, 34.0 ],
									"numinlets" : 1,
									"fontname" : "Arial",
									"frgb" : [ 0.0, 0.196078, 0.490196, 1.0 ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "delay 200",
									"numoutlets" : 1,
									"id" : "obj-12",
									"outlettype" : [ "bang" ],
									"fontsize" : 12.0,
									"patching_rect" : [ 90.0, 90.0, 63.0, 20.0 ],
									"numinlets" : 2,
									"fontname" : "Arial"
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "t b b",
									"numoutlets" : 2,
									"id" : "obj-10",
									"outlettype" : [ "bang", "bang" ],
									"fontsize" : 12.0,
									"patching_rect" : [ 90.0, 60.0, 109.0, 20.0 ],
									"numinlets" : 1,
									"fontname" : "Arial"
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "activate partikkel instrument",
									"linecount" : 3,
									"numoutlets" : 0,
									"id" : "obj-93",
									"fontsize" : 12.0,
									"patching_rect" : [ 34.0, 109.0, 72.0, 48.0 ],
									"numinlets" : 1,
									"fontname" : "Arial"
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "turn off partikkel instrument",
									"linecount" : 3,
									"numoutlets" : 0,
									"id" : "obj-91",
									"fontsize" : 12.0,
									"patching_rect" : [ 240.0, 75.0, 84.0, 48.0 ],
									"numinlets" : 1,
									"fontname" : "Arial"
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "e i2 0 0.1",
									"numoutlets" : 1,
									"id" : "obj-90",
									"outlettype" : [ "" ],
									"fontsize" : 12.0,
									"patching_rect" : [ 180.0, 90.0, 60.0, 18.0 ],
									"numinlets" : 2,
									"fontname" : "Arial"
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "e i1 0 -1",
									"numoutlets" : 1,
									"id" : "obj-89",
									"outlettype" : [ "" ],
									"fontsize" : 12.0,
									"patching_rect" : [ 90.0, 120.0, 54.0, 18.0 ],
									"numinlets" : 2,
									"fontname" : "Arial"
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "inlet",
									"numoutlets" : 1,
									"id" : "obj-5",
									"outlettype" : [ "bang" ],
									"patching_rect" : [ 90.0, 15.0, 25.0, 25.0 ],
									"numinlets" : 0,
									"comment" : ""
								}

							}
 ],
						"lines" : [ 							{
								"patchline" : 								{
									"source" : [ "obj-10", 0 ],
									"destination" : [ "obj-12", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-12", 0 ],
									"destination" : [ "obj-89", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-5", 0 ],
									"destination" : [ "obj-10", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-10", 1 ],
									"destination" : [ "obj-90", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-90", 0 ],
									"destination" : [ "obj-1", 0 ],
									"hidden" : 0,
									"midpoints" : [ 189.5, 155.0, 99.5, 155.0 ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-89", 0 ],
									"destination" : [ "obj-1", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
 ]
					}
,
					"saved_object_attributes" : 					{
						"default_fontface" : 0,
						"fontface" : 0,
						"fontsize" : 12.0,
						"default_fontname" : "Arial",
						"globalpatchername" : "",
						"default_fontsize" : 12.0,
						"fontname" : "Arial"
					}

				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "file position (in secs)",
					"numoutlets" : 0,
					"id" : "obj-9",
					"textcolor" : [ 0.0, 0.2, 0.490196, 1.0 ],
					"fontface" : 3,
					"fontsize" : 12.0,
					"patching_rect" : [ 550.0, 90.0, 143.0, 20.0 ],
					"numinlets" : 1,
					"fontname" : "Arial",
					"frgb" : [ 0.0, 0.2, 0.490196, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "csound~ 1.0.6 or later required.",
					"numoutlets" : 0,
					"id" : "obj-1",
					"textcolor" : [ 0.65098, 0.0, 0.019608, 1.0 ],
					"fontsize" : 12.0,
					"patching_rect" : [ 824.0, 162.0, 184.0, 20.0 ],
					"numinlets" : 1,
					"fontname" : "Arial",
					"frgb" : [ 0.65098, 0.0, 0.019608, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "p open",
					"numoutlets" : 1,
					"id" : "obj-2",
					"outlettype" : [ "" ],
					"fontsize" : 12.0,
					"patching_rect" : [ 121.0, 341.0, 47.0, 20.0 ],
					"numinlets" : 1,
					"hidden" : 1,
					"fontname" : "Arial",
					"patcher" : 					{
						"fileversion" : 1,
						"rect" : [ 414.0, 283.0, 169.0, 182.0 ],
						"bglocked" : 0,
						"defrect" : [ 414.0, 283.0, 169.0, 182.0 ],
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
									"maxclass" : "message",
									"text" : "sound",
									"numoutlets" : 1,
									"id" : "obj-4",
									"outlettype" : [ "" ],
									"fontsize" : 12.0,
									"patching_rect" : [ 30.0, 60.0, 43.0, 18.0 ],
									"numinlets" : 2,
									"fontname" : "Arial"
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "opendialog",
									"numoutlets" : 2,
									"id" : "obj-3",
									"outlettype" : [ "", "bang" ],
									"fontsize" : 12.0,
									"patching_rect" : [ 30.0, 90.0, 70.0, 20.0 ],
									"numinlets" : 1,
									"fontname" : "Arial"
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "outlet",
									"numoutlets" : 0,
									"id" : "obj-2",
									"patching_rect" : [ 30.0, 120.0, 25.0, 25.0 ],
									"numinlets" : 1,
									"comment" : ""
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "inlet",
									"numoutlets" : 1,
									"id" : "obj-1",
									"outlettype" : [ "bang" ],
									"patching_rect" : [ 30.0, 15.0, 25.0, 25.0 ],
									"numinlets" : 0,
									"comment" : ""
								}

							}
 ],
						"lines" : [ 							{
								"patchline" : 								{
									"source" : [ "obj-1", 0 ],
									"destination" : [ "obj-4", 0 ],
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
									"source" : [ "obj-3", 0 ],
									"destination" : [ "obj-2", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
 ]
					}
,
					"saved_object_attributes" : 					{
						"default_fontface" : 0,
						"fontface" : 0,
						"fontsize" : 12.0,
						"default_fontname" : "Arial",
						"globalpatchername" : "",
						"default_fontsize" : 12.0,
						"fontname" : "Arial"
					}

				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "Thanks to Thom Johansen, Torgeir Strand Henriksen, and Oeyvind Brandtsegg for creating partikkel.",
					"linecount" : 3,
					"numoutlets" : 0,
					"id" : "obj-3",
					"textcolor" : [ 0.65098, 0.0, 0.019608, 1.0 ],
					"fontsize" : 12.0,
					"patching_rect" : [ 824.0, 183.0, 203.0, 48.0 ],
					"numinlets" : 1,
					"fontname" : "Arial",
					"frgb" : [ 0.65098, 0.0, 0.019608, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "grain distribution (0=periodic, 1=scattered)",
					"linecount" : 4,
					"numoutlets" : 0,
					"id" : "obj-4",
					"textcolor" : [ 0.0, 0.2, 0.490196, 1.0 ],
					"fontface" : 3,
					"fontsize" : 12.0,
					"patching_rect" : [ 75.0, 34.0, 98.0, 62.0 ],
					"numinlets" : 1,
					"fontname" : "Arial",
					"frgb" : [ 0.0, 0.2, 0.490196, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "multislider",
					"numoutlets" : 2,
					"candicane2" : [ 0.145098, 0.203922, 0.356863, 1.0 ],
					"candicane7" : [ 0.878431, 0.243137, 0.145098, 1.0 ],
					"id" : "obj-5",
					"outlettype" : [ "", "" ],
					"candicane6" : [ 0.733333, 0.035294, 0.788235, 1.0 ],
					"setminmax" : [ 0.0, 100.0 ],
					"candicane5" : [ 0.584314, 0.827451, 0.431373, 1.0 ],
					"patching_rect" : [ 886.0, 437.0, 134.0, 71.0 ],
					"setstyle" : 5,
					"candicane4" : [ 0.439216, 0.619608, 0.070588, 1.0 ],
					"numinlets" : 1,
					"peakcolor" : [ 1.0, 0.098039, 0.117647, 1.0 ],
					"candicane3" : [ 0.290196, 0.411765, 0.713726, 1.0 ],
					"candicane8" : [ 0.027451, 0.447059, 0.501961, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "p cpu usage",
					"numoutlets" : 1,
					"id" : "obj-6",
					"outlettype" : [ "" ],
					"fontsize" : 12.0,
					"patching_rect" : [ 932.0, 530.0, 78.0, 20.0 ],
					"numinlets" : 1,
					"hidden" : 1,
					"fontname" : "Arial",
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
									"maxclass" : "message",
									"text" : "1",
									"numoutlets" : 1,
									"id" : "obj-1",
									"outlettype" : [ "" ],
									"fontsize" : 9.0,
									"patching_rect" : [ 35.0, 58.0, 16.0, 15.0 ],
									"numinlets" : 2,
									"fontname" : "Arial"
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "outlet",
									"numoutlets" : 0,
									"id" : "obj-2",
									"patching_rect" : [ 35.0, 126.0, 15.0, 15.0 ],
									"numinlets" : 1,
									"comment" : ""
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "inlet",
									"numoutlets" : 1,
									"id" : "obj-3",
									"outlettype" : [ "bang" ],
									"patching_rect" : [ 35.0, 40.0, 15.0, 15.0 ],
									"numinlets" : 0,
									"comment" : ""
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "metro 100",
									"numoutlets" : 1,
									"id" : "obj-4",
									"outlettype" : [ "bang" ],
									"fontsize" : 9.0,
									"patching_rect" : [ 35.0, 76.0, 58.0, 17.0 ],
									"numinlets" : 2,
									"fontname" : "Arial"
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "adstatus cpu",
									"numoutlets" : 2,
									"id" : "obj-5",
									"outlettype" : [ "", "int" ],
									"fontsize" : 9.0,
									"patching_rect" : [ 35.0, 103.0, 66.0, 17.0 ],
									"numinlets" : 2,
									"fontname" : "Arial"
								}

							}
 ],
						"lines" : [ 							{
								"patchline" : 								{
									"source" : [ "obj-3", 0 ],
									"destination" : [ "obj-1", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-1", 0 ],
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
						"default_fontface" : 0,
						"fontface" : 0,
						"fontsize" : 12.0,
						"default_fontname" : "Arial",
						"globalpatchername" : "",
						"default_fontsize" : 12.0,
						"fontname" : "Arial"
					}

				}

			}
, 			{
				"box" : 				{
					"maxclass" : "flonum",
					"numoutlets" : 2,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ],
					"id" : "obj-7",
					"outlettype" : [ "float", "bang" ],
					"fontface" : 1,
					"fontsize" : 12.0,
					"patching_rect" : [ 932.0, 509.0, 43.0, 20.0 ],
					"triscale" : 0.9,
					"numinlets" : 1,
					"fontname" : "Arial",
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "cpu",
					"numoutlets" : 0,
					"id" : "obj-8",
					"fontsize" : 12.0,
					"patching_rect" : [ 975.0, 510.0, 35.0, 20.0 ],
					"numinlets" : 1,
					"fontname" : "Arial"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "position glide time",
					"numoutlets" : 0,
					"id" : "obj-13",
					"textcolor" : [ 0.0, 0.2, 0.490196, 1.0 ],
					"fontface" : 3,
					"fontsize" : 12.0,
					"patching_rect" : [ 548.0, 139.0, 115.0, 20.0 ],
					"numinlets" : 1,
					"fontname" : "Arial",
					"frgb" : [ 0.0, 0.2, 0.490196, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "Grain Amp",
					"numoutlets" : 0,
					"id" : "obj-14",
					"textcolor" : [ 0.0, 0.2, 0.490196, 1.0 ],
					"fontface" : 3,
					"fontsize" : 12.0,
					"patching_rect" : [ 1.0, 73.0, 79.0, 20.0 ],
					"numinlets" : 1,
					"fontname" : "Arial",
					"frgb" : [ 0.0, 0.2, 0.490196, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "MIDI pitch",
					"numoutlets" : 0,
					"id" : "obj-15",
					"textcolor" : [ 0.0, 0.2, 0.490196, 1.0 ],
					"fontface" : 3,
					"fontsize" : 12.0,
					"patching_rect" : [ 149.0, 4.0, 79.0, 20.0 ],
					"numinlets" : 1,
					"fontname" : "Arial",
					"frgb" : [ 0.0, 0.2, 0.490196, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "1",
					"numoutlets" : 1,
					"id" : "obj-17",
					"outlettype" : [ "" ],
					"fontsize" : 12.0,
					"patching_rect" : [ 891.0, 19.0, 18.0, 18.0 ],
					"numinlets" : 2,
					"fontname" : "Arial"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "preset",
					"numoutlets" : 4,
					"spacing" : 2,
					"id" : "obj-20",
					"outlettype" : [ "preset", "int", "preset", "int" ],
					"bubblesize" : 10,
					"margin" : 4,
					"patching_rect" : [ 891.0, 41.0, 45.0, 21.0 ],
					"numinlets" : 1,
					"preset_data" : [ 						{
							"number" : 1,
							"data" : [ 5, "obj-80", "flonum", "float", 0.5, 5, "obj-65", "flonum", "float", 1.0, 5, "obj-63", "flonum", "float", 190.0, 5, "obj-61", "flonum", "float", 56.0, 5, "obj-59", "flonum", "float", 1.2, 5, "obj-57", "flonum", "float", 2.78, 5, "obj-53", "umenu", "int", 0, 5, "obj-51", "slider", "float", 120.0, 5, "obj-33", "flonum", "float", 69.0, 5, "obj-31", "flonum", "float", 0.06, 5, "obj-27", "flonum", "float", 1.0, 5, "obj-22", "flonum", "float", 0.01, 5, "obj-7", "flonum", "float", 27.0, 5, "obj-5", "multislider", "list", 27.0, 5, "obj-36", "flonum", "float", -12.0, 5, "obj-46", "flonum", "float", 2.0 ]
						}
 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "k_posGlide $1",
					"numoutlets" : 1,
					"id" : "obj-21",
					"outlettype" : [ "" ],
					"fontsize" : 12.0,
					"patching_rect" : [ 489.0, 161.0, 95.0, 18.0 ],
					"numinlets" : 2,
					"fontname" : "Arial"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "flonum",
					"varname" : "k_posGlide",
					"minimum" : 0.01,
					"numoutlets" : 2,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ],
					"id" : "obj-22",
					"maximum" : 8.0,
					"outlettype" : [ "float", "bang" ],
					"fontsize" : 12.0,
					"patching_rect" : [ 489.0, 140.0, 59.0, 20.0 ],
					"triscale" : 0.9,
					"numinlets" : 1,
					"fontname" : "Arial",
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "ezdac~",
					"ongradcolor2" : [ 1.0, 0.54902, 0.0, 1.0 ],
					"numoutlets" : 0,
					"id" : "obj-25",
					"patching_rect" : [ 681.0, 307.0, 65.0, 65.0 ],
					"numinlets" : 2,
					"offgradcolor1" : [ 0.0, 0.0, 0.0, 1.0 ],
					"ongradcolor1" : [ 1.0, 0.0, 0.0, 1.0 ],
					"bgcolor" : [ 0.0, 0.0, 0.0, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "k_g_amp $1",
					"numoutlets" : 1,
					"id" : "obj-26",
					"outlettype" : [ "" ],
					"fontsize" : 12.0,
					"patching_rect" : [ 2.0, 113.0, 78.0, 18.0 ],
					"numinlets" : 2,
					"fontname" : "Arial"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "flonum",
					"varname" : "k_g_amp",
					"minimum" : 0.0,
					"numoutlets" : 2,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ],
					"id" : "obj-27",
					"maximum" : 1.0,
					"outlettype" : [ "float", "bang" ],
					"fontsize" : 12.0,
					"patching_rect" : [ 2.0, 91.0, 50.0, 20.0 ],
					"triscale" : 0.9,
					"numinlets" : 1,
					"fontname" : "Arial",
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "grain window",
					"numoutlets" : 0,
					"id" : "obj-28",
					"textcolor" : [ 0.0, 0.2, 0.490196, 1.0 ],
					"fontface" : 3,
					"fontsize" : 12.0,
					"patching_rect" : [ 12.0, 158.0, 91.0, 20.0 ],
					"numinlets" : 1,
					"fontname" : "Arial",
					"frgb" : [ 0.0, 0.2, 0.490196, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "pitch glide time",
					"numoutlets" : 0,
					"id" : "obj-29",
					"textcolor" : [ 0.0, 0.2, 0.490196, 1.0 ],
					"fontface" : 3,
					"fontsize" : 12.0,
					"patching_rect" : [ 225.0, 4.0, 116.0, 20.0 ],
					"numinlets" : 1,
					"fontname" : "Arial",
					"frgb" : [ 0.0, 0.2, 0.490196, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "k_port $1",
					"numoutlets" : 1,
					"id" : "obj-30",
					"outlettype" : [ "" ],
					"fontsize" : 12.0,
					"patching_rect" : [ 236.0, 43.0, 71.0, 18.0 ],
					"numinlets" : 2,
					"fontname" : "Arial"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "flonum",
					"varname" : "k_port",
					"minimum" : 0.0,
					"numoutlets" : 2,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ],
					"id" : "obj-31",
					"maximum" : 8.0,
					"outlettype" : [ "float", "bang" ],
					"fontsize" : 12.0,
					"patching_rect" : [ 236.0, 22.0, 61.0, 20.0 ],
					"triscale" : 0.9,
					"numinlets" : 1,
					"fontname" : "Arial",
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"varname" : "load_file",
					"text" : "p load_file",
					"numoutlets" : 1,
					"id" : "obj-32",
					"outlettype" : [ "" ],
					"fontface" : 3,
					"fontsize" : 14.0,
					"color" : [ 1.0, 1.0, 1.0, 1.0 ],
					"patching_rect" : [ 121.0, 365.0, 81.0, 23.0 ],
					"numinlets" : 2,
					"fontname" : "Arial",
					"bgcolor" : [ 0.760784, 0.866667, 1.0, 1.0 ],
					"patcher" : 					{
						"fileversion" : 1,
						"rect" : [ 384.0, 326.0, 879.0, 395.0 ],
						"bglocked" : 0,
						"defrect" : [ 384.0, 326.0, 879.0, 395.0 ],
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
									"text" : "pattrforward parent::convert::file_length",
									"numoutlets" : 1,
									"id" : "obj-23",
									"outlettype" : [ "" ],
									"fontsize" : 12.0,
									"patching_rect" : [ 4.0, 167.0, 219.0, 20.0 ],
									"numinlets" : 1,
									"fontname" : "Arial"
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "p \"Get File Length in Seconds\"",
									"numoutlets" : 1,
									"id" : "obj-22",
									"outlettype" : [ "float" ],
									"fontsize" : 12.0,
									"patching_rect" : [ 4.0, 142.0, 174.0, 20.0 ],
									"numinlets" : 1,
									"fontname" : "Arial",
									"patcher" : 									{
										"fileversion" : 1,
										"rect" : [ 399.0, 544.0, 178.0, 200.0 ],
										"bglocked" : 0,
										"defrect" : [ 399.0, 544.0, 178.0, 200.0 ],
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
													"numoutlets" : 0,
													"id" : "obj-1",
													"patching_rect" : [ 79.0, 149.0, 28.0, 28.0 ],
													"numinlets" : 1,
													"comment" : ""
												}

											}
, 											{
												"box" : 												{
													"maxclass" : "inlet",
													"numoutlets" : 1,
													"id" : "obj-2",
													"outlettype" : [ "" ],
													"patching_rect" : [ 30.0, 15.0, 28.0, 28.0 ],
													"numinlets" : 0,
													"comment" : ""
												}

											}
, 											{
												"box" : 												{
													"maxclass" : "newobj",
													"text" : "* 0.001",
													"numoutlets" : 1,
													"id" : "obj-3",
													"outlettype" : [ "float" ],
													"fontsize" : 12.0,
													"patching_rect" : [ 79.0, 119.0, 70.0, 20.0 ],
													"numinlets" : 2,
													"fontname" : "Arial"
												}

											}
, 											{
												"box" : 												{
													"maxclass" : "newobj",
													"text" : "sfinfo~",
													"numoutlets" : 6,
													"id" : "obj-4",
													"outlettype" : [ "int", "int", "float", "float", "", "" ],
													"fontsize" : 12.0,
													"patching_rect" : [ 30.0, 90.0, 100.0, 20.0 ],
													"numinlets" : 1,
													"fontname" : "Arial"
												}

											}
, 											{
												"box" : 												{
													"maxclass" : "newobj",
													"text" : "prepend open",
													"numoutlets" : 1,
													"id" : "obj-5",
													"outlettype" : [ "" ],
													"fontsize" : 12.0,
													"patching_rect" : [ 30.0, 60.0, 89.0, 20.0 ],
													"numinlets" : 1,
													"fontname" : "Arial"
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
										"default_fontface" : 0,
										"fontface" : 0,
										"fontsize" : 12.0,
										"default_fontname" : "Arial",
										"globalpatchername" : "",
										"default_fontsize" : 12.0,
										"fontname" : "Arial"
									}

								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "By the way, loadsamp operations that don't replace tables always finish immediately. ",
									"linecount" : 2,
									"numoutlets" : 0,
									"id" : "obj-18",
									"textcolor" : [ 0.0, 0.196078, 0.490196, 1.0 ],
									"fontsize" : 12.0,
									"patching_rect" : [ 575.0, 126.0, 290.0, 34.0 ],
									"numinlets" : 1,
									"fontname" : "Arial",
									"frgb" : [ 0.0, 0.196078, 0.490196, 1.0 ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "When using loadsamp to replace a table, there is no way to know when it's finished.  That's why we need to delay the activation of instr #1.",
									"linecount" : 3,
									"numoutlets" : 0,
									"id" : "obj-16",
									"textcolor" : [ 0.0, 0.196078, 0.490196, 1.0 ],
									"fontsize" : 12.0,
									"patching_rect" : [ 575.0, 74.0, 280.0, 48.0 ],
									"numinlets" : 1,
									"fontname" : "Arial",
									"frgb" : [ 0.0, 0.196078, 0.490196, 1.0 ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "* 1000.",
									"numoutlets" : 1,
									"id" : "obj-4",
									"outlettype" : [ "float" ],
									"fontsize" : 12.0,
									"patching_rect" : [ 371.0, 51.0, 49.0, 20.0 ],
									"numinlets" : 2,
									"fontname" : "Arial"
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "Filename",
									"numoutlets" : 0,
									"id" : "obj-15",
									"fontsize" : 12.0,
									"patching_rect" : [ 167.0, 17.0, 61.0, 20.0 ],
									"numinlets" : 1,
									"fontname" : "Arial"
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "Activation Delay Time",
									"numoutlets" : 0,
									"id" : "obj-14",
									"fontsize" : 12.0,
									"patching_rect" : [ 396.0, 18.0, 150.0, 20.0 ],
									"numinlets" : 1,
									"fontname" : "Arial"
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "inlet",
									"numoutlets" : 1,
									"id" : "obj-13",
									"outlettype" : [ "float" ],
									"patching_rect" : [ 371.0, 15.0, 25.0, 25.0 ],
									"numinlets" : 0,
									"comment" : ""
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "delay 1000",
									"numoutlets" : 1,
									"id" : "obj-9",
									"outlettype" : [ "bang" ],
									"fontsize" : 12.0,
									"patching_rect" : [ 321.0, 82.0, 69.0, 20.0 ],
									"numinlets" : 2,
									"fontname" : "Arial"
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "pattrforward parent::csIN",
									"numoutlets" : 1,
									"id" : "obj-7",
									"outlettype" : [ "" ],
									"fontsize" : 12.0,
									"patching_rect" : [ 255.0, 147.0, 143.0, 20.0 ],
									"numinlets" : 1,
									"fontname" : "Arial"
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "Activate instr #1",
									"numoutlets" : 0,
									"id" : "obj-12",
									"textcolor" : [ 0.0, 0.196078, 0.490196, 1.0 ],
									"fontsize" : 12.0,
									"patching_rect" : [ 377.0, 108.0, 99.0, 20.0 ],
									"numinlets" : 1,
									"fontname" : "Arial",
									"frgb" : [ 0.0, 0.196078, 0.490196, 1.0 ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "Turn off instr #1",
									"numoutlets" : 0,
									"id" : "obj-11",
									"textcolor" : [ 0.0, 0.196078, 0.490196, 1.0 ],
									"fontsize" : 12.0,
									"patching_rect" : [ 160.0, 109.0, 99.0, 20.0 ],
									"numinlets" : 1,
									"fontname" : "Arial",
									"frgb" : [ 0.0, 0.196078, 0.490196, 1.0 ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "Prepare message so that table #1 is replaced with the left channel of the audio file.",
									"linecount" : 2,
									"numoutlets" : 0,
									"id" : "obj-8",
									"textcolor" : [ 0.0, 0.196078, 0.490196, 1.0 ],
									"fontsize" : 12.0,
									"patching_rect" : [ 400.0, 181.0, 256.0, 34.0 ],
									"numinlets" : 1,
									"fontname" : "Arial",
									"frgb" : [ 0.0, 0.196078, 0.490196, 1.0 ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "The \"loadsamp\" message is delayed in order to give the \"e i2 0 0.1\" event enough time to take effect.  Otherwise, we may replace a table that's currently in use.  \n\nInstr #2 contains the turnoff2 opcode.  When we send the event, [csound~] needs at least one Csound signal vector's worth of time to process that event.  200ms should be enough time for ksmps <= 8820 @ sr = 44100.",
									"linecount" : 7,
									"numoutlets" : 0,
									"id" : "obj-17",
									"textcolor" : [ 0.0, 0.196078, 0.490196, 1.0 ],
									"fontsize" : 12.0,
									"patching_rect" : [ 360.0, 222.0, 429.0, 103.0 ],
									"numinlets" : 1,
									"fontname" : "Arial",
									"frgb" : [ 0.0, 0.196078, 0.490196, 1.0 ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "p \"delay by 200ms\"",
									"numoutlets" : 1,
									"id" : "obj-19",
									"outlettype" : [ "" ],
									"fontsize" : 12.0,
									"patching_rect" : [ 240.0, 246.0, 113.0, 20.0 ],
									"numinlets" : 1,
									"fontname" : "Arial",
									"patcher" : 									{
										"fileversion" : 1,
										"rect" : [ 168.0, 176.0, 245.0, 292.0 ],
										"bglocked" : 0,
										"defrect" : [ 168.0, 176.0, 245.0, 292.0 ],
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
													"numoutlets" : 2,
													"id" : "obj-22",
													"outlettype" : [ "bang", "" ],
													"fontsize" : 12.0,
													"patching_rect" : [ 50.0, 100.0, 93.5, 20.0 ],
													"numinlets" : 1,
													"fontname" : "Arial"
												}

											}
, 											{
												"box" : 												{
													"maxclass" : "newobj",
													"text" : "prepend set",
													"numoutlets" : 1,
													"id" : "obj-20",
													"outlettype" : [ "" ],
													"fontsize" : 12.0,
													"patching_rect" : [ 125.0, 130.0, 74.0, 20.0 ],
													"numinlets" : 1,
													"fontname" : "Arial"
												}

											}
, 											{
												"box" : 												{
													"maxclass" : "newobj",
													"text" : "delay 200",
													"numoutlets" : 1,
													"id" : "obj-11",
													"outlettype" : [ "bang" ],
													"fontsize" : 12.0,
													"patching_rect" : [ 50.0, 130.0, 63.0, 20.0 ],
													"numinlets" : 2,
													"fontname" : "Arial"
												}

											}
, 											{
												"box" : 												{
													"maxclass" : "inlet",
													"numoutlets" : 1,
													"id" : "obj-16",
													"outlettype" : [ "" ],
													"patching_rect" : [ 50.0, 40.0, 25.0, 25.0 ],
													"numinlets" : 0,
													"comment" : ""
												}

											}
, 											{
												"box" : 												{
													"maxclass" : "outlet",
													"numoutlets" : 0,
													"id" : "obj-18",
													"patching_rect" : [ 82.5, 224.0, 25.0, 25.0 ],
													"numinlets" : 1,
													"comment" : ""
												}

											}
 ],
										"lines" : [ 											{
												"patchline" : 												{
													"source" : [ "obj-22", 1 ],
													"destination" : [ "obj-20", 0 ],
													"hidden" : 0,
													"midpoints" : [  ]
												}

											}
, 											{
												"patchline" : 												{
													"source" : [ "obj-16", 0 ],
													"destination" : [ "obj-22", 0 ],
													"hidden" : 0,
													"midpoints" : [  ]
												}

											}
, 											{
												"patchline" : 												{
													"source" : [ "obj-20", 0 ],
													"destination" : [ "obj-18", 0 ],
													"hidden" : 0,
													"midpoints" : [  ]
												}

											}
, 											{
												"patchline" : 												{
													"source" : [ "obj-22", 0 ],
													"destination" : [ "obj-11", 0 ],
													"hidden" : 0,
													"midpoints" : [  ]
												}

											}
, 											{
												"patchline" : 												{
													"source" : [ "obj-11", 0 ],
													"destination" : [ "obj-18", 0 ],
													"hidden" : 0,
													"midpoints" : [  ]
												}

											}
 ]
									}
,
									"saved_object_attributes" : 									{
										"default_fontface" : 0,
										"fontface" : 0,
										"fontsize" : 12.0,
										"default_fontname" : "Arial",
										"globalpatchername" : "",
										"default_fontsize" : 12.0,
										"fontname" : "Arial"
									}

								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "loadsamp -1 1 \"H:/samples/07 May Tricks 175bpm Bmin/vox.wav\"",
									"numoutlets" : 1,
									"id" : "obj-21",
									"outlettype" : [ "" ],
									"fontsize" : 12.0,
									"patching_rect" : [ 240.0, 331.0, 610.0, 18.0 ],
									"numinlets" : 2,
									"fontname" : "Arial"
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "pattrforward parent::csIN",
									"numoutlets" : 1,
									"id" : "obj-6",
									"outlettype" : [ "" ],
									"fontsize" : 12.0,
									"patching_rect" : [ 240.0, 367.0, 143.0, 20.0 ],
									"numinlets" : 1,
									"fontname" : "Arial"
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "When this patch receives a filename, instr #1 is turned off and the loadsamp message is built.  After 200ms, the loadsamp message is sent. After another 1000ms, the istr #1 is activated.",
									"linecount" : 4,
									"numoutlets" : 0,
									"id" : "obj-36",
									"textcolor" : [ 0.0, 0.196078, 0.490196, 1.0 ],
									"fontsize" : 12.0,
									"patching_rect" : [ 575.0, 8.0, 266.0, 62.0 ],
									"numinlets" : 1,
									"fontname" : "Arial",
									"frgb" : [ 0.0, 0.196078, 0.490196, 1.0 ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "t b l b",
									"numoutlets" : 3,
									"id" : "obj-10",
									"outlettype" : [ "bang", "", "bang" ],
									"fontsize" : 12.0,
									"patching_rect" : [ 225.0, 49.0, 49.0, 20.0 ],
									"numinlets" : 1,
									"fontname" : "Arial"
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "e i2 0 0.1",
									"numoutlets" : 1,
									"id" : "obj-90",
									"outlettype" : [ "" ],
									"fontsize" : 12.0,
									"patching_rect" : [ 255.0, 110.0, 60.0, 18.0 ],
									"numinlets" : 2,
									"fontname" : "Arial"
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "message",
									"text" : "e i1 0 -1",
									"numoutlets" : 1,
									"id" : "obj-89",
									"outlettype" : [ "" ],
									"fontsize" : 12.0,
									"patching_rect" : [ 321.0, 110.0, 54.0, 18.0 ],
									"numinlets" : 2,
									"fontname" : "Arial"
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "outlet",
									"numoutlets" : 0,
									"id" : "obj-1",
									"patching_rect" : [ 37.0, 94.0, 25.0, 25.0 ],
									"numinlets" : 1,
									"comment" : ""
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "prepend set",
									"numoutlets" : 1,
									"id" : "obj-2",
									"outlettype" : [ "" ],
									"fontsize" : 12.0,
									"patching_rect" : [ 37.0, 72.0, 80.0, 20.0 ],
									"numinlets" : 1,
									"fontname" : "Arial"
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "sprintf loadsamp -1 1 \\\"%s\\\"",
									"numoutlets" : 1,
									"id" : "obj-3",
									"outlettype" : [ "" ],
									"fontsize" : 12.0,
									"patching_rect" : [ 240.0, 187.0, 160.0, 20.0 ],
									"numinlets" : 1,
									"fontname" : "Arial"
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "inlet",
									"numoutlets" : 1,
									"id" : "obj-5",
									"outlettype" : [ "" ],
									"patching_rect" : [ 225.0, 15.0, 25.0, 25.0 ],
									"numinlets" : 0,
									"comment" : ""
								}

							}
 ],
						"lines" : [ 							{
								"patchline" : 								{
									"source" : [ "obj-9", 0 ],
									"destination" : [ "obj-89", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-89", 0 ],
									"destination" : [ "obj-7", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-90", 0 ],
									"destination" : [ "obj-7", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-10", 1 ],
									"destination" : [ "obj-3", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-10", 2 ],
									"destination" : [ "obj-90", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-5", 0 ],
									"destination" : [ "obj-10", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-3", 0 ],
									"destination" : [ "obj-19", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-19", 0 ],
									"destination" : [ "obj-21", 0 ],
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
									"source" : [ "obj-21", 0 ],
									"destination" : [ "obj-6", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-4", 0 ],
									"destination" : [ "obj-9", 1 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-13", 0 ],
									"destination" : [ "obj-4", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-22", 0 ],
									"destination" : [ "obj-23", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-5", 0 ],
									"destination" : [ "obj-2", 0 ],
									"hidden" : 0,
									"midpoints" : [ 234.5, 43.0, 46.5, 43.0 ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-10", 0 ],
									"destination" : [ "obj-9", 0 ],
									"hidden" : 0,
									"midpoints" : [ 234.5, 76.5, 330.5, 76.5 ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-5", 0 ],
									"destination" : [ "obj-22", 0 ],
									"hidden" : 0,
									"midpoints" : [ 234.5, 43.5, 13.5, 43.5 ]
								}

							}
 ]
					}
,
					"saved_object_attributes" : 					{
						"default_fontface" : 0,
						"fontface" : 0,
						"fontsize" : 12.0,
						"default_fontname" : "Arial",
						"globalpatchername" : "",
						"default_fontsize" : 12.0,
						"fontname" : "Arial"
					}

				}

			}
, 			{
				"box" : 				{
					"maxclass" : "flonum",
					"varname" : "midi_trans",
					"minimum" : 0.0,
					"numoutlets" : 2,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ],
					"id" : "obj-33",
					"maximum" : 127.0,
					"outlettype" : [ "float", "bang" ],
					"fontsize" : 12.0,
					"patching_rect" : [ 155.0, 22.0, 40.0, 20.0 ],
					"triscale" : 0.9,
					"numinlets" : 1,
					"fontname" : "Arial",
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "/ 440.",
					"numoutlets" : 1,
					"id" : "obj-34",
					"outlettype" : [ "float" ],
					"fontsize" : 12.0,
					"patching_rect" : [ 155.0, 64.0, 51.0, 20.0 ],
					"numinlets" : 2,
					"fontname" : "Arial"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "mtof",
					"numoutlets" : 1,
					"id" : "obj-35",
					"outlettype" : [ "" ],
					"fontsize" : 12.0,
					"patching_rect" : [ 155.0, 43.0, 39.0, 20.0 ],
					"numinlets" : 1,
					"fontname" : "Arial"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "Partikkel",
					"numoutlets" : 0,
					"id" : "obj-38",
					"textcolor" : [ 0.337255, 0.254902, 0.113725, 1.0 ],
					"fontsize" : 36.0,
					"patching_rect" : [ 824.0, 117.0, 169.0, 48.0 ],
					"numinlets" : 1,
					"fontname" : "Arial",
					"frgb" : [ 0.337255, 0.254902, 0.113725, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "Open File ...",
					"numoutlets" : 0,
					"id" : "obj-39",
					"textcolor" : [ 0.0, 0.2, 0.490196, 1.0 ],
					"fontface" : 3,
					"fontsize" : 12.0,
					"patching_rect" : [ 116.0, 255.0, 83.0, 20.0 ],
					"numinlets" : 1,
					"fontname" : "Arial",
					"frgb" : [ 0.0, 0.2, 0.490196, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "button",
					"numoutlets" : 1,
					"outlinecolor" : [ 0.0, 0.0, 0.0, 1.0 ],
					"id" : "obj-41",
					"outlettype" : [ "bang" ],
					"patching_rect" : [ 121.0, 272.0, 66.0, 66.0 ],
					"numinlets" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "\"H:/samples/07 May Tricks 175bpm Bmin/vox.wav\"",
					"numoutlets" : 1,
					"id" : "obj-44",
					"outlettype" : [ "" ],
					"fontface" : 1,
					"fontsize" : 12.0,
					"patching_rect" : [ 30.0, 397.0, 790.0, 18.0 ],
					"numinlets" : 2,
					"fontname" : "Arial",
					"bgcolor" : [ 0.980392, 1.0, 0.627451, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "grains / sec",
					"numoutlets" : 0,
					"id" : "obj-48",
					"textcolor" : [ 0.0, 0.2, 0.490196, 1.0 ],
					"fontface" : 3,
					"fontsize" : 12.0,
					"patching_rect" : [ 315.0, 73.0, 78.0, 20.0 ],
					"numinlets" : 1,
					"fontname" : "Arial",
					"frgb" : [ 0.0, 0.2, 0.490196, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "scope~",
					"numoutlets" : 0,
					"id" : "obj-49",
					"patching_rect" : [ 459.0, 239.0, 214.0, 133.0 ],
					"range" : [ -2.0, 2.0 ],
					"bufsize" : 64,
					"calccount" : 32,
					"numinlets" : 2,
					"rounded" : 0,
					"bgcolor" : [ 0.0, 0.0, 0.0, 1.0 ],
					"fgcolor" : [ 0.678431, 1.0, 0.780392, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"varname" : "convert",
					"text" : "p convert",
					"numoutlets" : 1,
					"id" : "obj-50",
					"outlettype" : [ "float" ],
					"fontsize" : 12.0,
					"color" : [ 1.0, 1.0, 1.0, 1.0 ],
					"patching_rect" : [ 488.0, 66.0, 60.0, 20.0 ],
					"numinlets" : 1,
					"fontname" : "Arial",
					"bgcolor" : [ 0.760784, 0.870588, 1.0, 1.0 ],
					"patcher" : 					{
						"fileversion" : 1,
						"rect" : [ 740.0, 116.0, 345.0, 194.0 ],
						"bglocked" : 0,
						"defrect" : [ 740.0, 116.0, 345.0, 194.0 ],
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
									"text" : "This number is set in the [load_file] sub-patch of the main patch.",
									"linecount" : 3,
									"numoutlets" : 0,
									"id" : "obj-12",
									"fontsize" : 12.0,
									"patching_rect" : [ 167.0, 50.0, 159.0, 48.0 ],
									"numinlets" : 1,
									"fontname" : "Arial"
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "* 1.",
									"numoutlets" : 1,
									"id" : "obj-10",
									"outlettype" : [ "float" ],
									"fontsize" : 12.0,
									"patching_rect" : [ 28.0, 127.0, 32.5, 20.0 ],
									"numinlets" : 2,
									"fontname" : "Arial"
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "t b f",
									"numoutlets" : 2,
									"id" : "obj-9",
									"outlettype" : [ "bang", "float" ],
									"fontsize" : 12.0,
									"patching_rect" : [ 88.0, 82.0, 32.5, 20.0 ],
									"numinlets" : 1,
									"fontname" : "Arial"
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "comment",
									"text" : "file_length",
									"numoutlets" : 0,
									"id" : "obj-8",
									"textcolor" : [ 0.866667, 0.027451, 0.027451, 1.0 ],
									"fontsize" : 12.0,
									"patching_rect" : [ 96.0, 31.0, 65.0, 20.0 ],
									"numinlets" : 1,
									"fontname" : "Arial",
									"frgb" : [ 0.866667, 0.027451, 0.027451, 1.0 ]
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "flonum",
									"varname" : "file_length",
									"numoutlets" : 2,
									"id" : "obj-6",
									"outlettype" : [ "float", "bang" ],
									"fontsize" : 12.0,
									"patching_rect" : [ 88.0, 52.0, 77.0, 20.0 ],
									"numinlets" : 1,
									"fontname" : "Arial"
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "newobj",
									"text" : "* 0.001",
									"numoutlets" : 1,
									"id" : "obj-3",
									"outlettype" : [ "float" ],
									"fontsize" : 12.0,
									"patching_rect" : [ 13.0, 52.0, 49.0, 20.0 ],
									"numinlets" : 2,
									"fontname" : "Arial"
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "outlet",
									"numoutlets" : 0,
									"id" : "obj-2",
									"patching_rect" : [ 28.0, 155.0, 25.0, 25.0 ],
									"numinlets" : 1,
									"comment" : ""
								}

							}
, 							{
								"box" : 								{
									"maxclass" : "inlet",
									"numoutlets" : 1,
									"id" : "obj-1",
									"outlettype" : [ "" ],
									"patching_rect" : [ 13.0, 7.0, 25.0, 25.0 ],
									"numinlets" : 0,
									"comment" : ""
								}

							}
 ],
						"lines" : [ 							{
								"patchline" : 								{
									"source" : [ "obj-1", 0 ],
									"destination" : [ "obj-3", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-6", 0 ],
									"destination" : [ "obj-9", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-3", 0 ],
									"destination" : [ "obj-10", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-9", 0 ],
									"destination" : [ "obj-10", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-9", 1 ],
									"destination" : [ "obj-10", 1 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
, 							{
								"patchline" : 								{
									"source" : [ "obj-10", 0 ],
									"destination" : [ "obj-2", 0 ],
									"hidden" : 0,
									"midpoints" : [  ]
								}

							}
 ]
					}
,
					"saved_object_attributes" : 					{
						"default_fontface" : 0,
						"fontface" : 0,
						"fontsize" : 12.0,
						"default_fontname" : "Arial",
						"globalpatchername" : "",
						"default_fontsize" : 12.0,
						"fontname" : "Arial"
					}

				}

			}
, 			{
				"box" : 				{
					"maxclass" : "slider",
					"varname" : "k_pos",
					"numoutlets" : 1,
					"id" : "obj-51",
					"outlettype" : [ "" ],
					"size" : 1001.0,
					"patching_rect" : [ 488.0, 36.0, 286.0, 27.0 ],
					"numinlets" : 1,
					"orientation" : 1
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "umenu",
					"varname" : "i_win",
					"numoutlets" : 3,
					"id" : "obj-53",
					"outlettype" : [ "int", "", "" ],
					"fontsize" : 12.0,
					"patching_rect" : [ 5.0, 178.0, 99.0, 20.0 ],
					"labelclick" : 1,
					"items" : [ "Hamming", ",", "Hanning", ",", "Bartlett", ",", "Blackman", ",", "BlackHar", ",", "Gaussian", ",", "Sync", ",", "Perc", "(lin)", ",", "Perc", "(exp)", ",", "Gate", ",", "RevPerc", "(lin)", ",", "RevPerc", "(exp)" ],
					"numinlets" : 1,
					"fontname" : "Arial",
					"types" : [  ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "k_posRand $1",
					"numoutlets" : 1,
					"id" : "obj-56",
					"outlettype" : [ "" ],
					"fontsize" : 12.0,
					"patching_rect" : [ 401.0, 113.0, 89.0, 18.0 ],
					"numinlets" : 2,
					"fontname" : "Arial"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "flonum",
					"varname" : "k_posRand",
					"minimum" : 0.0,
					"numoutlets" : 2,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ],
					"id" : "obj-57",
					"outlettype" : [ "float", "bang" ],
					"fontsize" : 12.0,
					"patching_rect" : [ 401.0, 91.0, 52.0, 20.0 ],
					"triscale" : 0.9,
					"numinlets" : 1,
					"fontname" : "Arial",
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "k_pos $1",
					"numoutlets" : 1,
					"id" : "obj-58",
					"outlettype" : [ "" ],
					"fontsize" : 12.0,
					"patching_rect" : [ 488.0, 113.0, 59.0, 18.0 ],
					"numinlets" : 2,
					"fontname" : "Arial"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "flonum",
					"minimum" : 0.0,
					"numoutlets" : 2,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ],
					"id" : "obj-59",
					"outlettype" : [ "float", "bang" ],
					"fontsize" : 12.0,
					"patching_rect" : [ 488.0, 91.0, 62.0, 20.0 ],
					"triscale" : 0.9,
					"numinlets" : 1,
					"fontname" : "Arial",
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "k_g_rate $1",
					"numoutlets" : 1,
					"id" : "obj-60",
					"outlettype" : [ "" ],
					"fontsize" : 12.0,
					"patching_rect" : [ 322.0, 113.0, 78.0, 18.0 ],
					"numinlets" : 2,
					"fontname" : "Arial"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "flonum",
					"varname" : "k_g_rate",
					"minimum" : 1.0,
					"numoutlets" : 2,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ],
					"id" : "obj-61",
					"maximum" : 500.0,
					"outlettype" : [ "float", "bang" ],
					"fontsize" : 12.0,
					"patching_rect" : [ 322.0, 91.0, 50.0, 20.0 ],
					"triscale" : 0.9,
					"numinlets" : 1,
					"fontname" : "Arial",
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "k_g_size $1",
					"numoutlets" : 1,
					"id" : "obj-62",
					"outlettype" : [ "" ],
					"fontsize" : 12.0,
					"patching_rect" : [ 240.0, 113.0, 77.0, 18.0 ],
					"numinlets" : 2,
					"fontname" : "Arial"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "flonum",
					"varname" : "k_g_size",
					"minimum" : 0.001,
					"numoutlets" : 2,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ],
					"id" : "obj-63",
					"outlettype" : [ "float", "bang" ],
					"fontsize" : 12.0,
					"patching_rect" : [ 240.0, 91.0, 52.0, 20.0 ],
					"triscale" : 0.9,
					"numinlets" : 1,
					"fontname" : "Arial",
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "k_g_trans $1",
					"numoutlets" : 1,
					"id" : "obj-64",
					"outlettype" : [ "" ],
					"fontsize" : 12.0,
					"patching_rect" : [ 155.0, 113.0, 84.0, 18.0 ],
					"numinlets" : 2,
					"fontname" : "Arial"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "flonum",
					"minimum" : 0.001,
					"numoutlets" : 2,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ],
					"id" : "obj-65",
					"outlettype" : [ "float", "bang" ],
					"fontsize" : 12.0,
					"patching_rect" : [ 155.0, 91.0, 50.0, 20.0 ],
					"triscale" : 0.9,
					"numinlets" : 1,
					"fontname" : "Arial",
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "pvar csIN",
					"numoutlets" : 1,
					"id" : "obj-66",
					"outlettype" : [ "" ],
					"fontsize" : 12.0,
					"patching_rect" : [ 774.0, 93.0, 67.0, 20.0 ],
					"numinlets" : 1,
					"hidden" : 1,
					"fontname" : "Arial"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "message 1",
					"numoutlets" : 1,
					"id" : "obj-67",
					"outlettype" : [ "" ],
					"fontsize" : 12.0,
					"patching_rect" : [ 955.0, 69.0, 71.0, 18.0 ],
					"numinlets" : 2,
					"fontname" : "Arial",
					"bgcolor" : [ 1.0, 0.890196, 0.090196, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "pattrforward cIN",
					"numoutlets" : 1,
					"id" : "obj-68",
					"outlettype" : [ "" ],
					"fontsize" : 12.0,
					"patching_rect" : [ 69.0, 136.0, 96.0, 20.0 ],
					"numinlets" : 1,
					"hidden" : 1,
					"fontname" : "Arial"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "prepend c",
					"numoutlets" : 1,
					"id" : "obj-69",
					"outlettype" : [ "" ],
					"fontsize" : 12.0,
					"patching_rect" : [ 706.0, 189.0, 68.0, 20.0 ],
					"numinlets" : 1,
					"fontname" : "Arial"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"varname" : "csIN",
					"text" : "t l",
					"numoutlets" : 1,
					"id" : "obj-70",
					"outlettype" : [ "" ],
					"fontsize" : 12.0,
					"patching_rect" : [ 681.0, 166.0, 21.0, 20.0 ],
					"numinlets" : 1,
					"fontname" : "Arial"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "csIN",
					"numoutlets" : 0,
					"id" : "obj-71",
					"textcolor" : [ 0.909804, 0.0, 0.0, 1.0 ],
					"fontsize" : 12.0,
					"patching_rect" : [ 650.0, 166.0, 33.0, 20.0 ],
					"numinlets" : 1,
					"fontname" : "Arial",
					"frgb" : [ 0.909804, 0.0, 0.0, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"varname" : "cIN",
					"text" : "t l",
					"numoutlets" : 1,
					"id" : "obj-72",
					"outlettype" : [ "" ],
					"fontsize" : 12.0,
					"patching_rect" : [ 706.0, 166.0, 21.0, 20.0 ],
					"numinlets" : 1,
					"fontname" : "Arial"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "t b b b b",
					"numoutlets" : 4,
					"id" : "obj-73",
					"outlettype" : [ "bang", "bang", "bang", "bang" ],
					"fontsize" : 12.0,
					"patching_rect" : [ 808.0, 40.0, 72.0, 20.0 ],
					"numinlets" : 1,
					"hidden" : 1,
					"fontname" : "Arial"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "k_g_dist $1",
					"numoutlets" : 1,
					"id" : "obj-79",
					"outlettype" : [ "" ],
					"fontsize" : 12.0,
					"patching_rect" : [ 81.0, 113.0, 72.0, 18.0 ],
					"numinlets" : 2,
					"fontname" : "Arial"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "flonum",
					"varname" : "i_g_dist",
					"minimum" : 0.0,
					"numoutlets" : 2,
					"htextcolor" : [ 0.870588, 0.870588, 0.870588, 1.0 ],
					"id" : "obj-80",
					"maximum" : 1.0,
					"outlettype" : [ "float", "bang" ],
					"fontsize" : 12.0,
					"patching_rect" : [ 81.0, 91.0, 50.0, 20.0 ],
					"triscale" : 0.9,
					"numinlets" : 1,
					"fontname" : "Arial",
					"bgcolor" : [ 0.866667, 0.866667, 0.866667, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "button",
					"numoutlets" : 1,
					"id" : "obj-81",
					"outlettype" : [ "bang" ],
					"patching_rect" : [ 770.0, 69.0, 18.0, 18.0 ],
					"numinlets" : 1,
					"fgcolor" : [ 0.156863, 0.8, 0.54902, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "stop",
					"numoutlets" : 1,
					"id" : "obj-82",
					"outlettype" : [ "" ],
					"fontsize" : 12.0,
					"patching_rect" : [ 790.0, 69.0, 35.0, 18.0 ],
					"numinlets" : 2,
					"fontname" : "Arial"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "message",
					"text" : "csound partikkel2.csd",
					"numoutlets" : 1,
					"id" : "obj-83",
					"outlettype" : [ "" ],
					"fontsize" : 12.0,
					"patching_rect" : [ 826.0, 69.0, 128.0, 18.0 ],
					"numinlets" : 2,
					"fontname" : "Arial",
					"bgcolor" : [ 1.0, 0.890196, 0.090196, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "newobj",
					"text" : "csound~",
					"numoutlets" : 6,
					"id" : "obj-84",
					"outlettype" : [ "signal", "signal", "list", "int", "bang", "bang" ],
					"fontsize" : 12.0,
					"patching_rect" : [ 681.0, 220.0, 109.0, 20.0 ],
					"numinlets" : 2,
					"fontname" : "Arial"
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "cIN",
					"numoutlets" : 0,
					"id" : "obj-85",
					"textcolor" : [ 0.909804, 0.0, 0.0, 1.0 ],
					"fontsize" : 12.0,
					"patching_rect" : [ 726.0, 166.0, 29.0, 20.0 ],
					"numinlets" : 1,
					"fontname" : "Arial",
					"frgb" : [ 0.909804, 0.0, 0.0, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "grain size (ms)",
					"numoutlets" : 0,
					"id" : "obj-87",
					"textcolor" : [ 0.0, 0.2, 0.490196, 1.0 ],
					"fontface" : 3,
					"fontsize" : 12.0,
					"patching_rect" : [ 221.0, 73.0, 106.0, 20.0 ],
					"numinlets" : 1,
					"fontname" : "Arial",
					"frgb" : [ 0.0, 0.2, 0.490196, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "comment",
					"text" : "Drop Files Here",
					"numoutlets" : 0,
					"id" : "obj-23",
					"textcolor" : [ 0.337255, 0.254902, 0.113725, 1.0 ],
					"fontsize" : 36.0,
					"patching_rect" : [ 321.0, 460.0, 266.0, 48.0 ],
					"numinlets" : 1,
					"fontname" : "Arial",
					"frgb" : [ 0.337255, 0.254902, 0.113725, 1.0 ]
				}

			}
, 			{
				"box" : 				{
					"maxclass" : "dropfile",
					"numoutlets" : 2,
					"id" : "obj-11",
					"outlettype" : [ "", "" ],
					"patching_rect" : [ 7.0, 420.0, 853.0, 131.0 ],
					"numinlets" : 1,
					"types" : [  ]
				}

			}
 ],
		"lines" : [ 			{
				"patchline" : 				{
					"source" : [ "obj-37", 0 ],
					"destination" : [ "obj-44", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-21", 0 ],
					"destination" : [ "obj-68", 0 ],
					"hidden" : 1,
					"midpoints" : [ 498.5, 183.0, 371.5, 183.0, 371.5, 132.0, 78.5, 132.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-69", 0 ],
					"destination" : [ "obj-84", 0 ],
					"hidden" : 0,
					"midpoints" : [ 715.5, 214.0, 690.5, 214.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-18", 0 ],
					"destination" : [ "obj-10", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-54", 0 ],
					"destination" : [ "obj-16", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-53", 0 ],
					"destination" : [ "obj-54", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-27", 0 ],
					"destination" : [ "obj-26", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-80", 0 ],
					"destination" : [ "obj-79", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-33", 0 ],
					"destination" : [ "obj-35", 0 ],
					"hidden" : 0,
					"midpoints" : [ 164.5, 40.0, 164.5, 40.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-35", 0 ],
					"destination" : [ "obj-34", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-34", 0 ],
					"destination" : [ "obj-65", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-65", 0 ],
					"destination" : [ "obj-64", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-32", 0 ],
					"destination" : [ "obj-44", 0 ],
					"hidden" : 1,
					"midpoints" : [ 130.5, 391.5, 39.5, 391.5 ]
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
					"source" : [ "obj-63", 0 ],
					"destination" : [ "obj-62", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-61", 0 ],
					"destination" : [ "obj-60", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-57", 0 ],
					"destination" : [ "obj-56", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-59", 0 ],
					"destination" : [ "obj-58", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
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
					"source" : [ "obj-72", 0 ],
					"destination" : [ "obj-69", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-70", 0 ],
					"destination" : [ "obj-84", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-73", 0 ],
					"destination" : [ "obj-81", 0 ],
					"hidden" : 1,
					"midpoints" : [ 817.5, 65.0, 779.5, 65.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-67", 0 ],
					"destination" : [ "obj-66", 0 ],
					"hidden" : 1,
					"midpoints" : [ 964.5, 88.0, 783.5, 88.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-83", 0 ],
					"destination" : [ "obj-66", 0 ],
					"hidden" : 1,
					"midpoints" : [ 835.5, 88.0, 783.5, 88.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-82", 0 ],
					"destination" : [ "obj-66", 0 ],
					"hidden" : 1,
					"midpoints" : [ 799.5, 88.0, 783.5, 88.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-81", 0 ],
					"destination" : [ "obj-66", 0 ],
					"hidden" : 1,
					"midpoints" : [ 779.5, 88.0, 783.5, 88.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-6", 0 ],
					"destination" : [ "obj-5", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-73", 1 ],
					"destination" : [ "obj-83", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-6", 0 ],
					"destination" : [ "obj-7", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-73", 3 ],
					"destination" : [ "obj-6", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-73", 2 ],
					"destination" : [ "obj-67", 0 ],
					"hidden" : 1,
					"midpoints" : [ 852.833313, 65.0, 964.5, 65.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-73", 3 ],
					"destination" : [ "obj-17", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-17", 0 ],
					"destination" : [ "obj-20", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-79", 0 ],
					"destination" : [ "obj-68", 0 ],
					"hidden" : 1,
					"midpoints" : [ 90.5, 132.0, 78.5, 132.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-64", 0 ],
					"destination" : [ "obj-68", 0 ],
					"hidden" : 1,
					"midpoints" : [ 164.5, 132.0, 78.5, 132.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-62", 0 ],
					"destination" : [ "obj-68", 0 ],
					"hidden" : 1,
					"midpoints" : [ 249.5, 132.0, 78.5, 132.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-58", 0 ],
					"destination" : [ "obj-68", 0 ],
					"hidden" : 1,
					"midpoints" : [ 497.5, 132.0, 78.5, 132.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-56", 0 ],
					"destination" : [ "obj-68", 0 ],
					"hidden" : 1,
					"midpoints" : [ 410.5, 132.0, 78.5, 132.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-60", 0 ],
					"destination" : [ "obj-68", 0 ],
					"hidden" : 1,
					"midpoints" : [ 331.5, 132.0, 78.5, 132.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-26", 0 ],
					"destination" : [ "obj-68", 0 ],
					"hidden" : 1,
					"midpoints" : [ 11.5, 132.0, 78.5, 132.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-30", 0 ],
					"destination" : [ "obj-68", 0 ],
					"hidden" : 1,
					"midpoints" : [ 245.5, 75.0, 78.5, 75.0 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-44", 0 ],
					"destination" : [ "obj-32", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-12", 0 ],
					"destination" : [ "obj-73", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-36", 0 ],
					"destination" : [ "obj-40", 2 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-84", 0 ],
					"destination" : [ "obj-40", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-84", 1 ],
					"destination" : [ "obj-40", 1 ],
					"hidden" : 0,
					"midpoints" : [ 708.5, 260.5, 713.5, 260.5 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-40", 0 ],
					"destination" : [ "obj-25", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-40", 1 ],
					"destination" : [ "obj-25", 1 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-84", 0 ],
					"destination" : [ "obj-49", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-11", 0 ],
					"destination" : [ "obj-32", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-46", 0 ],
					"destination" : [ "obj-32", 1 ],
					"hidden" : 1,
					"midpoints" : [ 276.5, 336.5, 192.5, 336.5 ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-41", 0 ],
					"destination" : [ "obj-2", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-2", 0 ],
					"destination" : [ "obj-32", 0 ],
					"hidden" : 1,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-51", 0 ],
					"destination" : [ "obj-50", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
, 			{
				"patchline" : 				{
					"source" : [ "obj-50", 0 ],
					"destination" : [ "obj-59", 0 ],
					"hidden" : 0,
					"midpoints" : [  ]
				}

			}
 ]
	}

}
