/**
 * By alexirae@gmail.com
 *
 * http://www.musicdsp.org/showone.php?id=257
 *
 * This is a very simple class that I'm using in my plugins for smoothing
 * parameter changes that directly affect audio stream.
 * It's a 1-pole LPF, very easy on CPU.
 * You can specify the speed response of the parameter in ms.
 * and sampling rate.
 *
 */
#define TWO_PI 6.283185307179586476925286766559f

class CParamSmooth {
public:
    CParamSmooth(float smoothingTimeMs, double samplingRate) {
        a = exp(-TWO_PI / (smoothingTimeMs * 0.001f * samplingRate));
        b = 1.0f - a;
        z = 0.0f;
    }

    ~CParamSmooth() { }

    inline float process(float in) {
        return z = (in * b) + (z * a);
    }
private:
    float a, b, z;
};
