package com.csounds.Csound6;

import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.PreferenceActivity;
import android.preference.ListPreference;

public class SettingsActivity extends PreferenceActivity implements SharedPreferences.OnSharedPreferenceChangeListener{
    public static final String KEY_LIST_PREFERENCE = "screenLayout";
    private ListPreference mListPreference;
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.settings);
        mListPreference=(ListPreference)findPreference(KEY_LIST_PREFERENCE);
    }
    @Override
    protected void onResume() {
        super.onResume();

        // Setup the initial values
        mListPreference.setSummary("Current value is " + mListPreference.getEntry().toString());

        // Set up a listener whenever a key changes
        getPreferenceScreen().getSharedPreferences().registerOnSharedPreferenceChangeListener(this);
    }

    @Override
    protected void onPause() {
        super.onPause();

        // Unregister the listener whenever a key changes
        getPreferenceScreen().getSharedPreferences().unregisterOnSharedPreferenceChangeListener(this);
    }

    public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key) {
        // Set new summary, when a preference value changes
        if (key.equals(KEY_LIST_PREFERENCE)) {
            mListPreference.setSummary("Current value is " + mListPreference.getEntry().toString());
        }
    }
}
