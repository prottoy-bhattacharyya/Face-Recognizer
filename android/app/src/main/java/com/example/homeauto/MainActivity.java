package com.example.homeauto;

import android.content.Intent;
import android.os.Bundle;

import androidx.appcompat.app.AppCompatActivity;

import com.example.homeauto.data.ServerConfig;
import com.example.homeauto.data.ServerConfigDbHelper;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        ServerConfig config = new ServerConfigDbHelper(this).getConfig();
        Class<?> target = (config != null) ? WebViewActivity.class : SetupActivity.class;
        startActivity(new Intent(this, target));
        finish();
    }
}
