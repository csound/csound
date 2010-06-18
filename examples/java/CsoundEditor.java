/*
 * Created on Apr 18, 2005
 *
 * TODO To change the template for this generated file go to
 * Window - Preferences - Java - Code Style - Code Templates
 */

// package csnd;

import csnd.Csound;
import csnd.CsoundFile;
import csnd.CsoundPerformanceThread;

import javax.swing.JFrame;

import java.io.File;
import java.lang.System;
import java.util.Properties;

import javax.swing.JFileChooser;
import javax.swing.filechooser.FileFilter;
import javax.swing.JToolBar;
import javax.swing.JButton;
import javax.swing.JTextField;
import javax.swing.JTabbedPane;
import javax.swing.JTextArea;
import javax.swing.JScrollPane;
import javax.swing.JOptionPane;

/**
 * @author mkg
 *
 * Rudimentary Java GUI front end for Csound,
 * mainly to demonstrate using the Java interface to Csound 5.
 */
public class CsoundEditor extends JFrame {

    private javax.swing.JPanel jContentPane = null;
    private JToolBar jToolBar = null;
    private JButton jButton = null;
    private JButton jButton1 = null;
    private JButton jButton2 = null;
    private JTextField commandTextField = null;
    private JButton jButton3 = null;
    private JTabbedPane jTabbedPane = null;
    private JScrollPane jScrollPane = null;
    private JTextArea orchestraTextArea = null;
    private JScrollPane jScrollPane1 = null;
    static JFileChooser fileChooser;
    static FileFilter csoundFileFilter;
    private JTextArea scoreTextArea = null;
    static {
	 System.loadLibrary("_jcsound");
        fileChooser = new JFileChooser();
        fileChooser.setAcceptAllFileFilterUsed(true);
        csoundFileFilter = new CsoundFileFilter();
        fileChooser.addChoosableFileFilter(csoundFileFilter);
    }
    public Properties properties = new Properties(System.getProperties());
    private String soundfilePlayer = "";
    public Csound csound = null;
    public CsoundFile csoundFile = null;
	public CsoundPerformanceThread csoundPerformanceThread = null;
    private JButton jButton4 = null;

    /**
     * This method initializes jToolBar
     *
     * @return javax.swing.JToolBar
     */
    private JToolBar getJToolBar() {
        if (jToolBar == null) {
            jToolBar = new JToolBar();
            jToolBar.add(getJButton());
            jToolBar.add(getJButton1());
            jToolBar.add(getJButton2());
            jToolBar.add(getJButton4());
            jToolBar.add(getJButton3());
            jToolBar.add(getCommandTextField());
        }
        return jToolBar;
    }

    public static class CsoundFileFilter extends FileFilter {

        /*
         * (non-Javadoc)
         *
         * @see java.io.FileFilter#accept(java.io.File)
         */
        public boolean accept(File pathname) {
            String filename = pathname.getAbsolutePath();
            if (filename.endsWith(".csd")) {
                return true;
            } else if (filename.endsWith(".orc")) {
                return true;
            } else if (filename.endsWith(".sco")) {
                return true;
            }
            return false;
        }

        /*
         * (non-Javadoc)
         *
         * @see javax.swing.filechooser.FileFilter#getDescription()
         */
        public String getDescription() {
            return "Csound files";
        }
    }

    /**
     * This method initializes jButton
     *
     * @return javax.swing.JButton
     */
    private JButton getJButton() {
        if (jButton == null) {
            jButton = new JButton();
            jButton.setText("Open");
            jButton.addActionListener(new java.awt.event.ActionListener() {
                public void actionPerformed(java.awt.event.ActionEvent e) {
                    fileChooser.showOpenDialog(CsoundEditor.this);
                    File file = fileChooser.getSelectedFile();
                    if (file != null) {
                        csoundFile.load(file.getAbsolutePath());
                        csoundFile.setFilename(file.getAbsolutePath());
                        updateView();
                    }
                }
            });
        }
        return jButton;
    }

