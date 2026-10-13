package com.example.csoundapp;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.widget.Button;
import android.widget.SeekBar;
import android.widget.TextView;

import com.example.csoundapp.databinding.ActivityMainBinding;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'csoundapp' library on application startup.
    static {
        System.loadLibrary("csoundapp");
    }

    private ActivityMainBinding binding;
    private Button bt;
    private SeekBar sb, sb2;
    private TextView tv;
    boolean pressed = false;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        tv = binding.sampleText;
        tv.setText("stopped");
        bt = binding.button;
        bt.setSoundEffectsEnabled(false);
        bt.setText("play");
        bt.setOnClickListener(v -> {
            if(!pressed) {
                startCsound();
                pressed = true;
                tv.setText("playing");
                bt.setText("stop");
            } else {
                pressed = false;
                bt.setText("play");
                stopCsound();
                tv.setText("stopped");
            }
        });
        sb = binding.seekBar;
        sb.setProgress(100);
        sb.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                tv.setText(String.format("volume: %d", progress));
                setVolume(progress);
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {

            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                if(pressed) tv.setText("playing");
                else tv.setText("stopped");
            }
        });

        sb2 = binding.seekBar2;
        sb2.setProgress(50);
        sb2.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                float oct = progress*0.08f;
                tv.setText(String.format("octave: %.2f", oct));
                setOctave(oct);
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {

            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                if(pressed) tv.setText("playing");
                else tv.setText("stopped");
            }
        });

    }

    @Override
    protected void onDestroy(){
        stopCsound();
        super.onDestroy();
    }
    /**
     * Native methods from C++
     */
    public native void startCsound();
    public native void stopCsound();
    public native void setVolume(int v);

    public native void setOctave(float v);
}