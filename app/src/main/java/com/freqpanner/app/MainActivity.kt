package com.freqpanner.app

import android.app.Activity
import android.media.AudioFormat
import android.media.AudioRecord
import android.media.AudioTrack
import android.media.MediaRecorder
import android.os.Bundle
import android.widget.Button
import android.widget.Switch
import android.widget.TextView
import android.widget.Toast

class MainActivity : Activity() {
    
    private var audioThread: Thread? = null
    private var isRunning = false
    
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        
        val toggleButton = findViewById<Button>(R.id.toggleButton)
        val statusText = findViewById<TextView>(R.id.statusText)
        val intervalSwitch = findViewById<Switch>(R.id.intervalSwitch)
        
        nativeSetSamplingRate(44100)
        nativeSetInterval(2000)
        
        toggleButton.setOnClickListener {
            if (isRunning) {
                stopProcessing()
                toggleButton.text = "START"
                statusText.text = "Status: OFF"
                statusText.setTextColor(android.graphics.Color.RED)
            } else {
                startProcessing()
                toggleButton.text = "STOP"
                statusText.text = "Status: ACTIVE"
                statusText.setTextColor(android.graphics.Color.GREEN)
            }
        }
        
        intervalSwitch.setOnCheckedChangeListener { _, isChecked ->
            nativeSetInterval(if (isChecked) 1000 else 2000)
            Toast.makeText(this, if (isChecked) "Fast switch (1s)" else "Slow switch (2s)", Toast.LENGTH_SHORT).show()
        }
    }
    
    private fun startProcessing() {
        isRunning = true
        nativeSetEnabled(true)
        
        audioThread = Thread {
            val sampleRate = 44100
            val bufferSize = AudioRecord.getMinBufferSize(
                sampleRate,
                AudioFormat.CHANNEL_IN_STEREO,
                AudioFormat.ENCODING_PCM_FLOAT
            )
            
            val audioRecord = AudioRecord(
                MediaRecorder.AudioSource.MIC,
                sampleRate,
                AudioFormat.CHANNEL_IN_STEREO,
                AudioFormat.ENCODING_PCM_FLOAT,
                bufferSize
            )
            
            val audioTrack = AudioTrack(
                AudioTrack.MODE_STREAM,
                sampleRate,
                AudioFormat.CHANNEL_OUT_STEREO,
                AudioFormat.ENCODING_PCM_FLOAT,
                bufferSize,
                AudioTrack.MODE_STREAM
            )
            
            audioRecord.startRecording()
            audioTrack.play()
            
            val buffer = FloatArray(bufferSize / 2)
            
            while (isRunning) {
                val read = audioRecord.read(buffer, 0, buffer.size, AudioRecord.READ_BLOCKING)
                if (read > 0) {
                    nativeProcess(buffer, read / 2)
                    audioTrack.write(buffer, 0, read, AudioTrack.WRITE_BLOCKING)
                }
            }
            
            audioRecord.stop()
            audioRecord.release()
            audioTrack.stop()
            audioTrack.release()
        }
        audioThread?.start()
    }
    
    private fun stopProcessing() {
        isRunning = false
        nativeSetEnabled(false)
        audioThread?.interrupt()
        audioThread = null
    }
    
    override fun onDestroy() {
        super.onDestroy()
        stopProcessing()
    }
    
    private external fun nativeSetEnabled(enable: Boolean)
    private external fun nativeSetInterval(intervalMs: Int)
    private external fun nativeSetSamplingRate(rate: Int)
    private external fun nativeProcess(buffer: FloatArray, size: Int)
    
    companion object {
        init {
            System.loadLibrary("frequency_panner")
        }
    }
}
