#pragma once

#if JUCE_LINUX

class CVUtil 
{
public:
    /*
    normalize
    in: CV input value(CV_MIN~CV_MAX)
    return 0.~1.
    */
    static float normalize(const int& in)
    {
        return (float)in / (float) CV_MAX;//0~1
    }


    /*
    normalizeWithEmphasizeCenter
    For Knob
    in: CV input value(CV_MIN~CV_MAX)
    return -1.~1.
    */
    static float normalizeWithEmphasizeCenter (const int& in)
    {
        float n = (float)(in - CV_HALF) / (float) CV_MAX;//-1~1
        return n * n * n;//-1~1
    }

    /*
    scale
    Scaling input cv value to outMin~outMax range.
    https://gist.github.com/AkiyukiOkayasu/21a142dec807de345e2e859703bbab27
    in: CV input value(CV_MIN~CV_MAX)
    return outMin~outMax
    */
    static float scale(const int& in, const float& outMin, const float& outMax)
    {
        const float outRange = outMax - outMin;
        const float a = outRange / (float)CV_MAX;
        const float b = (CV_MAX * outMin - outMax * CV_MIN) / CV_MAX;
        return a * (float)in + b;
    }

private:
    static constexpr int CV_MIN = 0;
    static constexpr int CV_MAX = 4095;    
    static constexpr int CV_HALF = 2047;

    // This calss can't be instantiated, it's just a holder for static methods.
    CVUtil() = delete;
};

#endif//JUCE_LINUX