    public void updateView() {
        String command = csoundFile.getCommand();
        commandTextField.setText(command);
        String orchestra = csoundFile.getOrchestra();
        orchestraTextArea.setText(orchestra);
        String score = csoundFile.getScore();
        scoreTextArea.setText(score);
        setTitle("[ C S O U N D ] " + csoundFile.getFilename());
    }

    public void updateModel() {
        String command = commandTextField.getText();
        csoundFile.setCommand(command);
        String orchestra = orchestraTextArea.getText();
        csoundFile.setOrchestra(orchestra);
        String score = scoreTextArea.getText();
        csoundFile.setScore(score);
    }

//    public void updateScore() {
//        csound.setScore(score.getCsoundScore());
//    }

    /**
     * This method initializes jButton1
     *
     * @return javax.swing.JButton
     */
    private JButton getJButton1() {
        if (jButton1 == null) {
            jButton1 = new JButton();
            jButton1.setText("Save");
            jButton1.addActionListener(new java.awt.event.ActionListener() {
                public void actionPerformed(java.awt.event.ActionEvent e) {
                    fileChooser.showSaveDialog(CsoundEditor.this);
                    File file = fileChooser.getSelectedFile();
                    if (file != null) {
                        updateModel();
                        csoundFile.save(file.getAbsolutePath());
                    }
                }
            });
        }
        return jButton1;
    }

    /**
     * This method initializes jButton2
     *
     * @return javax.swing.JButton
     */
    private JButton getJButton2() {
        if (jButton2 == null) {
            jButton2 = new JButton();
            jButton2.setText("Perform");
            jButton2.addActionListener(new java.awt.event.ActionListener() {
                public void actionPerformed(java.awt.event.ActionEvent e) {
                    perform();
                }
            });
        }
        return jButton2;
    }

    public void perform() {
        updateModel();
        try {
            csoundFile.exportForPerformance();
            csound.Compile("tmp.orc", "tmp.sco", "-odac");
	    csoundPerformanceThread = new CsoundPerformanceThread(csound);
            csoundPerformanceThread.Play();
        } catch (Exception x) {
            x.printStackTrace();
        }
    }

    /**
     * This method initializes commandTextField
     *
     * @return javax.swing.JTextField
     */
    private JTextField getCommandTextField() {
        if (commandTextField == null) {
            commandTextField = new JTextField();
            commandTextField.setFont(new java.awt.Font("Courier New",
                    java.awt.Font.PLAIN, 12));
        }
        return commandTextField;
    }

    /**
     * This method initializes jButton3
     *
     * @return javax.swing.JButton
     */
    private JButton getJButton3() {
        if (jButton3 == null) {
            jButton3 = new JButton();
            jButton3.setText("Play");
            jButton3.addActionListener(new java.awt.event.ActionListener() {
                public void actionPerformed(java.awt.event.ActionEvent e) {
                    try {
                        String soundfile = null;//csound.getOutputSoundfileName();
                        File file = new File(soundfile);
                        Runtime.getRuntime().exec(
                                soundfilePlayer + " " + file.getAbsolutePath());
                    } catch (Exception x) {
                        x.printStackTrace();
                    }
                }
            });
        }
        return jButton3;
    }

    /**
     * This method initializes jTabbedPane
     *
     * @return javax.swing.JTabbedPane
     */
    private JTabbedPane getJTabbedPane() {
        if (jTabbedPane == null) {
            jTabbedPane = new JTabbedPane();
            jTabbedPane.addTab("Orchestra", null, getJScrollPane(), null);
            jTabbedPane.addTab("Score", null, getJScrollPane1(), null);
        }
        return jTabbedPane;
    }

