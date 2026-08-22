#include <jni.h>
#include <android/log.h>
#include <math.h>
#include <string.h>

#define LOG_TAG "FrequencyPanner"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// Simple IIR filter for low-pass
class LowPassFilter {
private:
    float alpha = 0.1f;
    float prevOutput = 0.0f;
    
public:
    void setCutoff(float cutoff, float sampleRate) {
        float rc = 1.0f / (2.0f * M_PI * cutoff);
        float dt = 1.0f / sampleRate;
        alpha = dt / (rc + dt);
    }
    
    float process(float input) {
        float output = alpha * input + (1.0f - alpha) * prevOutput;
        prevOutput = output;
        return output;
    }
    
    void reset() {
        prevOutput = 0.0f;
    }
};

// Global state
static LowPassFilter lowLeft, lowRight;
static bool bassOnRight = true;
static bool enabled = false;
static uint32_t sampleRate = 44100;
static uint32_t samplesUntilSwitch = 0;
static uint32_t switchInterval = 2000;

extern "C" {

JNIEXPORT void JNICALL
Java_com_freqpanner_app_MainActivity_nativeSetEnabled(JNIEnv *env, jobject thiz, jboolean enable) {
    enabled = enable;
    lowLeft.reset();
    lowRight.reset();
    bassOnRight = true;
    samplesUntilSwitch = (sampleRate * switchInterval) / 1000;
    LOGI("Frequency Panner %s", enable ? "enabled" : "disabled");
}

JNIEXPORT void JNICALL
Java_com_freqpanner_app_MainActivity_nativeSetInterval(JNIEnv *env, jobject thiz, jint intervalMs) {
    switchInterval = intervalMs;
    samplesUntilSwitch = (sampleRate * switchInterval) / 1000;
    LOGI("Switch interval: %d ms", intervalMs);
}

JNIEXPORT void JNICALL
Java_com_freqpanner_app_MainActivity_nativeSetSamplingRate(JNIEnv *env, jobject thiz, jint rate) {
    sampleRate = rate;
    float cutoff = 250.0f;
    lowLeft.setCutoff(cutoff, sampleRate);
    lowRight.setCutoff(cutoff, sampleRate);
    samplesUntilSwitch = (sampleRate * switchInterval) / 1000;
}

JNIEXPORT void JNICALL
Java_com_freqpanner_app_MainActivity_nativeProcess(JNIEnv *env, jobject thiz,
                                                     jfloatArray buffer, jint size) {
    if (!enabled) return;
    
    jfloat *samples = env->GetFloatArrayElements(buffer, nullptr);
    
    for (int i = 0; i < size; i++) {
        float left = samples[2 * i];
        float right = samples[2 * i + 1];
        
        // Split into low and high bands
        float leftLow = lowLeft.process(left);
        float leftHigh = left - leftLow;
        
        float rightLow = lowRight.process(right);
        float rightHigh = right - rightLow;
        
        // Pan bands
        if (bassOnRight) {
            samples[2 * i] = leftHigh * 1.2f + rightLow * 0.3f;
            samples[2 * i + 1] = rightLow * 1.2f + leftHigh * 0.3f;
        } else {
            samples[2 * i] = leftLow * 1.2f + rightHigh * 0.3f;
            samples[2 * i + 1] = rightHigh * 1.2f + leftLow * 0.3f;
        }
        
        // Update switch timer
        if (samplesUntilSwitch > 0) {
            samplesUntilSwitch--;
        } else {
            bassOnRight = !bassOnRight;
            samplesUntilSwitch = (sampleRate * switchInterval) / 1000;
        }
    }
    
    env->ReleaseFloatArrayElements(buffer, samples, 0);
}

} // extern "C"
