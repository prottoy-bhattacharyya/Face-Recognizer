package com.example.homeauto;

import android.content.Intent;
import android.os.Bundle;
import android.text.TextUtils;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;

import com.example.homeauto.data.ServerConfigDbHelper;

public class SetupActivity extends AppCompatActivity {

    private EditText ipInput;
    private EditText portInput;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_setup);

        ipInput = findViewById(R.id.ip_input);
        portInput = findViewById(R.id.port_input);
        Button saveButton = findViewById(R.id.save_button);

        saveButton.setOnClickListener(v -> saveAndConnect());

        ViewCompat.setOnApplyWindowInsetsListener(findViewById(R.id.setup_root), (v, insets) -> {
            Insets systemBars = insets.getInsets(WindowInsetsCompat.Type.systemBars());
            v.setPadding(systemBars.left, systemBars.top, systemBars.right, systemBars.bottom);
            return insets;
        });
    }

    private void saveAndConnect() {
        String ip = ipInput.getText().toString().trim();
        String portText = portInput.getText().toString().trim();

        if (TextUtils.isEmpty(ip)) {
            ipInput.setError(getString(R.string.error_ip_empty));
            ipInput.requestFocus();
            return;
        }

        int port;
        try {
            port = Integer.parseInt(portText);
        } catch (NumberFormatException e) {
            port = -1;
        }
        if (port < 1 || port > 65535) {
            portInput.setError(getString(R.string.error_port_invalid));
            portInput.requestFocus();
            return;
        }

        new ServerConfigDbHelper(this).saveConfig(ip, port);
        Toast.makeText(this, R.string.saved_connecting, Toast.LENGTH_SHORT).show();
        startActivity(new Intent(this, WebViewActivity.class));
        finish();
    }
}
