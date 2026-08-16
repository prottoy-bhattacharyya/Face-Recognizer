package com.example.homeauto.data;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;

public class ServerConfigDbHelper extends SQLiteOpenHelper {

    private static final String DB_NAME = "homeauto.db";
    private static final int DB_VERSION = 1;

    private static final String TABLE = "server_config";
    private static final String COL_ID = "_id";
    private static final String COL_IP = "ip";
    private static final String COL_PORT = "port";

    private static final int ROW_ID = 1;

    public ServerConfigDbHelper(Context context) {
        super(context, DB_NAME, null, DB_VERSION);
    }

    @Override
    public void onCreate(SQLiteDatabase db) {
        db.execSQL("CREATE TABLE " + TABLE + " (" +
                COL_ID + " INTEGER PRIMARY KEY, " +
                COL_IP + " TEXT NOT NULL, " +
                COL_PORT + " INTEGER NOT NULL)");
    }

    @Override
    public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
        db.execSQL("DROP TABLE IF EXISTS " + TABLE);
        onCreate(db);
    }

    public void saveConfig(String ip, int port) {
        SQLiteDatabase db = getWritableDatabase();
        ContentValues values = new ContentValues();
        values.put(COL_ID, ROW_ID);
        values.put(COL_IP, ip.trim());
        values.put(COL_PORT, port);
        db.insertWithOnConflict(TABLE, null, values, SQLiteDatabase.CONFLICT_REPLACE);
    }

    public ServerConfig getConfig() {
        SQLiteDatabase db = getReadableDatabase();
        try (Cursor cursor = db.query(TABLE, null, COL_ID + " = ?",
                new String[]{String.valueOf(ROW_ID)}, null, null, null)) {
            if (cursor.moveToFirst()) {
                return new ServerConfig(
                        cursor.getString(cursor.getColumnIndexOrThrow(COL_IP)),
                        cursor.getInt(cursor.getColumnIndexOrThrow(COL_PORT)));
            }
        }
        return null;
    }

    public void clearConfig() {
        SQLiteDatabase db = getWritableDatabase();
        db.delete(TABLE, COL_ID + " = ?", new String[]{String.valueOf(ROW_ID)});
    }
}