    /**
     * This method initializes jScrollPane
     *
     * @return javax.swing.JScrollPane
     */
    private JScrollPane getJScrollPane() {
        if (jScrollPane == null) {
            jScrollPane = new JScrollPane();
            jScrollPane.setViewportView(getOrchestraTextArea());
            jScrollPane.setFont(new java.awt.Font("Courier New",
                    java.awt.Font.PLAIN, 12));
        }
        return jScrollPane;
    }

    /**
     * This method initializes orchestraTextArea
     *
     * @return javax.swing.JTextArea
     */
    private JTextArea getOrchestraTextArea() {
        if (orchestraTextArea == null) {
            orchestraTextArea = new JTextArea();
            orchestraTextArea.setFont(new java.awt.Font("Courier New",
                    java.awt.Font.PLAIN, 12));

        }
        return orchestraTextArea;
    }

    /**
     * This method initializes jScrollPane1
     *
     * @return javax.swing.JScrollPane
     */
    private JScrollPane getJScrollPane1() {
        if (jScrollPane1 == null) {
            jScrollPane1 = new JScrollPane();
            jScrollPane1.setViewportView(getScoreTextArea());
        }
        return jScrollPane1;
    }

    /**
     * This method initializes scoreTextArea
     *
     * @return javax.swing.JTextArea
     */
    private JTextArea getScoreTextArea() {
        if (scoreTextArea == null) {
            scoreTextArea = new JTextArea();
            scoreTextArea.setFont(new java.awt.Font("Courier New",
                    java.awt.Font.PLAIN, 12));
        }
        return scoreTextArea;
    }

    /**
     * This method initializes jButton4
     *
     * @return javax.swing.JButton
     */
    private JButton getJButton4() {
        if (jButton4 == null) {
            jButton4 = new JButton();
            jButton4.setActionCommand("Stop");
            jButton4.setText("Stop");
            jButton4.addActionListener(new java.awt.event.ActionListener() {
                public void actionPerformed(java.awt.event.ActionEvent e) {
                    csoundPerformanceThread.Stop();
					csoundPerformanceThread.Join();
					csound.Cleanup();
                }
            });
        }
        return jButton4;
    }

    public static void main(String[] args) {
        CsoundEditor csoundEditor = new CsoundEditor();
        csoundEditor.setTitle("Csound");
        csoundEditor.move(100, 100);
        csoundEditor.setVisible(true);
    }

    /**
     * This is the default constructor
     */
    public CsoundEditor() {
        super();
	try {
	 properties.load(new java.io.FileInputStream(
	          "CsoundEditor.properties"));
	} catch (Exception x) {
	x.printStackTrace();
        }
        csound = new Csound();
	csoundPerformanceThread = new CsoundPerformanceThread(csound);
        csoundFile = new CsoundFile();
       soundfilePlayer = properties.getProperty("SoundfilePlayer",
         soundfilePlayer);
        initialize();
	String orchestra = properties.getProperty("DefaultOrchestra");
		if (orchestra != null) {
			csoundFile.load(orchestra);
	    csoundFile.setFilename(orchestra);
	    updateView();
		}
    }

    /**
     * This method initializes this
     *
     * @return void
     */
    private void initialize() {
        this.setSize(762, 396);
        this.setContentPane(getJContentPane());
        this.addWindowListener(new java.awt.event.WindowAdapter() {
            public void windowClosing(java.awt.event.WindowEvent e) {
                System.err.println("Closing window and exiting...");
                Runtime.getRuntime().exit(0);

            }
        });
        this.setTitle("JFrame");
    }

    /**
     * This method initializes jContentPane
     *
     * @return javax.swing.JPanel
     */
    private javax.swing.JPanel getJContentPane() {
        if (jContentPane == null) {
            jContentPane = new javax.swing.JPanel();
            jContentPane.setLayout(new java.awt.BorderLayout());
            jContentPane.add(getJToolBar(), java.awt.BorderLayout.NORTH);
            jContentPane.add(getJTabbedPane(), java.awt.BorderLayout.CENTER);
        }
        return jContentPane;
    }
} //  @jve:decl-index=0:visual-constraint="10,10"
