package com.example.getclock;

import androidx.appcompat.app.AppCompatActivity;

import android.annotation.SuppressLint;
import android.app.AlarmManager;
import android.content.Context;
import android.content.SharedPreferences;
import android.icu.text.SimpleDateFormat;
import android.icu.util.Calendar;
import android.icu.util.GregorianCalendar;
import android.icu.util.TimeZone;
import android.os.Bundle;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import com.android.volley.Request;
import com.android.volley.RequestQueue;
import com.android.volley.toolbox.StringRequest;
import com.android.volley.toolbox.Volley;

import java.util.Date;
import java.util.HashMap;
import java.util.Locale;
import java.util.Map;

public class MainActivity extends AppCompatActivity {

    public static final String myPref = "MyPref", lamp_id = "lamp_id", on_time = "on_time";

    @SuppressLint({"SetTextI18n", "ResourceType"})
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        AlarmManager alarmManager = (AlarmManager) getSystemService(Context.ALARM_SERVICE);
        SharedPreferences.Editor editor = getSharedPreferences(myPref,0).edit();

        RequestQueue queue = Volley.newRequestQueue(this);

        Button sendToLamp = findViewById(R.id.sendToLamp);
        Button setLampIp = findViewById(R.id.setLampIp);
        TextView nextAlarm = findViewById(R.id.nextAlarm);
        TextView onTime = findViewById(R.id.onTime);
        EditText lampIp = findViewById(R.id.lampIP);

        AlarmManager.AlarmClockInfo next = alarmManager.getNextAlarmClock();



        TimeZone mTimeZone = TimeZone.getDefault();
        Date now = new Date();
        int GMTOffset = mTimeZone.getOffset(now.getTime()) / 1000;

        Date nextAlarmDate = new Date(next.getTriggerTime());
        nextAlarmDate.setTime(nextAlarmDate.getTime() + mTimeZone.getOffset(now.getTime())); // remove GMT
        long alarmTimestamp = nextAlarmDate.getTime() / 1000; // epoch seconds


        SharedPreferences sp = getSharedPreferences(myPref,0);
        lampIp.setText(sp.getString(lamp_id,"localhost"));
        onTime.setText(sp.getString(on_time,"20"));

        nextAlarm.setText(new SimpleDateFormat(
                "EEEE dd MMMM HH:mm",
                Locale.FRANCE)
                .format(new Date(next.getTriggerTime())));

        setLampIp.setOnClickListener(v -> {
            editor.putString(lamp_id, lampIp.getText().toString());
            editor.apply();
            Toast.makeText(MainActivity.this, "Enregistré !", Toast.LENGTH_SHORT).show();

        });

        sendToLamp.setOnClickListener(v -> {
            String url = "http://" + sp.getString(lamp_id,"localhost") + "/";
            editor.putString(on_time, onTime.getText().toString());
            editor.apply();

            StringRequest request = new StringRequest(Request.Method.POST, url,
                    response -> {
                        Toast.makeText(MainActivity.this, "Envoyé à la lampe !", Toast.LENGTH_SHORT).show();
                    },
                    error -> {
                        Toast.makeText(MainActivity.this, "Erreur = " + error, Toast.LENGTH_SHORT).show();
                    }) {
                @Override
                protected Map<String, String> getParams() {

                    Map<String, String> params = new HashMap<String, String>();

                    Date durationTimestamp = new Date(Integer.parseInt(sp.getString(on_time,"20")) * 60000);
                    Date startTimestamp = new Date(nextAlarmDate.getTime() - durationTimestamp.getTime());

                    params.put("starttimestamp", String.valueOf(startTimestamp.getTime() / 1000)); //Epoch to seconds
                    params.put("durationtimestamp", String.valueOf(durationTimestamp.getTime() / 1000)); //Epoch to seconds
                    params.put("alarm", new SimpleDateFormat("EEE HH:mm", Locale.FRANCE)
                            .format(new Date(next.getTriggerTime())));
                    params.put("gmtoffset", String.valueOf(GMTOffset));

                    return params;
                }
            };

            queue.add(request);
        });

    }


}