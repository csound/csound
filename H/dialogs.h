/*  
    dialogs.h:

    Copyright (C) 1995 John ffitch

    This file is part of Csound.

    The Csound Library is free software; you can redistribute it
    and/or modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    Csound is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with Csound; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/


#define         ORCH            601
#define         SCORE           602
#define         BUTTON_SF       603
#define         SOUND           604
#define         BUTTON_Sn       605
#define         BUTTON_A        606
#define         BUTTON_B        607
#define         BUTTON_C        608
#define         BUTTON_D        609
#define         BUTTON_U8       610
#define         BUTTON_8        611
#define         BUTTON_16       612
#define         BUTTON_32       613
#define         BUTTON_FL       614
#define         BUTTON_NG       615
#define         BUTTON_AG       616
#define         BUTTON_FG       617
#define         BUTTON_PS       618
#define         BUTTON_PLY      619
#define         BUTTON_CNT      620
#define         BUTTON_LOG      621
#define         BUTTON_HLP      622
#define         OTHERS          623
#define         UTIL            624
#define         BUTTON_OF       625
#define         BUTTON_EW       626
#define         BUTTON_EO       627
#define         BUTTON_ES       628
#define         IDC_WAVEOUTDEV  629
#define         X_CSCORE        630
#define         X_INITTIME      631
#define         X_NOSOUND       632
#define         X_VERBOS        633
#define         X_REWRITE       634
#define         X_SCOT          635
#define         X_HEART         636
#define         X_NOTIFY        637
#define         X_T1            638
#define         X_SRATE         639
#define         X_T2            640
#define         X_KRATE         641
#define         X_T3            642
#define         X_MSGLEV        643
#define         X_T4            644
#define         X_BEATS         645
#define         X_T5            646
#define         X_MIDIFILE      647
#define         X_MIDI          648
#define         X_T6            649
#define         X_T7            650
#define         X_PIDIIN        651
#define         X_XTRFILE       652
#define         X_EXTRACT       653
#define         X_T8            654
#define         X_BUFFER        655
#define         X_INPUT         656
#define         X_T9            657
#define         X_IN            658
#define         IDC_WAVEOUTTXT  659
#define         X_T0            660
#define         X_T10           661
#define         X_LBUFFER       662
#define         X_TRKEND        663
#define         X_MIDIIN        664
#define         X_MIDIINDEV     665
#define         PROJ            666
#define         SLIDE           667
#define         ENVIRON         668
#define         X_KEEPTMP       669
#define         U_HETRO         670
#define         U_LPANAL        671
#define         U_PVANAL        672
#define         U_CVANAL        673
#define         U_SNDINFO       674
#define         U_OPCODES       675
#define         U_ALLCODES      676
#define         U_PVLOOK        677
#define         BUTTON_PK       678
#define         BUTTON_DAC      679
#define         C_T1            680
#define         C_INPUT         681
#define         C_IN            682
#define         C_T2            683
#define         C_OUT           684
#define         C_OUTPUT        685
#define         C_SAMPLE        686
#define         C_T3            687
#define         C_BEGIN         688
#define         C_T4            689
#define         C_DUR           690
#define         C_T5            691
#define         C_FF            692
#define         C_C5            693
#define         C_C1            694
#define         C_C2            695
#define         C_C3            696
#define         C_C4            697
#define         BUTTON_MIDI     698
#define         U_DNOISE        699
#define         X_DITHER        700

#define         C_FUND          718
#define         C_HARM          719
#define         C_MAX           720
#define         C_MIN           721
#define         C_NUM           722
#define         C_FILTER        723

#define         P_VERBOSE       764
#define         P_GAIN          765
#define         P_SHARP         766
#define         C_POLES         767
#define         P_NUM           768
#define         C_SLICE         769
#define         C_PCHLOW        770
#define         C_PCHHIGH       771
#define         C_VERBOSE       772
#define         C_DEBUG         773
#define         C_GRAPHICS      774
#define         C_OLDFORM       775
#define         P_THRESH        776
#define         P_END           777
#define         P_ENDS          778
#define         P_BEGINS        779
#define         C_DISP          780
#define         C_FRAME         781
#define         C_OVLP          782
#define         C_FRAMEI        783
#define         C_LATCH         784
#define         P_DECIM         785
#define         P_OVERLAP       786
#define         P_LENGTH        787
#define         P_BANDS         788
#define         P_INN           789
#define         P_T1            790
#define         P_INPUT         791
#define         P_IN            792
#define         P_T2            793
#define         P_OUT           794
#define         P_OUTPUT        795
#define         P_SAMPLE        796
#define         P_T3            797
#define         P_BEGIN         798
#define         P_T4            799
#define         P_DUR           800
#define         P_T5            801
#define         P_FF            802
#define         P_FRAME         803
#define         P_C1            804
#define         P_C2            805
#define         P_C3            806
#define         P_C4            807
#define         P_T6            808
#define         P_WIND          809
#define         P_T7            810
#define         P_HOP           811
#define         P_T8            812
#define         P_LATCH         813
#define         P_T9            814
#define         P_DISP          815
#define         P_FBIN          816
#define         P_LBIN          817
#define         P_FFRAME        818
#define         P_LFRAME        819
#define         P_PINT          820
/*RWD 5:2001*/
#define         BUTTON_24       821

//RWD: VC++ needs some numbers for these
#ifndef CWIN
#  define CWIN          847
#endif
#ifndef DREAM
#  define DREAM         848
#endif
//
#define IDD_SLIDERS                     901
#define IDD_SETSLIDER                   902
#define IDC_SLIDER1                     9001
#define IDC_EDIT1                       9002
#define IDC_BUTTON1                     9003
#define IDC_SLIDER2                     9004
#define IDC_EDIT2                       9005
#define IDC_BUTTON2                     9006
#define IDC_SLIDER3                     9007
#define IDC_EDIT3                       9008
#define IDC_BUTTON3                     9009
#define IDC_SLIDER4                     9010
#define IDC_EDIT4                       9011
#define IDC_BUTTON4                     9012
#define IDC_SLIDER5                     9013
#define IDC_EDIT5                       9014
#define IDC_BUTTON5                     9015
#define IDC_SLIDER6                     9016
#define IDC_EDIT6                       9017
#define IDC_BUTTON6                     9018
#define IDC_SLIDER7                     9019
#define IDC_EDIT7                       9020
#define IDC_BUTTON7                     9021
#define IDC_SLIDER8                     9022
#define IDC_EDIT8                       9023
#define IDC_BUTTON8                     9024
#define IDC_SLIDER9                     9025
#define IDC_EDIT9                       9026
#define IDC_BUTTON9                     9027
#define IDC_SLIDER10                    9028
#define IDC_EDIT10                      9029
#define IDC_BUTTON10                    9030

#define RG_NAME                         9031
#define RG_MIN                          9032
#define RG_MAX                          9033
#define RG_VAL                          9034

#define SE_SSDIR                        9501
#define SE_SFDIR                        9502
#define SE_SADIR                        9503
#define SE_MIDIDEV                      9504
#define SE_NOSTOP                       9505
#define SE_SSBUTTON                     9506
#define SE_SFBUTTON                     9507
#define SE_SABUTTON                     9508

#define IDD_BUTTONS                     903
#define IDD_CHECKS                      904
#define IDC_CHECK1                      9040
#define IDC_CHECK2                      9041
#define IDC_CHECK3                      9042
#define IDC_CHECK4                      9043
#define IDC_NAME1                       9044
#define IDC_NAME2                       9045
#define IDC_NAME3                       9046
#define IDC_NAME4                       9047

