package com.example.homeauto.data;

public class ServerConfig {

    private final String ip;
    private final int port;

    public ServerConfig(String ip, int port) {
        this.ip = ip;
        this.port = port;
    }

    public String getIp() {
        return ip;
    }

    public int getPort() {
        return port;
    }

    public String getUrl() {
        return "http://" + ip + ":" + port + "/";
    }
}
