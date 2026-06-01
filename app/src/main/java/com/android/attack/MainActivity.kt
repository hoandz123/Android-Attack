package com.android.attack

import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import com.android.attack.databinding.ActivityMainBinding

class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        binding.sampleText.text = stringFromNative()
    }

    companion object {
        init {
            System.loadLibrary("attack")
        }

        @JvmStatic
        external fun stringFromNative(): String
    }
}
