/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package cs6editor;

import javax.swing.JFrame;
import javax.swing.SwingUtilities;

/**
 *
 * @author stevenyi
 */
public class CS6Editor {

    /**
     * @param args the command line arguments
     */
    public static void main(String[] args) {
        SwingUtilities.invokeLater(new Runnable() {

            @Override
            public void run() {
                MainWindow w = new MainWindow();
                w.setSize(800, 600);
                w.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
                w.setVisible(true);
            }
        });
    }
}
