/**
 * CSDPlayer.java: a simple working Java API example
 *
 */

import java.io.*;
import javax.swing.JFrame;
import java.io.File;
import java.lang.System;
import java.util.Properties;
import javax.swing.JFileChooser;
import javax.swing.filechooser.FileFilter;
import csnd.Csound;

/**
 *
 * @author  Victor Lazzarini, 2005
 */

public class CSDPlayer extends javax.swing.JFrame {

    private csperf cs;
    private Thread a;
    private JFileChooser choose;
    private boolean ready;
    private String csdfile = "";
    private javax.swing.JButton stopButton;
    private javax.swing.JButton pauseButton;
    private javax.swing.JButton playButton;
    private javax.swing.JButton openButton;
    private javax.swing.JButton quitButton;
    private javax.swing.JPanel mainPanel;

    public CSDPlayer() {
        initComponents();
        choose = new JFileChooser();
        FileFilter fil = new CSDFilter();
        choose.addChoosableFileFilter(fil);
        ready = false;
    }

    private void initComponents() {
        mainPanel = new javax.swing.JPanel();
        playButton = new javax.swing.JButton();
        pauseButton = new javax.swing.JButton();
        stopButton = new javax.swing.JButton();
        openButton = new javax.swing.JButton();
        quitButton = new javax.swing.JButton();

        quitButton.setText("quit");

        setDefaultCloseOperation(javax.swing.WindowConstants.DO_NOTHING_ON_CLOSE);
        playButton.setText("play");
        playButton.addActionListener(new java.awt.event.ActionListener() {
                public void actionPerformed(java.awt.event.ActionEvent evt) {
                    playButtonActionPerformed(evt);
                }
            });

        mainPanel.add(playButton);

        pauseButton.setText("pause");
        pauseButton.addActionListener(new java.awt.event.ActionListener() {
                public void actionPerformed(java.awt.event.ActionEvent evt) {
                    pauseButtonActionPerformed(evt);
                }
            });

        mainPanel.add(pauseButton);

        stopButton.setText("Stop");
        stopButton.addActionListener(new java.awt.event.ActionListener() {
                public void actionPerformed(java.awt.event.ActionEvent evt) {
                    stopButtonActionPerformed(evt);
                }
            });

        mainPanel.add(stopButton);

        openButton.setText("open CSD");
        openButton.addActionListener(new java.awt.event.ActionListener() {
                public void actionPerformed(java.awt.event.ActionEvent evt) {
                    openButtonActionPerformed(evt);
                }
            });

        mainPanel.add(openButton);

        quitButton.setText("quit");
        quitButton.addActionListener(new java.awt.event.ActionListener() {
                public void actionPerformed(java.awt.event.ActionEvent evt) {
                    quitButtonActionPerformed(evt);
                }
            });

        mainPanel.add(quitButton);

        getContentPane().add(mainPanel, java.awt.BorderLayout.NORTH);
        setTitle("CSDPlayer");
        pack();
    }

    private void quitButtonActionPerformed(java.awt.event.ActionEvent evt) {
        if(ready){
        cs.stop();
        try{
        while(a.isAlive());
        } catch (Exception e) {
            java.lang.System.exit(0);
        }
        }
        java.lang.System.exit(0);
    }

    private void openButtonActionPerformed(java.awt.event.ActionEvent evt) {
        if(choose.showOpenDialog(this) != choose.CANCEL_OPTION){
            File csd = choose.getSelectedFile();
            if(ready) cs.stop();
            csdfile = csd.getAbsolutePath();
            setTitle("CSDPlayer " + csd.getName());
        }

    }

    private void stopButtonActionPerformed(java.awt.event.ActionEvent evt) {
        if(ready) {
            cs.stop();
            ready = false;
        }
    }

    private void playButtonActionPerformed(java.awt.event.ActionEvent evt) {
        if(!ready && csdfile != ""){
         cs = new csperf(csdfile);
         ready = true;
        if(!cs.isOn()) {
                a = new Thread(cs);
                a.start();
        } else cs.play();

        }
    }

    private void pauseButtonActionPerformed(java.awt.event.ActionEvent evt) {
        if(ready)
            cs.pause();
    }

    public static void main(String args[]) {
        java.awt.EventQueue.invokeLater(new Runnable() {
                public void run() {
                    new CSDPlayer().setVisible(true);
                }
            });
    }

    public class csperf implements Runnable {

        Csound csp;
        boolean on;
        boolean pause;
        String  csd;

        public csperf(String s){
            csd = s;
            on = false;
            pause = false;
                csp  = new Csound();
        }

        public void run() {
            try{
                int res = csp.Compile(csd, "-m0", "-d");
                if(res == 0){
                    on = true;
                    while(on){
                        if(!pause) csp.PerformKsmps();
                         }
                }
            }
            catch (Exception e) {
                java.lang.System.err.println("Could not Perform...\n");
                java.lang.System.exit(1);
            }
                csp.Reset();
        }
        public void stop() {
            on = false;
        }
        public void pause(){
            if(!pause) pause = true;
            else pause = false;
        }
        public void play(){
            pause = false;
        }
        public boolean isOn(){
            return on;
        }
    };

   public static class CSDFilter extends FileFilter {

       public boolean accept(File pathname) {
           String csd = pathname.getAbsolutePath();
           if (csd.endsWith(".csd")) return true;
           else return false;
        }
       public String getDescription() {
            return "Unified Csound files";
        }
   };

}
