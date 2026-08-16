package com.example.homeauto;

import android.annotation.SuppressLint;
import android.content.Intent;
import android.graphics.Bitmap;
import android.net.http.SslError;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.text.TextUtils;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.webkit.SslErrorHandler;
import android.webkit.WebChromeClient;
import android.webkit.WebResourceError;
import android.webkit.WebResourceRequest;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.TextView;

import androidx.activity.OnBackPressedCallback;
import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;

import com.example.homeauto.data.ServerConfig;
import com.example.homeauto.data.ServerConfigDbHelper;

public class WebViewActivity extends AppCompatActivity {

    private static final long LOAD_TIMEOUT_MS = 15000L;
    private static final long LOADING_DELAY_MS = 300L;

    private static final int ERROR_NONE = 0;
    private static final int ERROR_CONNECTION = 1;
    private static final int ERROR_TIMEOUT = 2;

    private WebView webView;
    private ProgressBar progressBar;
    private View loadingView;
    private View errorView;
    private TextView errorTitle;
    private TextView errorMessage;
    private Button topChangeServerButton;
    private String homeUrl;

    private int loadError;
    private boolean connectedOnce;

    private final Handler mainHandler = new Handler(Looper.getMainLooper());
    private final Runnable showLoadingRunnable = new Runnable() {
        @Override
        public void run() {
            loadingView.setVisibility(View.VISIBLE);
        }
    };
    private final Runnable loadTimeoutRunnable = new Runnable() {
        @Override
        public void run() {
            loadError = ERROR_TIMEOUT;
            clearStoredConfig();
            showError();
        }
    };

    @SuppressLint("SetJavaScriptEnabled")
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_web);

        ServerConfig config = new ServerConfigDbHelper(this).getConfig();
        if (config == null) {
            startActivity(new Intent(this, SetupActivity.class));
            finish();
            return;
        }
        homeUrl = config.getUrl();

        webView = findViewById(R.id.web_view);
        progressBar = findViewById(R.id.progress_bar);
        loadingView = findViewById(R.id.loading_view);
        errorView = findViewById(R.id.error_view);
        errorTitle = findViewById(R.id.error_title);
        errorMessage = findViewById(R.id.error_message);
        Button retryButton = findViewById(R.id.retry_button);
        Button changeServerButton = findViewById(R.id.change_server_button);
        topChangeServerButton = findViewById(R.id.top_change_server_button);

        WebSettings settings = webView.getSettings();
        settings.setJavaScriptEnabled(true);
        settings.setDomStorageEnabled(true);
        settings.setLoadWithOverviewMode(true);
        settings.setUseWideViewPort(true);

        webView.setWebViewClient(new WebViewClient() {
            @Override
            public void onPageStarted(WebView view, String url, Bitmap favicon) {
                loadError = ERROR_NONE;
                showError(false);
                hideLoading();
                mainHandler.removeCallbacks(loadTimeoutRunnable);
                mainHandler.postDelayed(loadTimeoutRunnable, LOAD_TIMEOUT_MS);
                mainHandler.postDelayed(showLoadingRunnable, LOADING_DELAY_MS);
                progressBar.setVisibility(View.VISIBLE);
                if (!connectedOnce) {
                    topChangeServerButton.setVisibility(View.VISIBLE);
                }
            }

            @Override
            public void onPageFinished(WebView view, String url) {
                mainHandler.removeCallbacks(loadTimeoutRunnable);
                mainHandler.removeCallbacks(showLoadingRunnable);
                hideLoading();
                progressBar.setVisibility(View.GONE);
                if (loadError == ERROR_NONE) {
                    connectedOnce = true;
                    topChangeServerButton.setVisibility(View.GONE);
                    showError(false);
                }
            }

            @Override
            public void onReceivedError(WebView view, WebResourceRequest request, WebResourceError error) {
                if (request.isForMainFrame()) {
                    loadError = ERROR_CONNECTION;
                    mainHandler.removeCallbacks(loadTimeoutRunnable);
                    mainHandler.removeCallbacks(showLoadingRunnable);
                    progressBar.setVisibility(View.GONE);
                    clearStoredConfig();
                    showError();
                }
            }

            @Override
            public void onReceivedSslError(WebView view, SslErrorHandler handler, SslError error) {
                loadError = ERROR_CONNECTION;
                mainHandler.removeCallbacks(loadTimeoutRunnable);
                mainHandler.removeCallbacks(showLoadingRunnable);
                progressBar.setVisibility(View.GONE);
                handler.cancel();
                clearStoredConfig();
                showError();
            }
        });

        webView.setWebChromeClient(new WebChromeClient() {
            @Override
            public void onProgressChanged(WebView view, int newProgress) {
                if (newProgress > 0 && newProgress < 100) {
                    progressBar.setProgress(newProgress);
                }
            }
        });

        retryButton.setOnClickListener(v -> retry());
        changeServerButton.setOnClickListener(v -> goToSetup());
        topChangeServerButton.setOnClickListener(v -> goToSetup());

        getOnBackPressedDispatcher().addCallback(this, new OnBackPressedCallback(true) {
            @Override
            public void handleOnBackPressed() {
                if (errorView.getVisibility() == View.VISIBLE) {
                    if (webView.canGoBack()) {
                        showError(false);
                        webView.goBack();
                        return;
                    }
                    setEnabled(false);
                    getOnBackPressedDispatcher().onBackPressed();
                    return;
                }
                if (webView.canGoBack()) {
                    webView.goBack();
                } else {
                    setEnabled(false);
                    getOnBackPressedDispatcher().onBackPressed();
                }
            }
        });

        ViewCompat.setOnApplyWindowInsetsListener(findViewById(R.id.web_root), (v, insets) -> {
            Insets systemBars = insets.getInsets(WindowInsetsCompat.Type.systemBars());
            v.setPadding(systemBars.left, systemBars.top, systemBars.right, systemBars.bottom);
            return insets;
        });

        loadPage();
    }

    private void loadPage() {
        showError(false);
        webView.loadUrl(homeUrl);
    }

    private void retry() {
        showError(false);
        if (TextUtils.isEmpty(webView.getUrl())) {
            webView.loadUrl(homeUrl);
        } else {
            webView.reload();
        }
    }

    private void clearStoredConfig() {
        new ServerConfigDbHelper(this).clearConfig();
    }

    private void hideLoading() {
        loadingView.setVisibility(View.GONE);
    }

    private void goToSetup() {
        clearStoredConfig();
        startActivity(new Intent(this, SetupActivity.class));
        finish();
    }

    private void showError() {
        showError(true);
    }

    private void showError(boolean show) {
        if (show) {
            mainHandler.removeCallbacks(showLoadingRunnable);
            hideLoading();
            topChangeServerButton.setVisibility(View.GONE);
            if (loadError == ERROR_TIMEOUT) {
                errorTitle.setText(R.string.web_error_title_timeout);
                errorMessage.setText(getString(R.string.web_error_message_timeout, homeUrl));
            } else {
                errorTitle.setText(R.string.web_error_title);
                errorMessage.setText(getString(R.string.web_error_message, homeUrl));
            }
            errorView.setVisibility(View.VISIBLE);
            webView.setVisibility(View.GONE);
            progressBar.setVisibility(View.GONE);
        } else {
            errorView.setVisibility(View.GONE);
            webView.setVisibility(View.VISIBLE);
        }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.menu_web, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(@NonNull MenuItem item) {
        int id = item.getItemId();
        if (id == R.id.action_refresh) {
            retry();
            return true;
        }
        if (id == R.id.action_change_server) {
            goToSetup();
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (webView != null) {
            webView.onResume();
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        if (webView != null) {
            webView.onPause();
        }
    }

    @Override
    protected void onDestroy() {
        mainHandler.removeCallbacks(loadTimeoutRunnable);
        mainHandler.removeCallbacks(showLoadingRunnable);
        if (webView != null) {
            webView.loadUrl("about:blank");
            webView.destroy();
        }
        super.onDestroy();
    }
}
